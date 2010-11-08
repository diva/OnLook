/** 
 * @file ascentuploadbrowser.h
 * @Author Duncan Garrett (Hg Beeks)
 * Meant as a replacement to using a system file browser for uploads.
 *
 * Created August 27 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * k ilu bye
 */

#include "llviewerprecompiledheaders.h"

#include "ascentuploadbrowser.h"

//UI Elements
#include "llbutton.h" //Buttons
#include "llcombobox.h" //Combo dropdowns
#include "llscrolllistctrl.h" //List box for filenames
#include "lluictrlfactory.h" //Loads the XUI

// project includes
#include "llresmgr.h"
#include "llsdserialize.h" //XML Parsing - Probably not needed
#include "llviewercontrol.h"
#include "llviewerwindow.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLSD					ASFloaterUploadBrowser::mUploaderSettings;
ASFloaterUploadBrowser* ASFloaterUploadBrowser::sInstance = NULL;


///----------------------------------------------------------------------------
/// Class ASFloaterUploadBrowser
///----------------------------------------------------------------------------

// Default constructor
ASFloaterUploadBrowser::ASFloaterUploadBrowser() 
:	LLFloater(std::string("floater_upload_browser"), 
		std::string("FloaterUploadRect"), 
		LLStringUtil::null)
{	
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_upload_browser.xml");

	mUploaderSettings.clear();
	mUploaderSettings = gSavedSettings.getLLSD("AscentUploadSettings");

	mPathName = mUploaderSettings["ActivePath"].asString();
	if (mPathName == "None")
		mPathName = gDirUtilp->getExecutableDir();
	mFilterType = "None";
	
	
	//File list ------------------------------------------------------
	mFileList = getChild<LLScrollListCtrl>("file_list");
	childSetCommitCallback("file_list", onClickFile, this);
	childSetDoubleClickCallback("file_list", onDoubleClick);

	//Above File List ------------------------------------------------
	
	mBookmarkCombo = getChild<LLComboBox>("bookmark_combo");
	S32 index;
	for (index = 0; index < mUploaderSettings["Bookmarks"].size(); index++)
	{
		std::string bookmark = mUploaderSettings["Bookmarks"][index].asString();
		if (bookmark != "")
			mBookmarkCombo->add(bookmark, ADD_BOTTOM);
	}

	mDriveCombo = getChild<LLComboBox>("drive_combo");
	childSetCommitCallback("drive_combo", onChangeDrives, this);
	//This is so unbelievably shitty I can't handle it -HgB
	std::string drive_letters[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"}; //Oh my god it's somehow worse now -HgB
	
	mDriveCombo->removeall();
	for (index = 0; index < 26; index++)
	{
		std::string dir = drive_letters[index] + ":";
		S32 file_count = gDirUtilp->countFilesInDir(dir + gDirUtilp->getDirDelimiter(), "*.*");
		if(file_count)
		{
			mDriveCombo->add(dir, ADD_BOTTOM);
		}
	}
	
	childSetAction("directory_button", onClickFilepathGoto, this);
	childSetCommitCallback("dir_path", onDirCommit, (void*)this);
	//Below File List ------------------------------------------------
	childSetCommitCallback("file_filter_combo", onUpdateFilter, this);
	
	
	refresh();
	mFileList->sortByColumn(std::string("file_name"), TRUE);
	mFileList->sortByColumn(std::string("file_type"), TRUE);
	childHide("multiple_uploads_label");
	childHide("bad_image_text");
}

// Destroys the object
ASFloaterUploadBrowser::~ASFloaterUploadBrowser()
{
	sInstance = NULL;
}

void ASFloaterUploadBrowser::onDirCommit(LLUICtrl* ctrl, void* data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)data;
	if (panelp)
	{
		panelp->onClickFilepathGoto(data);
	}
}

void ASFloaterUploadBrowser::updateBrowser(void* data, std::string new_path)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)data;
	if ((new_path != panelp->mPathName)||(new_path == ""))
	{
		panelp->mPathName = new_path;
		panelp->refresh();
		panelp->mFileList->selectFirstItem();
		panelp->childSetValue("asset_name", "");
	}
}

//static
void ASFloaterUploadBrowser::onClickFilepathGoto(void* data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)data;
	std::string new_path = panelp->childGetValue("dir_path");
	panelp->updateBrowser(data, new_path);
}

