// <edit>
 
#include "llviewerprecompiledheaders.h"
#include "llfloaterexportregion.h"
#include "llfloaterexport.h"
#include "lluictrlfactory.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llfilepicker.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llimportobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"

LLFloaterExportRegion* LLFloaterExportRegion::sInstance = NULL;

LLFloaterExportRegion::LLFloaterExportRegion(const LLSD& unused)
{
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_export.xml");

	//populate the list of objects to export
	//int numOfObjects = gObjectList.getNumObjects();

}


LLFloaterExportRegion::~LLFloaterExportRegion()
{
	sInstance = NULL;
}

BOOL LLFloaterExportRegion::postBuild(void)
{
	updateNamesProgress();

	childSetAction("select_all_btn", onClickSelectAll, this);
	childSetEnabled("select_objects_btn", FALSE);
	childSetEnabled("select_wearables_btn", FALSE);

	childSetAction("save_as_btn", onClickSaveAs, this);
	childSetEnabled("make_copy_btn", FALSE);

	return TRUE;
}

//static
void LLFloaterExportRegion::onClickSelectAll(void* user_data)
{
	LLFloaterExport* floater = (LLFloaterExport*)user_data;
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("export_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
	std::vector<LLScrollListItem*>::iterator items_end = items.end();
	if(items.size())
	{
		bool new_value = !((*item_iter)->getColumn(LIST_CHECKED)->getValue());
		for( ; item_iter != items_end; ++item_iter)
		{
			LLScrollListItem* item = (*item_iter);
			item->getColumn(LIST_CHECKED)->setValue(new_value);
		}
	}
}

LLSD LLFloaterExportRegion::getLLSD()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
	std::vector<LLScrollListItem*> items = list->getAllData();
	LLSD sd;
	std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
	std::vector<LLScrollListItem*>::iterator items_end = items.end();
	for( ; item_iter != items_end; ++item_iter)
	{
		LLScrollListItem* item = (*item_iter);
		if(item->getColumn(LIST_CHECKED)->getValue())
		{
			LLSD item_sd = mExportables[item->getUUID()];
			LLSD::map_iterator map_iter = item_sd.beginMap();
			LLSD::map_iterator map_end = item_sd.endMap();
			for( ; map_iter != map_end; ++map_iter)
			{
				std::string key((*map_iter).first);
				LLSD data = (*map_iter).second;
				// copy it...
				sd[key] = data;
			}
		}
	}
	return sd;
}

//static
void LLFloaterExportRegion::onClickSaveAs(void* user_data)
{
	LLFloaterExport* floater = (LLFloaterExport*)user_data;
	LLSD sd = floater->getLLSD();

	if(sd.size())
	{
		std::string default_filename = "untitled";

		// set correct names within llsd
		LLSD::map_iterator map_iter = sd.beginMap();
		LLSD::map_iterator map_end = sd.endMap();
		for( ; map_iter != map_end; ++map_iter)
		{
			std::istringstream keystr((*map_iter).first);
			U32 key;
			keystr >> key;
			LLSD item = (*map_iter).second;
			if(item["type"].asString() == "prim")
			{
				std::string name = floater->mPrimNameMap[key];
				item["name"] = name;
				// I don't understand C++ :(
				sd[(*map_iter).first] = item;
			}
		}

		// count the number of selected items
		LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("export_list");
		std::vector<LLScrollListItem*> items = list->getAllData();
		int item_count = 0;
		LLUUID avatarid = (*(items.begin()))->getColumn(LIST_AVATARID)->getValue().asUUID();
		bool all_same_avatarid = true;
		std::vector<LLScrollListItem*>::iterator item_iter = items.begin();
		std::vector<LLScrollListItem*>::iterator items_end = items.end();
		for( ; item_iter != items_end; ++item_iter)
		{
			LLScrollListItem* item = (*item_iter);
			if(item->getColumn(LIST_CHECKED)->getValue())
			{
				item_count++;
				if(item->getColumn(LIST_AVATARID)->getValue().asUUID() != avatarid)
				{
					all_same_avatarid = false;
				}
			}
		}

		if(item_count == 1)
		{
			// Exporting one item?  Use its name for the filename.
			// But make sure it's a root!
			LLSD target = (*(sd.beginMap())).second;
			if(target.has("parent"))
			{
				std::string parentid = target["parent"].asString();
				target = sd[parentid];
			}
			if(target.has("name"))
			{
				if(target["name"].asString().length() > 0)
				{
					default_filename = target["name"].asString();
				}
			}
		}
		else
		{
			// Multiple items?
			// If they're all part of the same avatar, use the avatar's name as filename.
			if(all_same_avatarid)
			{
				std::string fullname;
				if(gCacheName->getFullName(avatarid, fullname))
				{
					default_filename = fullname;
				}
			}
		}

		LLFilePicker& file_picker = LLFilePicker::instance();
		if(file_picker.getSaveFile( LLFilePicker::FFSAVE_XML, LLDir::getScrubbedFileName(default_filename + ".xml")))
		{
			std::string file_name = file_picker.getFirstFile();
			llofstream export_file(file_name);
			LLSDSerialize::toPrettyXML(sd, export_file);
			export_file.close();

			std::string msg = "Saved ";
			msg.append(file_name);
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
		}
	}
	else
	{
		std::string msg = "No exportable items selected";
		LLChat chat(msg);
		LLFloaterChat::addChat(chat);
		return;
	}
	
	floater->close();
}

void LLFloaterExportRegion::addToPrimList(LLViewerObject* object)
{
	mPrimList.push_back(object->getLocalID());
	LLViewerObject::child_list_t child_list = object->getChildren();
	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		LLViewerObject* child = *i;
		if(child->getPCode() < LL_PCODE_APP)
		{
			mPrimList.push_back(child->getLocalID());
		}
	}
}

void LLFloaterExportRegion::updateNamesProgress()
{
	childSetText("names_progress_text", llformat("Names retrieved: %d of %d", mPrimNameMap.size(), mPrimList.size()));
}

void LLFloaterExportRegion::receivePrimName(LLViewerObject* object, std::string name)
{
	LLUUID fullid = object->getID();
	U32 localid = object->getLocalID();
	if(std::find(mPrimList.begin(), mPrimList.end(), localid) != mPrimList.end())
	{
		mPrimNameMap[localid] = name;
		LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
		S32 item_index = list->getItemIndex(fullid);
		if(item_index != -1)
		{
			std::vector<LLScrollListItem*> items = list->getAllData();
			std::vector<LLScrollListItem*>::iterator iter = items.begin();
			std::vector<LLScrollListItem*>::iterator end = items.end();
			for( ; iter != end; ++iter)
			{
				if((*iter)->getUUID() == fullid)
				{
					(*iter)->getColumn(LIST_NAME)->setValue(name + " (worn on " + utf8str_tolower(object->getAttachmentPointName()) + ")");
					break;
				}
			}
		}
		updateNamesProgress();
	}
}

// static
void LLFloaterExportRegion::receiveObjectProperties(LLUUID fullid, std::string name, std::string desc)
{
	LLViewerObject* object = gObjectList.findObject(fullid);
	std::vector<LLFloaterExport*>::iterator iter = LLFloaterExport::instances.begin();
	std::vector<LLFloaterExport*>::iterator end = LLFloaterExport::instances.end();
	for( ; iter != end; ++iter)
	{
		(*iter)->receivePrimName(object, name);
	}
}

// </edit>
