/** 
 * @file llremoteparcelrequest.cpp
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "message.h"

#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
//#include "llurlentry.h"
#include "llviewerregion.h"
#include "llview.h"

#include "llagent.h"
#include "llremoteparcelrequest.h"


LLRemoteParcelRequestResponder::LLRemoteParcelRequestResponder(LLHandle<LLRemoteParcelInfoObserver> observer_handle)
	 : mObserverHandle(observer_handle)
{}

//If we get back a normal response, handle it here
//virtual
void LLRemoteParcelRequestResponder::httpSuccess(void)
{
	LLUUID parcel_id = mContent["parcel_id"];

	// Panel inspecting the information may be closed and destroyed
	// before this response is received.
	LLRemoteParcelInfoObserver* observer = mObserverHandle.get();
	if (observer)
	{
		observer->setParcelID(parcel_id);
	}
}

//If we get back an error (not found, etc...), handle it here
//virtual
void LLRemoteParcelRequestResponder::httpFailure(void)
{
	llinfos << "httpFailure: " << dumpResponse() << llendl;

	// Panel inspecting the information may be closed and destroyed
	// before this response is received.
	LLRemoteParcelInfoObserver* observer = mObserverHandle.get();
	if (observer)
	{
		observer->setErrorStatus(mStatus, mReason);
	}
}

void LLRemoteParcelInfoProcessor::addObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
	observer_multimap_t::iterator it;
	observer_multimap_t::iterator start = mObservers.lower_bound(parcel_id);
	observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

	// Check if the observer is already in observers list for this UUID
	for(it = start; it != end; ++it)
	{
		if (it->second.get() == observer)
		{
			return;
		}
	}

	mObservers.insert(std::make_pair(parcel_id, observer->getObserverHandle()));
}

void LLRemoteParcelInfoProcessor::removeObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
	if (!observer)
	{
		return;
	}

	observer_multimap_t::iterator it;
	observer_multimap_t::iterator start = mObservers.lower_bound(parcel_id);
	observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

	for(it = start; it != end; ++it)
	{
		if (it->second.get() == observer)
		{
			mObservers.erase(it);
			break;
		}
	}
}

//static
void LLRemoteParcelInfoProcessor::processParcelInfoReply(LLMessageSystem* msg, void**)
{
	LLParcelData parcel_data;

	msg->getUUID	("Data", "ParcelID", parcel_data.parcel_id);
	msg->getUUID	("Data", "OwnerID", parcel_data.owner_id);
	msg->getString	("Data", "Name", parcel_data.name);
	msg->getString	("Data", "Desc", parcel_data.desc);
	msg->getS32		("Data", "ActualArea", parcel_data.actual_area);
	msg->getS32		("Data", "BillableArea", parcel_data.billable_area);
	msg->getU8		("Data", "Flags", parcel_data.flags);
	msg->getF32		("Data", "GlobalX", parcel_data.global_x);
	msg->getF32		("Data", "GlobalY", parcel_data.global_y);
	msg->getF32		("Data", "GlobalZ", parcel_data.global_z);
	msg->getString	("Data", "SimName", parcel_data.sim_name);
	msg->getUUID	("Data", "SnapshotID", parcel_data.snapshot_id);
	msg->getF32		("Data", "Dwell", parcel_data.dwell);
	msg->getS32		("Data", "SalePrice", parcel_data.sale_price);
	msg->getS32		("Data", "AuctionID", parcel_data.auction_id);

	LLRemoteParcelInfoProcessor::observer_multimap_t & observers = LLRemoteParcelInfoProcessor::getInstance()->mObservers;

	typedef std::vector<observer_multimap_t::iterator> deadlist_t;
	deadlist_t dead_iters;

	observer_multimap_t::iterator oi = observers.lower_bound(parcel_data.parcel_id);
	observer_multimap_t::iterator end = observers.upper_bound(parcel_data.parcel_id);

	while (oi != end)
	{
		// increment the loop iterator now since it may become invalid below
		observer_multimap_t::iterator cur_oi = oi++;

		LLRemoteParcelInfoObserver * observer = cur_oi->second.get();
		if(observer)
		{
			// may invalidate cur_oi if the observer removes itself
			observer->processParcelInfo(parcel_data);
		}
		else
		{
			// the handle points to an expired observer, so don't keep it
			// around anymore
			dead_iters.push_back(cur_oi);
		}
	}

	deadlist_t::iterator i;
	deadlist_t::iterator end_dead = dead_iters.end();
	for(i = dead_iters.begin(); i != end_dead; ++i)
	{
		observers.erase(*i);
	}

#ifdef LL_LLURLENTRY_H
	LLUrlEntryParcel::LLParcelData url_data;
	url_data.parcel_id = parcel_data.parcel_id;
	url_data.name = parcel_data.name;
	url_data.sim_name = parcel_data.sim_name;
	url_data.global_x = parcel_data.global_x;
	url_data.global_y = parcel_data.global_y;
	url_data.global_z = parcel_data.global_z;

	// Pass the parcel data to LLUrlEntryParcel to render
	// human readable parcel name.
	LLUrlEntryParcel::processParcelInfo(url_data);
#endif //LL_LLURLENTRY_H
}

void LLRemoteParcelInfoProcessor::sendParcelInfoRequest(const LLUUID& parcel_id)
{
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessage("ParcelInfoRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("ParcelID", parcel_id);
	gAgent.sendReliableMessage();
}