void ASFloaterUploadBrowser::onClickFile(LLUICtrl* ctrl, void* user_data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)user_data;
	panelp->refreshUploadOptions();
}

void ASFloaterUploadBrowser::onChangeDrives(LLUICtrl* ctrl, void* user_data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)user_data;
	if (panelp->mDriveCombo->getSelectedValue().asString() != panelp->mFilterType)
	{
		panelp->updateBrowser(user_data, panelp->mDriveCombo->getSelectedValue().asString());
	}
}

void ASFloaterUploadBrowser::onUpdateFilter(LLUICtrl* ctrl, void* user_data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)user_data;
	LLComboBox* combo = panelp->getChild<LLComboBox>("file_filter_combo");
	if (combo->getSelectedValue().asString() != panelp->mFilterType)
	{
		panelp->mFilterType = "";
		panelp->mFilterType = combo->getSelectedValue().asString();
		panelp->updateBrowser(user_data, "");
	}
}

void ASFloaterUploadBrowser::refreshUploadOptions()
{
	if (!mFileList->isEmpty())
	{
		if(!mFileList->getSelectedIDs().count())
		{
			llinfos << "No selection, clearing field." << llendl;
			childSetValue("asset_name", "");
		}
		else 
		{
			if (mFileList->getFirstSelected()->getColumn(LIST_ASSET_TYPE)->getValue().asInteger() == LIST_TYPE_FILE)
			{
				std::string name;
				bool show_tex = false;
				bool show_snd = false;
				bool show_anm = false;
				bool show_multiple = false;
				if (mFileList->getAllSelected().size() > 1)
				{
					
					llinfos << "Selected multiple files." << llendl;
					name = "(Multiple)";
					show_multiple = true;
					/*LLButton* expand_button = getChild<LLButton>("expand_collapse_btn");
					expand_button->setLabelArg("[COST]", std::string(mFileList->getAllSelected().size() * 10));*/
					childSetValue("multiple_uploads_label", "Multiple files selected. Total cost is: " + llformat("%d", mFileList->getAllSelected().size() * 10));
				}
				else
				{
					int type = mFileList->getFirstSelected()->getColumn(LIST_FILE_TYPE)->getValue().asInteger();
					llinfos << "Selected a file, type" << type << llendl;
					if (type == FILE_TEXTURE)
					{
						show_tex = true;
					}
					else if (type == FILE_SOUND)
					{
						show_snd = true;
					}
					else if (type == FILE_ANIMATION)
					{
						show_anm = true;
					}
					name = mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString();
					
				}
				childSetVisible("texture_preview_label", (show_tex && !show_multiple));
				childSetVisible("texture_preview_combo", (show_tex && !show_multiple));
				childSetVisible("multiple_uploads_label", show_multiple);
				childSetValue("asset_name", name);
			}
		}
	}
}

void ASFloaterUploadBrowser::onDoubleClick(void* user_data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)user_data;
	panelp->handleDoubleClick();
}

void ASFloaterUploadBrowser::handleDoubleClick()
{
	if (mFileList->getFirstSelected()->getColumn(LIST_ASSET_TYPE)->getValue().asInteger() == LIST_TYPE_PARENT)
	{
		S32 dirLimiterIndex = mPathName.find_last_of(gDirUtilp->getDirDelimiter());
		mPathName = mPathName.substr(0, dirLimiterIndex);
		refresh();
		mFileList->selectFirstItem();
	}
	else if (mFileList->getFirstSelected()->getColumn(LIST_ASSET_TYPE)->getValue().asInteger() == LIST_TYPE_FOLDER)
	{
		//Make sure that it's an actual folder so you don't get stuck - Specifically meant for files with no extension. -HgB
		std::string new_path = mPathName + gDirUtilp->getDirDelimiter() + mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString();
		S32 file_count = gDirUtilp->countFilesInDir(new_path, "*.*");
		if(!file_count)
			return;
		mPathName = mPathName + gDirUtilp->getDirDelimiter() + mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString();
		refresh();
		mFileList->selectFirstItem();

	}
	childSetValue("asset_name", "");
}

