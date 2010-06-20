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
}


LLFloaterExportRegion::~LLFloaterExportRegion()
{
	sInstance = NULL;
}

BOOL LLFloaterExportRegion::postBuild(void)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
	
	
	LLViewerRegion* objregion = NULL;
	LLViewerRegion* agentregion = gAgent.getRegion();

	if(!agentregion) return TRUE;

	for (LLDynamicArrayPtr<LLPointer<LLViewerObject> >::iterator iter = gObjectList.getObjectMap().begin();
		 iter != gObjectList.getObjectMap().end(); iter++)
	{
		LLViewerObject* objectp = (*iter);
		if(!objectp || objectp->isDead()) continue;

		objregion = objectp->getRegion();

		//if(!objregion || objregion->getHandle() != agentregion->getHandle()) continue;

		if(objectp->isAvatar() || objectp->isAttachment()) continue; //we dont want avatars right now...

		if(objectp->isRoot())
		{
			std::string objectp_id = llformat("%d", objectp->getLocalID());

			if(list->getItemIndex(objectp->getID()) == -1)
			{
				bool is_attachment = false;
				bool is_root = true;
				LLViewerObject* parentp = objectp->getSubParent();
				if(parentp)
				{
					if(!parentp->isAvatar())
					{
						// parent is a prim I guess
						is_root = false;
					}
					else
					{
						// parent is an avatar
						is_attachment = true;
						//if(!avatars[parentp]) avatars[parentp] = true;
					}
				}

				bool is_prim = true;
				if(objectp->getPCode() >= LL_PCODE_APP)
				{
					is_prim = false;
				}

				//bool is_avatar = objectp->isAvatar();

			
				if(is_root && is_prim)
				{
					LLSD element;
					element["id"] = objectp->getID();

					LLSD& check_column = element["columns"][LIST_CHECKED];
					check_column["column"] = "checked";
					check_column["type"] = "checkbox";
					check_column["value"] = true;

					LLSD& type_column = element["columns"][LIST_TYPE];
					type_column["column"] = "type";
					type_column["type"] = "icon";
					type_column["value"] = "inv_item_object.tga";

					LLSD& name_column = element["columns"][LIST_NAME];
					name_column["column"] = "name";
					/*if(is_attachment)
						name_column["value"] = nodep->mName + " (worn on " + utf8str_tolower(objectp->getAttachmentPointName()) + ")";
					else*/
						name_column["value"] = "Object";

					LLSD& avatarid_column = element["columns"][LIST_AVATARID];
					avatarid_column["column"] = "avatarid";
					if(is_attachment)
						avatarid_column["value"] = parentp->getID();
					else
						avatarid_column["value"] = LLUUID::null;

					LLExportable* exportable = new LLExportable(objectp, "Object", mPrimNameMap);
					mExportables[objectp->getID()] = exportable->asLLSD();

					list->addElement(element, ADD_BOTTOM);

					addToPrimList(objectp);
				}//Do we really want avatars in the region exporter?
				/*
				else if(is_avatar)
				{
					if(!avatars[objectp])
					{
						avatars[objectp] = true;
					}
				}
				*/
			}
		}
		U32 localid = objectp->getLocalID();
		std::string name = "Object";
		mPrimNameMap[localid] = name;
		//Let's get names...
		LLViewerObject::child_list_t child_list = objectp->getChildren();
		for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
		{
			LLViewerObject* childp = *i;

			LLViewerObject::child_list_t select_list = childp->getChildren();
			LLViewerObject::child_list_t::iterator select_iter;
			int block_counter;

			gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, childp->getLocalID());
			block_counter = 0;
			for (select_iter = select_list.begin(); select_iter != select_list.end(); ++select_iter)
			{
				block_counter++;
				if(block_counter >= 254)
				{
					// start a new message
					gMessageSystem->sendReliable(childp->getRegion()->getHost());
					gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
				}
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
				gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, (*select_iter)->getLocalID());
			}
			gMessageSystem->sendReliable(childp->getRegion()->getHost());

			gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, childp->getLocalID());
			block_counter = 0;
			for (select_iter = select_list.begin(); select_iter != select_list.end(); ++select_iter)
			{
				block_counter++;
				if(block_counter >= 254)
				{
					// start a new message
					gMessageSystem->sendReliable(childp->getRegion()->getHost());
					gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
				}
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
				gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, (*select_iter)->getLocalID());
			}
			gMessageSystem->sendReliable(childp->getRegion()->getHost());
		}
	}
	//Do we really want avatars in the region exporter?
	/*std::map<LLViewerObject*, bool>::iterator avatar_iter = avatars.begin();
	std::map<LLViewerObject*, bool>::iterator avatars_end = avatars.end();
	for( ; avatar_iter != avatars_end; avatar_iter++)
	{
		LLViewerObject* avatar = (*avatar_iter).first;
		addAvatarStuff((LLVOAvatar*)avatar);
	}*/

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
