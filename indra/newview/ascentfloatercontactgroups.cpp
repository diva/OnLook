/** 
 * @file ascentfloatercontactgroups.cpp
 * @Author Charley Levenque
 * Allows the user to assign friends to contact groups for advanced sorting.
 *
 * Created Sept 6th 2010
 * 
 * ALL SOURCE CODE IS PROVIDED "AS IS." THE CREATOR MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.

*/

#include "llviewerprecompiledheaders.h"

#include "ascentfloatercontactgroups.h"

//UI Elements
#include "llbutton.h" //Buttons
#include "llcombobox.h" //Combo dropdowns
#include "llscrolllistctrl.h" //List box for filenames
#include "lluictrlfactory.h" //Loads the XUI

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h"
#include "lldarray.h"
#include "llfile.h"
#include "llchat.h"
#include "llfloaterchat.h"

ASFloaterContactGroups* ASFloaterContactGroups::sInstance = NULL;
LLDynamicArray<LLUUID> ASFloaterContactGroups::mSelectedUUIDs;
LLSD ASFloaterContactGroups::mContactGroupData;

ASFloaterContactGroups::ASFloaterContactGroups()
:	LLFloater(std::string("floater_contact_groups"), std::string("FloaterContactRect"), LLStringUtil::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_contact_groups.xml");
}

// static
void ASFloaterContactGroups::show(LLDynamicArray<LLUUID> ids)
{
    if (!sInstance)
	sInstance = new ASFloaterContactGroups();

	mSelectedUUIDs = ids;

    sInstance->open();
	sInstance->populateGroupList();
	sInstance->populateFriendList();

	sInstance->childSetAction("Cancel", onBtnClose, sInstance);
	sInstance->childSetAction("Save", onBtnSave, sInstance);
	sInstance->childSetAction("Create", onBtnCreate, sInstance);
	sInstance->childSetAction("Delete", onBtnDelete, sInstance);
}

void ASFloaterContactGroups::onBtnDelete(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;

	if(self)
	{
		LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("group_scroll_list");
		if(scroller != NULL) 
		{
			self->deleteContactGroup(scroller->getValue().asString());
			self->populateGroupList();
		}
	}
}

void ASFloaterContactGroups::onBtnSave(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;

	if(self)
	{
		if (self->mSelectedUUIDs.count() > 0)
		{
			LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("group_scroll_list");
			if(scroller != NULL) 
			{
				for (S32 i = self->mSelectedUUIDs.count(); i > 0; --i)
				{
					self->addContactMember(scroller->getValue().asString(), self->mSelectedUUIDs.get(i));
				}
			}
		}
	}
}

void ASFloaterContactGroups::onBtnClose(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;
	if(self) self->close();
}

void ASFloaterContactGroups::onBtnCreate(void* userdata)
{
	ASFloaterContactGroups* self = (ASFloaterContactGroups*)userdata;
	if(self)
	{
		LLLineEditor* editor = self->getChild<LLLineEditor>("add_group_lineedit");
		if (editor)
		{
			LLScrollListCtrl* scroller = self->getChild<LLScrollListCtrl>("friend_scroll_list");
			if(scroller != NULL) 
			{
				self->createContactGroup(editor->getValue().asString());
				self->populateGroupList();
			}
		}
		else
		{
			LLChat msg("Null editor");
			LLFloaterChat::addChat(msg);
		}
	}
	else
	{
		LLChat msg("Null floater");
		LLFloaterChat::addChat(msg);
	}
}

ASFloaterContactGroups::~ASFloaterContactGroups()
{
    sInstance=NULL;
}

void ASFloaterContactGroups::populateFriendList()
{
	LLScrollListCtrl* scroller = getChild<LLScrollListCtrl>("friend_scroll_list");
	if(scroller != NULL) 
	{
		
	}
}

void ASFloaterContactGroups::addContactMember(std::string contact_grp, LLUUID to_add)
{
	ASFloaterContactGroups::mContactGroupData[contact_grp].append(to_add.asString());
	gSavedPerAccountSettings.setLLSD("AscentContactGroups", ASFloaterContactGroups::mContactGroupData);
}

void ASFloaterContactGroups::deleteContactGroup(std::string contact_grp)
{
	
}

void ASFloaterContactGroups::createContactGroup(std::string contact_grp)
{
	ASFloaterContactGroups::mContactGroupData["ASC_MASTER_GROUP_LIST"].append(contact_grp);
	gSavedPerAccountSettings.setLLSD("AscentContactGroups", ASFloaterContactGroups::mContactGroupData);
	populateGroupList();
}

void ASFloaterContactGroups::populateGroupList()
{
	ASFloaterContactGroups::mContactGroupData = gSavedPerAccountSettings.getLLSD("AscentContactGroups");
	LLScrollListCtrl* scroller = getChild<LLScrollListCtrl>("group_scroll_list");
	if(scroller != NULL) 
	{
		scroller->deleteAllItems();

		S32 count = ASFloaterContactGroups::mContactGroupData["ASC_MASTER_GROUP_LIST"].size();
		S32 index;
		for (index = 0; index < count; index++)
		{
			scroller->addSimpleElement(ASFloaterContactGroups::mContactGroupData["ASC_MASTER_GROUP_LIST"][index].asString(), ADD_BOTTOM);
		}
	} 
	else
	{
		LLChat msg("Null Scroller");
		LLFloaterChat::addChat(msg);
	}
}