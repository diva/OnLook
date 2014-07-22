/**
* @file llpanelgroupbulk.cpp
* @brief Implementation of llpanelgroupbulk
* @author Baker@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "llpanelgroupbulk.h"
#include "llpanelgroupbulkimpl.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistitem.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"


//////////////////////////////////////////////////////////////////////////
// Implementation of llpanelgroupbulkimpl.h functions
//////////////////////////////////////////////////////////////////////////
LLPanelGroupBulkImpl::LLPanelGroupBulkImpl(const LLUUID& group_id) :
	mGroupID(group_id),
	mBulkAgentList(NULL),
	mOKButton(NULL),
	mRemoveButton(NULL),
	mGroupName(NULL),
	mLoadingText(),
	mTooManySelected(),
	mCloseCallback(NULL),
	mCloseCallbackUserData(NULL),
	mRoleNames(NULL),
	mOwnerWarning(),
	mAlreadyInGroup(),
	mConfirmedOwnerInvite(false),
	mListFullNotificationSent(false)
{}

LLPanelGroupBulkImpl::~LLPanelGroupBulkImpl()
{
	for (avatar_name_cache_connection_map_t::iterator it = mAvatarNameCacheConnections.begin(); it != mAvatarNameCacheConnections.end(); ++it)
	{
		if (it->second.connected())
		{
			it->second.disconnect();
		}
	}
	mAvatarNameCacheConnections.clear();
}

void LLPanelGroupBulkImpl::callbackClickAdd(void* userdata)
{
	LLPanelGroupBulk* panelp = (LLPanelGroupBulk*)userdata;

	if(panelp)
	{
		//Right now this is hard coded with some knowledge that it is part
		//of a floater since the avatar picker needs to be added as a dependent
		//floater to the parent floater.
		//Soon the avatar picker will be embedded into this panel
		//instead of being it's own separate floater.  But that is next week.
		//This will do for now. -jwolk May 10, 2006
		/* Singu Note: We're different, we don't do this..
		LLView* button = panelp->findChild<LLButton>("add_button");
		*/
		LLFloater* root_floater = gFloaterView->getParentFloater(panelp);
		LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(
		//	boost::bind(callbackAddUsers, _1, panelp->mImplementation), TRUE, FALSE, FALSE, root_floater->getName(), button);
			boost::bind(&LLPanelGroupBulkImpl::callbackAddUsers, panelp->mImplementation, _1), TRUE);
		if(picker)
		{
			root_floater->addDependentFloater(picker);
		}
	}
}

void LLPanelGroupBulkImpl::callbackClickRemove(void* userdata)
{
	LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata;
	if (selfp) 
		selfp->handleRemove();
}

void LLPanelGroupBulkImpl::callbackClickCancel(void* userdata)
{
	LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata;
	if(selfp) 
		(*(selfp->mCloseCallback))(selfp->mCloseCallbackUserData);
}

void LLPanelGroupBulkImpl::callbackSelect(LLUICtrl* ctrl, void* userdata)
{
	LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata;
	if (selfp) 
		selfp->handleSelection();
}

void LLPanelGroupBulkImpl::callbackAddUsers(const uuid_vec_t& agent_ids)
{
	std::vector<std::string> names;
	for (S32 i = 0; i < (S32)agent_ids.size(); i++)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(agent_ids[i], &av_name))
		{
			onAvatarNameCache(agent_ids[i], av_name);
		}
		else 
		{
			if (mAvatarNameCacheConnections[agent_ids[i]].connected())
			{
				mAvatarNameCacheConnections[agent_ids[i]].disconnect();
			}
			// *TODO : Add a callback per avatar name being fetched.
			mAvatarNameCacheConnections[agent_ids[i]] = LLAvatarNameCache::get(agent_ids[i],boost::bind(&LLPanelGroupBulkImpl::onAvatarNameCache, this, _1, _2));
		}
	}
}

