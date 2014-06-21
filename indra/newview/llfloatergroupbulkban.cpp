/**
* @file llfloatergroupbulkban.cpp
* @brief Floater to ban Residents from a group.
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

#include "llfloatergroupbulkban.h"
#include "llpanelgroupbulkban.h"
#include "lltrans.h"
#include "lldraghandle.h"

const LLRect FGB_RECT(0, 320, 210, 0);

class LLFloaterGroupBulkBan::impl
{
public:
	impl(const LLUUID& group_id) : mGroupID(group_id), mBulkBanPanelp(NULL) {}
	~impl() {}

	static void closeFloater(void* data);

public:
	LLUUID mGroupID;
	LLPanelGroupBulkBan* mBulkBanPanelp;

	static std::map<LLUUID, LLFloaterGroupBulkBan*> sInstances;
};

//
// Globals
//
std::map<LLUUID, LLFloaterGroupBulkBan*> LLFloaterGroupBulkBan::impl::sInstances;

void LLFloaterGroupBulkBan::impl::closeFloater(void* data)
{
	LLFloaterGroupBulkBan* floaterp = (LLFloaterGroupBulkBan*)data;
	if(floaterp)
		floaterp->close();
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------
LLFloaterGroupBulkBan::LLFloaterGroupBulkBan(const std::string& name,
										   const LLRect &rect,
										   const std::string& title,
										   const LLUUID& group_id)
:	LLFloater(name, rect, title)
{
	S32 floater_header_size = LLFLOATER_HEADER_SIZE;
	LLRect contents(getRect());
	contents.mTop -= floater_header_size;

	mImpl = new impl(group_id);
	mImpl->mBulkBanPanelp = new LLPanelGroupBulkBan(group_id);

	setTitle(mImpl->mBulkBanPanelp->getString("GroupBulkBan"));
	mImpl->mBulkBanPanelp->setCloseCallback(impl::closeFloater, this);
	mImpl->mBulkBanPanelp->setRect(contents);

	addChild(mImpl->mBulkBanPanelp);
}

LLFloaterGroupBulkBan::~LLFloaterGroupBulkBan()
{
	if(mImpl->mGroupID.notNull())
	{
		impl::sInstances.erase(mImpl->mGroupID);
	}

	delete mImpl->mBulkBanPanelp;
	delete mImpl;
}

void LLFloaterGroupBulkBan::showForGroup(const LLUUID& group_id, uuid_vec_t* agent_ids)
{
	// Make sure group_id isn't null
	if (group_id.isNull())
	{
		llwarns << "LLFloaterGroupInvite::showForGroup with null group_id!" << llendl;
		return;
	}

	// If we don't have a floater for this group, create one.
	LLFloaterGroupBulkBan* fgb = get_if_there(impl::sInstances,
		group_id,
		(LLFloaterGroupBulkBan*)NULL);
	if (!fgb)
	{
		fgb = new LLFloaterGroupBulkBan("groupban",
										FGB_RECT,
										"Group Ban",
										group_id);
		fgb->getDragHandle()->setTitle(fgb->mImpl->mBulkBanPanelp->getString("GroupBulkBan"));

		impl::sInstances[group_id] = fgb;

		fgb->mImpl->mBulkBanPanelp->clear();
	}

	if (agent_ids != NULL)
	{
		fgb->mImpl->mBulkBanPanelp->addUsers(*agent_ids);
	}

	fgb->center();
	fgb->open();
	fgb->mImpl->mBulkBanPanelp->update();
}
