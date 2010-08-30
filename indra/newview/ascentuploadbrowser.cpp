/** 
 * @file ascentuploadbrowser.h
 * @Author Duncan Garrett
 * Meant as a replacement to using Windows' file browser for uploads.
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
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h" //XML Parsing - Probably not needed

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

ASFloaterUploadBrowser* ASFloaterUploadBrowser::sInstance = NULL;


///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------

// Default constructor
ASFloaterUploadBrowser::ASFloaterUploadBrowser() 
:	LLFloater(std::string("floater_upload_browser"), std::string("FloaterUploadRect"), LLStringUtil::null)
{
	mPathName = "C:\\Users\\Duncan\\Documents"+gDirUtilp->getDirDelimiter(); //(gDirUtilp->getSkinBaseDir()+gDirUtilp->getDirDelimiter());
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_upload_browser.xml");
	mFileList = getChild<LLScrollListCtrl>("file_list");
	childSetCommitCallback("file_list", onClickFile, this);
	childSetDoubleClickCallback("file_list", onDoubleClick);
	refresh();
	mFileList->sortByColumn(std::string("file_name"), TRUE);
	mFileList->sortByColumn(std::string("file_type"), TRUE);
}

// Destroys the object
ASFloaterUploadBrowser::~ASFloaterUploadBrowser()
{
	sInstance = NULL;
}

void ASFloaterUploadBrowser::onClickFile(LLUICtrl* ctrl, void* user_data)
{
	ASFloaterUploadBrowser* panelp = (ASFloaterUploadBrowser*)user_data;
	panelp->refreshUploadOptions();
}

void ASFloaterUploadBrowser::refreshUploadOptions()
{
	if (mFileList->getFirstSelected()->getColumn(LIST_FILE_TYPE)->getValue().asInteger() == LIST_TYPE_FILE)
	{
		if (mFileList->getAllSelected().size() > 1)
		{
			llinfos << "Selected multiple files." << llendl;
			childSetValue("asset_name", "(Multiple)");
		}
		else
		{
			llinfos << "Selected a file." << llendl;
			std::string name = mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString();
			childSetValue("asset_name", name);
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
	if (mFileList->getFirstSelected()->getColumn(LIST_FILE_TYPE)->getValue().asInteger() == LIST_TYPE_PARENT)
	{
		llinfos << "Double-clicked Parent." << llendl;
	}
	else if (mFileList->getFirstSelected()->getColumn(LIST_FILE_TYPE)->getValue().asInteger() == LIST_TYPE_FOLDER)
	{
		/*std::string newPath = (mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString()+gDirUtilp->getDirDelimiter());
		newPath = (mPathName+newPath);
		mPathName = newPath.c_str();*/
		llinfos << "Double-clicked Folder. Directory should change to " << mPathName << mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString() << llendl;
		//mPathName.append(mFileList->getFirstSelected()->getColumn(LIST_FILE_NAME)->getValue().asString().c_str() + gDirUtilp->getDirDelimiter());
		refresh();
	}
}

void ASFloaterUploadBrowser::refresh()
{
	llinfos << "Emptying existing file list." << mPathName << llendl;
	mFileList->deleteAllItems();
	llinfos << "Getting file listing at " << mPathName << llendl;
	bool found = true;
	while(found) 
	{
		std::string filename;
		found = gDirUtilp->getNextFileInDir(mPathName, "*.*", filename, false);
		if(found)
		{

			S32 periodIndex = filename.find_last_of(".");
			std::string extension = filename.substr(periodIndex + 1, filename.length() - 1);
			llinfos << extension << llendl;
			LLSD element;
			element["path"] = mPathName + filename;

			LLSD& filename_column = element["columns"][LIST_FILE_NAME];
			filename_column["column"] = "file_name";
			filename_column["font"] = "SANSSERIF";
			filename_column["font-style"] = "NORMAL";	
			
			LLSD& filetype_column = element["columns"][LIST_FILE_TYPE];
			filetype_column["column"] = "file_type";
			filetype_column["type"] = "number";

			LLSD& invtype_column = element["columns"][LIST_INVENTORY_TYPE];
			invtype_column["column"] = "icon_inventory_type";
			invtype_column["type"] = "icon";
			invtype_column["value"] = "inv_folder_trash.tga";

			if ((extension == "jpeg")||(extension == "jpg")||(extension == "tga")
				||(extension == "png")||(extension == "bmp"))
			{
				invtype_column["value"] = "inv_item_texture.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = LIST_TYPE_FILE;
			}
			else if (extension == "wav")
			{	
				invtype_column["value"] = "inv_item_sound.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = LIST_TYPE_FILE;
			}
			else if ((extension == "bvh")||(extension == "anim"))
			{	
				invtype_column["value"] = "inv_item_animation.tga";
				filename_column["value"] = filename.substr(0, periodIndex);
				filetype_column["value"] = LIST_TYPE_FILE;
			}
			else if ((extension == filename.substr(0, filename.length() - 1))&&(filename != "."))
			{
				invtype_column["value"] = "inv_folder_plain_closed.tga";
				filename_column["value"] = filename;
				filetype_column["value"] = LIST_TYPE_FOLDER;
			}
			else if (filename == "..")
			{
				invtype_column["value"] = "inv_folder_plain_open.tga";
				filename_column["value"] = filename;
				filetype_column["value"] = LIST_TYPE_PARENT;
			}
			if (invtype_column["value"].asString() != "inv_folder_trash.tga")
			{
				mFileList->addElement(element, ADD_BOTTOM);
			}
		}
	}
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