void LLPanelGroupBulkImpl::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	if (mAvatarNameCacheConnections[agent_id].connected())
	{
		mAvatarNameCacheConnections[agent_id].disconnect();
	}
	std::vector<std::string> names;
	uuid_vec_t agent_ids;
	agent_ids.push_back(agent_id);
	std::string name;
	LLAvatarNameCache::getPNSName(av_name, name);
	names.push_back(name);

	addUsers(names, agent_ids);
}

void LLPanelGroupBulkImpl::handleRemove()
{
	std::vector<LLScrollListItem*> selection = mBulkAgentList->getAllSelected();
	if (selection.empty()) 
		return;

	std::vector<LLScrollListItem*>::iterator iter;
	for(iter = selection.begin(); iter != selection.end(); ++iter)
	{
		mInviteeIDs.erase((*iter)->getUUID());
	}

	mBulkAgentList->deleteSelectedItems();
	mRemoveButton->setEnabled(FALSE);

	if(	mOKButton && mOKButton->getEnabled() &&
		mBulkAgentList->isEmpty())
	{
		mOKButton->setEnabled(FALSE);
	}
}

void LLPanelGroupBulkImpl::handleSelection()
{
	std::vector<LLScrollListItem*> selection = mBulkAgentList->getAllSelected();
	if (selection.empty())
		mRemoveButton->setEnabled(FALSE);
	else
		mRemoveButton->setEnabled(TRUE);
}

void LLPanelGroupBulkImpl::addUsers(const std::vector<std::string>& names, const uuid_vec_t& agent_ids)
{
	std::string name;
	LLUUID id;

	if(mListFullNotificationSent)
	{	
		return;
	}

	if(	!mListFullNotificationSent &&
		(names.size() + mInviteeIDs.size() > MAX_GROUP_INVITES))
	{
		mListFullNotificationSent = true;

		// Fail! Show a warning and don't add any names.
		LLSD msg;
		msg["MESSAGE"] = mTooManySelected;
		LLNotificationsUtil::add("GenericAlert", msg);
		return;
	}

	for (S32 i = 0; i < (S32)names.size(); ++i)
	{
		name = names[i];
		id = agent_ids[i];

		if(mInviteeIDs.find(id) != mInviteeIDs.end())
		{
			continue;
		}

		//add the name to the names list
		LLSD row;
		row["id"] = id;
		row["columns"][0]["value"] = name;

		mBulkAgentList->addElement(row);
		mInviteeIDs.insert(id);

		// We've successfully added someone to the list.
		if(mOKButton && !mOKButton->getEnabled())
			mOKButton->setEnabled(TRUE);
	}
}

void LLPanelGroupBulkImpl::setGroupName(std::string name)
{
	if(mGroupName)
		mGroupName->setText(name);
}


LLPanelGroupBulk::LLPanelGroupBulk(const LLUUID& group_id) : 
	LLPanel(),
	mImplementation(new LLPanelGroupBulkImpl(group_id)),
	mPendingGroupPropertiesUpdate(false),
	mPendingRoleDataUpdate(false),
	mPendingMemberDataUpdate(false)
{}

LLPanelGroupBulk::~LLPanelGroupBulk()
{
	delete mImplementation;
}

void LLPanelGroupBulk::clear()
{
	mImplementation->mInviteeIDs.clear();

	if(mImplementation->mBulkAgentList)
		mImplementation->mBulkAgentList->deleteAllItems();
	
	if(mImplementation->mOKButton)
		mImplementation->mOKButton->setEnabled(FALSE);
}

void LLPanelGroupBulk::update()
{
	updateGroupName();
	updateGroupData();
}

void LLPanelGroupBulk::draw()
{
	LLPanel::draw();
	update();
}

void LLPanelGroupBulk::updateGroupName()
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);

	if(	gdatap &&
		gdatap->isGroupPropertiesDataComplete())
	{
		// Only do work if the current group name differs
		if(mImplementation->mGroupName->getText().compare(gdatap->mName) != 0)
			mImplementation->setGroupName(gdatap->mName);
	}
	else
	{
		mImplementation->setGroupName(mImplementation->mLoadingText);
	}
}

