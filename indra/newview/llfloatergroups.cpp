/** 
 * @file llfloatergroups.cpp
 * @brief LLPanelGroups class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

/*
 * Shown from Edit -> Groups...
 * Shows the agent's groups and allows the edit window to be invoked.
 * Also overloaded to allow picking of a single group for assigning 
 * objects and land to groups.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergroups.h"
#include "llfloatergroupinvite.h"

#include "message.h"
#include "roles_constants.h"

#include "hbfloatergrouptitles.h"
#include "llagent.h"
#include "llbutton.h"
#include "llfloatergroupinfo.h"
#include "llfloaterdirectory.h"
#include "llfocusmgr.h"
#include "llgroupactions.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llimview.h"

#include "hippolimits.h"

using namespace LLOldEvents;

// static
std::map<const LLUUID, LLFloaterGroupPicker*> LLFloaterGroupPicker::sInstances;

// helper functions
void init_group_list(LLScrollListCtrl* ctrl, const LLUUID& highlight_id, const std::string& none_text, U64 powers_mask = GP_ALL_POWERS);

///----------------------------------------------------------------------------
/// Class LLFloaterGroupPicker
///----------------------------------------------------------------------------

// static
LLFloaterGroupPicker* LLFloaterGroupPicker::findInstance(const LLSD& seed)
{
	instance_map_t::iterator found_it = sInstances.find(seed.asUUID());
	if (found_it != sInstances.end())
	{
		return found_it->second;
	}
	return NULL;
}

// static
LLFloaterGroupPicker* LLFloaterGroupPicker::createInstance(const LLSD &seed)
{
	LLFloaterGroupPicker* pickerp = new LLFloaterGroupPicker(seed);
	LLUICtrlFactory::getInstance()->buildFloater(pickerp, "floater_choose_group.xml");
	return pickerp;
}

LLFloaterGroupPicker::LLFloaterGroupPicker(const LLSD& seed) : 
	mSelectCallback(NULL),
	mCallbackUserdata(NULL),
	mPowersMask(GP_ALL_POWERS)
{
	mID = seed.asUUID();
	sInstances.insert(std::make_pair(mID, this));
}

LLFloaterGroupPicker::~LLFloaterGroupPicker()
{
	sInstances.erase(mID);
}

void LLFloaterGroupPicker::setSelectCallback(void (*callback)(LLUUID, void*), 
									void* userdata)
{
	mSelectCallback = callback;
	mCallbackUserdata = userdata;
}

void LLFloaterGroupPicker::setPowersMask(U64 powers_mask)
{
	mPowersMask = powers_mask;
	postBuild();
}


BOOL LLFloaterGroupPicker::postBuild()
{

	const std::string none_text = getString("none");
	LLScrollListCtrl* group_list = getChild<LLScrollListCtrl>("group list");
	init_group_list(group_list, gAgent.getGroupID(), none_text, mPowersMask);

	childSetAction("OK", onBtnOK, this);

	childSetAction("Cancel", onBtnCancel, this);

	setDefaultBtn("OK");

	group_list->setDoubleClickCallback(boost::bind(&LLFloaterGroupPicker::onBtnOK,this));

	childEnable("OK");

	return TRUE;
}

void LLFloaterGroupPicker::onBtnOK(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->ok();
}

void LLFloaterGroupPicker::onBtnCancel(void* userdata)
{
	LLFloaterGroupPicker* self = (LLFloaterGroupPicker*)userdata;
	if(self) self->close();
}


void LLFloaterGroupPicker::ok()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}
	if(mSelectCallback)
	{
		mSelectCallback(group_id, mCallbackUserdata);
	}

	close();
}

///----------------------------------------------------------------------------
/// Class LLPanelGroups
///----------------------------------------------------------------------------

//LLEventListener
//virtual
bool LLPanelGroups::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (event->desc() == "new group")
	{
		reset();
		return true;
	}
	return false;
}

// Default constructor
LLPanelGroups::LLPanelGroups() :
	LLPanel()
{
	gAgent.addListener(this, "new group");
}

LLPanelGroups::~LLPanelGroups()
{
	gAgent.removeListener(this);
}

// clear the group list, and get a fresh set of info.
void LLPanelGroups::reset()
{
	childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
	childSetTextArg("groupcount", "[MAX]", llformat("%d", gHippoLimits->getMaxAgentGroups()));
	
	const std::string none_text = getString("none");
	init_group_list(getChild<LLScrollListCtrl>("group list"), gAgent.getGroupID(), none_text);
	enableButtons();
}

BOOL LLPanelGroups::postBuild()
{
	childSetTextArg("groupcount", "[COUNT]", llformat("%d",gAgent.mGroups.count()));
	childSetTextArg("groupcount", "[MAX]", llformat("%d", gHippoLimits->getMaxAgentGroups()));

	const std::string none_text = getString("none");
	LLScrollListCtrl *group_list = getChild<LLScrollListCtrl>("group list");
	init_group_list(group_list, gAgent.getGroupID(), none_text);
	group_list->setCommitCallback(boost::bind(&LLPanelGroups::onGroupList,this));
	group_list->setSortChangedCallback(boost::bind(&LLPanelGroups::onGroupSortChanged,this)); //Force 'none' to always be first entry.
	group_list->setDoubleClickCallback(boost::bind(LLGroupActions::startIM, boost::bind(&LLScrollListCtrl::getCurrentID, group_list)));

	getChild<LLUICtrl>("Activate")->setCommitCallback(boost::bind(LLGroupActions::activate, boost::bind(&LLScrollListCtrl::getCurrentID, group_list)));

	getChild<LLUICtrl>("Info")->setCommitCallback(boost::bind(LLGroupActions::show, boost::bind(&LLScrollListCtrl::getCurrentID, group_list)));

	getChild<LLUICtrl>("IM")->setCommitCallback(boost::bind(LLGroupActions::startIM, boost::bind(&LLScrollListCtrl::getCurrentID, group_list)));

	getChild<LLUICtrl>("Leave")->setCommitCallback(boost::bind(LLGroupActions::leave, boost::bind(&LLScrollListCtrl::getCurrentID, group_list)));

	getChild<LLUICtrl>("Create")->setCommitCallback(boost::bind(LLGroupActions::createGroup));

	getChild<LLUICtrl>("Search...")->setCommitCallback(boost::bind(LLGroupActions::search));
	
	childSetAction("Invite...", onBtnInvite, this);

	getChild<LLUICtrl>("Titles...")->setCommitCallback(boost::bind(HBFloaterGroupTitles::toggle));

	setDefaultBtn("IM");

	reset();

	return TRUE;
}

void LLPanelGroups::enableButtons()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}

	if(group_id != gAgent.getGroupID())
	{
		childEnable("Activate");
	}
	else
	{
		childDisable("Activate");
	}
	if (group_id.notNull())
	{
		childEnable("Info");
		childEnable("IM");
		childEnable("Leave");
	}
	else
	{
		childDisable("Info");
		childDisable("IM");
		childDisable("Leave");
	}
	if(gAgent.mGroups.count() < gHippoLimits->getMaxAgentGroups())
	{
		childEnable("Create");
	}
	else
	{
		childDisable("Create");
	}
	if (group_id.notNull() && gAgent.hasPowerInGroup(group_id, GP_MEMBER_INVITE))
	{
		LLPanelGroups::childEnable("Invite...");
	}
	else
	{
		LLPanelGroups::childDisable("Invite...");
	}
}


void LLPanelGroups::onBtnInvite(void* userdata)
{
	LLPanelGroups* self = (LLPanelGroups*)userdata;
	if(self) self->invite();
}

void LLPanelGroups::invite()
{
	LLCtrlListInterface *group_list = childGetListInterface("group list");
	LLUUID group_id;

	//if (group_list && (group_id = group_list->getCurrentID()).notNull())
	
	if (group_list)
	{
		group_id = group_list->getCurrentID();
	}

		LLFloaterGroupInvite::showForGroup(group_id);
}

void LLPanelGroups::onGroupSortChanged()
{
	LLScrollListCtrl *group_list = getChild<LLScrollListCtrl>("group list");
	if(!group_list)
		return;

	group_list->moveToFront(group_list->getItemIndex(LLUUID::null));
}

void LLPanelGroups::onGroupList()
{
	enableButtons();

	LLScrollListCtrl *group_list = getChild<LLScrollListCtrl>("group list");
	if(!group_list)
		return;

	LLScrollListItem *item = group_list->getFirstSelected();
	if(!item)
		return;

	const LLUUID group_id =  item->getValue().asUUID();
	if(group_id.isNull())
		return;

	LLGroupData group_data;
	if(!gAgent.getGroupData(group_id,group_data))
		return;

	bool list_in_profile = item->getColumn(1)->getValue().asBoolean();
	bool receive_chat = item->getColumn(2)->getValue().asBoolean();
	bool recieve_notify = item->getColumn(3)->getValue().asBoolean();
	bool update_floaters = false;
	if(gIMMgr->getIgnoreGroup(group_id) == receive_chat)
	{
		gIMMgr->updateIgnoreGroup(group_id, !receive_chat);
		update_floaters = true;
	}
	if(	(bool)group_data.mListInProfile != list_in_profile ||
		(bool)group_data.mAcceptNotices != recieve_notify )
	{
		gAgent.setUserGroupFlags(group_id, recieve_notify, list_in_profile);
	}
	else if(update_floaters) //gAgent.setUserGroupFlags already calls update_group_floaters
		update_group_floaters(group_id);
}

LLSD create_group_element(const LLGroupData *group_datap, const LLUUID &active_group, const std::string& none_text, const U64 &powers_mask)
{
	if(group_datap && !((powers_mask == GP_ALL_POWERS) || ((group_datap->mPowers & powers_mask) != 0)))
		return LLSD();
	const LLUUID &id = group_datap ? group_datap->mID : LLUUID::null;
	const bool enabled = !!group_datap;
	
	std::string style = (group_datap && group_datap->mListInProfile) ? "BOLD" : "NORMAL";
	if(active_group == id)
	{
		style.append("|ITALIC");
	}
	LLSD element;
	element["id"] = id;
	LLSD& name_column = element["columns"][0];
	name_column["column"] = "name";
	name_column["value"] = group_datap ? group_datap->mName : none_text;
	name_column["font"] = "SANSSERIF";
	name_column["font-style"] = style;

	LLSD& show_column = element["columns"][1];
	show_column["column"] = "is_listed_group";
	show_column["type"] = "checkbox";
	show_column["enabled"] = enabled;
	show_column["value"] = enabled && group_datap->mListInProfile;

	LLSD& chat_column = element["columns"][2];
	chat_column["column"] = "is_chattable_group";
	chat_column["type"] = "checkbox";
	chat_column["enabled"] = enabled;
	chat_column["value"] = enabled && !gIMMgr->getIgnoreGroup(id);

	LLSD& notice_column = element["columns"][3];
	notice_column["column"] = "is_notice_group";
	notice_column["type"] = "checkbox";
	notice_column["enabled"] = enabled;
	notice_column["value"] = enabled && group_datap->mAcceptNotices;

	return element;
}

void init_group_list(LLScrollListCtrl* ctrl, const LLUUID& highlight_id, const std::string& none_text, U64 powers_mask)
{
	S32 count = gAgent.mGroups.count();
	LLUUID id;
	LLCtrlListInterface *group_list = ctrl->getListInterface();
	if (!group_list) return;

	const LLUUID selected_id	= group_list->getSelectedValue();
	const S32	selected_idx	= group_list->getFirstSelectedIndex();
	const S32	scroll_pos		= ctrl->getScrollPos();

	group_list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	for(S32 i = 0; i < count; ++i)
	{
		LLSD element = create_group_element(&gAgent.mGroups.get(i), highlight_id, none_text, powers_mask);
		if(element.size())
			group_list->addElement(element, ADD_SORTED);
	}

	// add "none" to list at top
	group_list->addElement(create_group_element(NULL, highlight_id, none_text, powers_mask), ADD_TOP);

	if(selected_id.notNull())
		group_list->selectByValue(selected_id);
	else
		group_list->selectByValue(highlight_id);			//highlight is actually active group
	if(selected_idx!=group_list->getFirstSelectedIndex())	//if index changed then our stored pos is pointless.
		ctrl->scrollToShowSelected();
	else
		ctrl->setScrollPos(scroll_pos);
}