void ASFloaterUploadBrowser::refresh()
{
	std::string filename;
	std::string fullPath = mPathName + gDirUtilp->getDirDelimiter();
	mFileList->deselectAllItems();
	mFileList->deleteAllItems();
	childSetValue("dir_path", gDirUtilp->getDirName(fullPath));
	mUploaderSettings["ActivePath"] = mPathName;
	gSavedSettings.setLLSD("AscentUploadSettings", mUploaderSettings);
	gDirUtilp->getNextFileInDir(gDirUtilp->getChatLogsDir(),"*", filename, false); //Clears the last file
	bool found = true;
	S32 file_count = 0;
	while(found) 
	{
		found = gDirUtilp->getNextFileInDir(fullPath, "*.*", filename, false);
		if(found)
		{
			S32 periodIndex = filename.find_last_of(".");
			std::string extension = filename.substr(periodIndex + 1, filename.length() - 1);
			std::string extensionL = utf8str_tolower(extension);
			LLSD element;
			element["path"] = mPathName + filename;

			LLSD& filename_column = element["columns"][LIST_FILE_NAME];
			filename_column["column"] = "file_name";
			filename_column["font"] = "SANSSERIF";
			filename_column["font-style"] = "NORMAL";	
			
			LLSD& filetype_column = element["columns"][LIST_FILE_TYPE];
			filetype_column["column"] = "file_type";
			filetype_column["type"] = "number";

			LLSD& assettype_column = element["columns"][LIST_ASSET_TYPE];
			assettype_column["column"] = "asset_type";
			assettype_column["type"] = "number";

			LLSD& invtype_column = element["columns"][LIST_INVENTORY_TYPE];
			invtype_column["column"] = "icon_inventory_type";
			invtype_column["type"] = "icon";
			invtype_column["value"] = "inv_folder_trash.tga";


			if (((extensionL == "jpeg")||(extensionL == "jpg")||(extensionL == "tga")
				||(extensionL == "png")||(extensionL == "bmp"))&&((mFilterType == "None")||(mFilterType == "Texture")))
			{
				invtype_column["value"] = "inv_item_texture.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = FILE_TEXTURE;
				assettype_column["value"] = LIST_TYPE_FILE;

			}
			else if ((extensionL == "wav")&&((mFilterType == "None")||(mFilterType == "Sound")))
			{	
				invtype_column["value"] = "inv_item_sound.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = FILE_SOUND;
				assettype_column["value"] = LIST_TYPE_FILE;
			}
			else if (((extensionL == "bvh")||(extensionL == "anim"))&&((mFilterType == "None")||(mFilterType == "Animation")))
			{	
				invtype_column["value"] = "inv_item_animation.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = FILE_ANIMATION;
				assettype_column["value"] = LIST_TYPE_FILE;
			}
			else if ((extension == filename.substr(0, filename.length() - 1))&&(filename != "."))
			{
				std::string test_path = mPathName + gDirUtilp->getDirDelimiter() + filename + gDirUtilp->getDirDelimiter();
				S32 file_count = gDirUtilp->countFilesInDir(test_path, "*.*");
				if(file_count)
				{
					invtype_column["value"] = "inv_folder_plain_closed.tga";
					filename_column["value"] = filename;
					filetype_column["value"] = FOLDER;
					assettype_column["value"] = LIST_TYPE_FOLDER;
				}
			}
			else if (filename == "..")
			{
				invtype_column["value"] = "inv_folder_plain_open.tga";
				filename_column["value"] = filename;
				filetype_column["value"] = FOLDER;
				assettype_column["value"] = LIST_TYPE_PARENT;
			}
			if (invtype_column["value"].asString() != "inv_folder_trash.tga")
			{
				mFileList->addElement(element, ADD_BOTTOM);
				if (assettype_column["value"].asInteger() == LIST_TYPE_FILE)
				{
					file_count++;
				}
			}
		}
	}
	
	std::string result;
	LLResMgr::getInstance()->getIntegerString(result, file_count);
	if (result == "")
		result = "0";
	childSetTextArg("result_label",  "[COUNT]", result);

	mFileList->sortItems();
	llinfos << "Total files loaded: " << result << "." << llendl;
}

// static
void ASFloaterUploadBrowser::show(void*)
{
	if (!sInstance)
	{
		sInstance = new ASFloaterUploadBrowser();
	}

	sInstance->open();	 /*Flawfinder: ignore*/
}