void LLPanelGroupBulk::updateGroupData()
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);
	if(gdatap && gdatap->isGroupPropertiesDataComplete())
	{
		mPendingGroupPropertiesUpdate = false;
	}
	else
	{
		if(!mPendingGroupPropertiesUpdate)
		{
			mPendingGroupPropertiesUpdate = true;
			LLGroupMgr::getInstance()->sendGroupPropertiesRequest(mImplementation->mGroupID);
		}
	}

	if(gdatap && gdatap->isRoleDataComplete())
	{
		mPendingRoleDataUpdate = false;
	}
	else
	{
		if(!mPendingRoleDataUpdate)
		{
			mPendingRoleDataUpdate = true;
			LLGroupMgr::getInstance()->sendGroupRoleDataRequest(mImplementation->mGroupID);
		}
	}

	if(gdatap && gdatap->isMemberDataComplete())
	{
		mPendingMemberDataUpdate = false;
	}
	else
	{
		if(!mPendingMemberDataUpdate)
		{
			mPendingMemberDataUpdate = true;
			LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mImplementation->mGroupID);
		}
	}
}

void LLPanelGroupBulk::addUserCallback(const LLUUID& id, const LLAvatarName& av_name)
{
	std::vector<std::string> names;
	uuid_vec_t agent_ids;
	agent_ids.push_back(id);
	std::string name;
	LLAvatarNameCache::getPNSName(av_name, name);
	names.push_back(name);

	mImplementation->addUsers(names, agent_ids);
}

void LLPanelGroupBulk::setCloseCallback(void (*close_callback)(void*), void* data)
{
	mImplementation->mCloseCallback         = close_callback;
	mImplementation->mCloseCallbackUserData = data;
}

void LLPanelGroupBulk::addUsers(uuid_vec_t& agent_ids)
{
	std::vector<std::string> names;
	for (S32 i = 0; i < (S32)agent_ids.size(); i++)
	{
		std::string fullname;
		LLUUID agent_id = agent_ids[i];
		LLViewerObject* dest = gObjectList.findObject(agent_id);
		if(dest && dest->isAvatar())
		{
			LLNameValue* nvfirst = dest->getNVPair("FirstName");
			LLNameValue* nvlast = dest->getNVPair("LastName");
			if(nvfirst && nvlast)
			{
				fullname = LLCacheName::buildFullName(
					nvfirst->getString(), nvlast->getString());

			}
			if (!fullname.empty())
			{
				names.push_back(fullname);
			} 
			else 
			{
				llwarns << "llPanelGroupBulk: Selected avatar has no name: " << dest->getID() << llendl;
				names.push_back("(Unknown)");
			}
		}
		else
		{
			//looks like user try to invite offline friend
			//for offline avatar_id gObjectList.findObject() will return null
			//so we need to do this additional search in avatar tracker, see EXT-4732
			//if (LLAvatarTracker::instance().isBuddy(agent_id)) // Singu Note: We may be using this from another avatar list like group profile, disregard friendship status.
			{
				LLAvatarName av_name;
				if (!LLAvatarNameCache::get(agent_id, &av_name))
				{
					// actually it should happen, just in case
					LLAvatarNameCache::get(LLUUID(agent_id), boost::bind(&LLPanelGroupBulk::addUserCallback, this, _1, _2));
					// for this special case!
					//when there is no cached name we should remove resident from agent_ids list to avoid breaking of sequence
					// removed id will be added in callback
					agent_ids.erase(agent_ids.begin() + i);
				}
				else
				{
					std::string name;
					LLAvatarNameCache::getPNSName(av_name, name);
					names.push_back(name);
				}
			}
		}
	}
	mImplementation->mListFullNotificationSent = false;
	mImplementation->addUsers(names, agent_ids);
}

