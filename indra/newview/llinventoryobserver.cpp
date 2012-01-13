/** 
 * @file llinventoryobserver.cpp
 * @brief Implementation of the inventory observers used to track agent inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llinventoryobserver.h"

#include "llassetstorage.h"
#include "llcrc.h"
#include "lldir.h"
#include "llsys.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llinventorybridge.h"
//#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewerregion.h"
#include "llappviewer.h"
#include "lldbstrings.h"
#include "llviewerstats.h"
#include "llnotificationsutil.h"
#include "llcallbacklist.h"
#include "llpreview.h"
#include "llviewercontrol.h"
#include "llvoavatarself.h"
#include "llsdutil.h"
#include <deque>

const F32 LLInventoryFetchItemsObserver::FETCH_TIMER_EXPIRY = 60.0f;

void fetch_items_from_llsd(const LLSD& items_llsd);

void LLInventoryCompletionObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mIncomplete.empty())
	{
		for(uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				it = mIncomplete.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mComplete.push_back(*it);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if(mIncomplete.empty())
		{
			done();
		}
	}
}

void LLInventoryCompletionObserver::watchItem(const LLUUID& id)
{
	if(id.notNull())
	{
		mIncomplete.push_back(id);
	}
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const LLUUID& item_id)
{
	mIDs.clear();
	if (item_id != LLUUID::null)
	{
		mIDs.push_back(item_id);
	}
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids)
	: mIDs(item_ids)
{
}

BOOL LLInventoryFetchItemsObserver::isFinished() const
{
	return mIncomplete.empty();
}

void LLInventoryFetchItemsObserver::changed(U32 mask)
{
	lldebugs << this << " remaining incomplete " << mIncomplete.size()
			 << " complete " << mComplete.size()
			 << " wait period " << mFetchingPeriod.getRemainingTimeF32()
			 << llendl;

	// scan through the incomplete items and move or erase them as
	// appropriate.
	if (!mIncomplete.empty())
	{
		// Have we exceeded max wait time?
		bool timeout_expired = mFetchingPeriod.hasExpired();

		for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			const LLUUID& item_id = (*it);
			LLViewerInventoryItem* item = gInventory.getItem(item_id);
			if (item && item->isComplete())
			{
				mComplete.push_back(item_id);
				it = mIncomplete.erase(it);
			}
			else
			{
				if (timeout_expired)
				{
					// Just concede that this item hasn't arrived in reasonable time and continue on.
					llwarns << "Fetcher timed out when fetching inventory item UUID: " << item_id << LL_ENDL;
					it = mIncomplete.erase(it);
				}
				else
				{
					// Keep trying.
					++it;
				}
			}
		}

	}

	if (mIncomplete.empty())
	{
		lldebugs << this << " done at remaining incomplete "
				 << mIncomplete.size() << " complete " << mComplete.size() << llendl;
		done();
	}
	//llinfos << "LLInventoryFetchItemsObserver::changed() mComplete size " << mComplete.size() << llendl;
	//llinfos << "LLInventoryFetchItemsObserver::changed() mIncomplete size " << mIncomplete.size() << llendl;
}

void LLInventoryFetchItemsObserver::startFetch()
{
	LLUUID owner_id;
	LLSD items_llsd;
	for (uuid_vec_t::const_iterator it = mIDs.begin(); it < mIDs.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (item)
		{
			if (item->isComplete())
			{
				// It's complete, so put it on the complete container.
				mComplete.push_back(*it);
				continue;
			}
			else
			{
				owner_id = item->getPermissions().getOwner();
			}
		}
		else
		{
			// assume it's agent inventory.
			owner_id = gAgent.getID();
		}

		// Ignore categories since they're not items.  We
		// could also just add this to mComplete but not sure what the
		// side-effects would be, so ignoring to be safe.
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if (cat)
		{
			continue;
		}

		// It's incomplete, so put it on the incomplete container, and
		// pack this on the message.
		mIncomplete.push_back(*it);
		
		// Prepare the data to fetch
		LLSD item_entry;
		item_entry["owner_id"] = owner_id;
		item_entry["item_id"] = (*it);
		items_llsd.append(item_entry);
	}

	mFetchingPeriod.reset();
	mFetchingPeriod.setTimerExpirySec(FETCH_TIMER_EXPIRY);

	fetch_items_from_llsd(items_llsd);
}

void LLInventoryFetchObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mIncomplete.empty())
	{
		for(uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				// BUG: This can cause done() to get called prematurely below.
				// This happens with the LLGestureInventoryFetchObserver that
				// loads gestures at startup. JC
				it = mIncomplete.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mComplete.push_back(*it);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if(mIncomplete.empty())
		{
			done();
		}
	}
	//llinfos << "LLInventoryFetchObserver::changed() mComplete size " << mComplete.size() << llendl;
	//llinfos << "LLInventoryFetchObserver::changed() mIncomplete size " << mIncomplete.size() << llendl;
}

bool LLInventoryFetchObserver::isEverythingComplete() const
{
	return mIncomplete.empty();
}

void fetch_items_from_llsd(const LLSD& items_llsd)
{
	if (!items_llsd.size() || gDisconnected) return;
	LLSD body;
	body[0]["cap_name"] = "FetchInventory";
	body[1]["cap_name"] = "FetchLib";
	for (S32 i=0; i<items_llsd.size();i++)
	{
		if (items_llsd[i]["owner_id"].asString() == gAgent.getID().asString())
		{
			body[0]["items"].append(items_llsd[i]);
			continue;
		}
		else if (items_llsd[i]["owner_id"].asString() == ALEXANDRIA_LINDEN_ID.asString())
		{
			body[1]["items"].append(items_llsd[i]);
			continue;
		}
	}
		
	for (S32 i=0; i<body.size(); i++)
	{
		if (!gAgent.getRegion())
		{
			llwarns << "Agent's region is null" << llendl;
			break;
		}

		if (0 == body[i]["items"].size()) {
			lldebugs << "Skipping body with no items to fetch" << llendl;
			continue;
		}

		std::string url = gAgent.getRegion()->getCapability(body[i]["cap_name"].asString());
		if (!url.empty())
		{
			body[i]["agent_id"]	= gAgent.getID();
			LLHTTPClient::post(url, body[i], new LLInventoryModel::fetchInventoryResponder(body[i]));
			continue;
		}

		LLMessageSystem* msg = gMessageSystem;
		BOOL start_new_message = TRUE;
		for (S32 j=0; j<body[i]["items"].size(); j++)
		{
			LLSD item_entry = body[i]["items"][j];
			if(start_new_message)
			{
				start_new_message = FALSE;
				msg->newMessageFast(_PREHASH_FetchInventory);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_OwnerID, item_entry["owner_id"].asUUID());
			msg->addUUIDFast(_PREHASH_ItemID, item_entry["item_id"].asUUID());
			if(msg->isSendFull(NULL))
			{
				start_new_message = TRUE;
				gAgent.sendReliableMessage();
			}
		}
		if(!start_new_message)
		{
			gAgent.sendReliableMessage();
		}
	}
}

void LLInventoryFetchObserver::fetchItems(
	const uuid_vec_t& ids)
{
	LLUUID owner_id;
	LLSD items_llsd;
	for(uuid_vec_t::const_iterator it = ids.begin(); it < ids.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if(item)
		{
			if(item->isComplete())
			{
				// It's complete, so put it on the complete container.
				mComplete.push_back(*it);
				continue;
			}
			else
			{
				owner_id = item->getPermissions().getOwner();
			}
		}
		else
		{
			// assume it's agent inventory.
			owner_id = gAgent.getID();
		}
		
		// It's incomplete, so put it on the incomplete container, and
		// pack this on the message.
		mIncomplete.push_back(*it);
		
		// Prepare the data to fetch
		LLSD item_entry;
		item_entry["owner_id"] = owner_id;
		item_entry["item_id"] = (*it);
		items_llsd.append(item_entry);
	}
	fetch_items_from_llsd(items_llsd);
}
// virtual
void LLInventoryFetchDescendentsObserver::changed(U32 mask)
{
	for(folder_ref_t::iterator it = mIncompleteFolders.begin(); it < mIncompleteFolders.end();)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if(!cat)
		{
			it = mIncompleteFolders.erase(it);
			continue;
		}
		if(isComplete(cat))
		{
			mCompleteFolders.push_back(*it);
			it = mIncompleteFolders.erase(it);
			continue;
		}
		++it;
	}
	if(mIncompleteFolders.empty())
	{
		done();
	}
}

void LLInventoryFetchDescendentsObserver::fetchDescendents(
	const folder_ref_t& ids)
{
	for(folder_ref_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if(!cat) continue;
		if(!isComplete(cat))
		{
			cat->fetchDescendents();		//blindly fetch it without seeing if anything else is fetching it.
			mIncompleteFolders.push_back(*it);	//Add to list of things being downloaded for this observer.
		}
		else
		{
			mCompleteFolders.push_back(*it);
		}
	}
}

bool LLInventoryFetchDescendentsObserver::isEverythingComplete() const
{
	return mIncompleteFolders.empty();
}

bool LLInventoryFetchDescendentsObserver::isComplete(LLViewerInventoryCategory* cat)
{
	S32 version = cat->getVersion();
	S32 descendents = cat->getDescendentCount();
	if((LLViewerInventoryCategory::VERSION_UNKNOWN == version)
	   || (LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN == descendents))
	{
		return false;
	}
	// it might be complete - check known descendents against
	// currently available.
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
	if(!cats || !items)
	{
		// bit of a hack - pretend we're done if they are gone or
		// incomplete. should never know, but it would suck if this
		// kept tight looping because of a corrupt memory state.
		return true;
	}
	S32 known = cats->count() + items->count();
	if(descendents == known)
	{
		// hey - we're done.
		return true;
	}
	return false;
}

void LLInventoryFetchComboObserver::changed(U32 mask)
{
	if(!mIncompleteItems.empty())
	{
		for(item_ref_t::iterator it = mIncompleteItems.begin(); it < mIncompleteItems.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				it = mIncompleteItems.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mCompleteItems.push_back(*it);
				it = mIncompleteItems.erase(it);
				continue;
			}
			++it;
		}
	}
	if(!mIncompleteFolders.empty())
	{
		for(folder_ref_t::iterator it = mIncompleteFolders.begin(); it < mIncompleteFolders.end();)
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
			if(!cat)
			{
				it = mIncompleteFolders.erase(it);
				continue;
			}
			if(gInventory.isCategoryComplete(*it))
			{
				mCompleteFolders.push_back(*it);
				it = mIncompleteFolders.erase(it);
				continue;
			}
			++it;
		}
	}
	if(!mDone && mIncompleteItems.empty() && mIncompleteFolders.empty())
	{
		mDone = true;
		done();
	}
}

void LLInventoryFetchComboObserver::fetch(
	const folder_ref_t& folder_ids,
	const item_ref_t& item_ids)
{
	lldebugs << "LLInventoryFetchComboObserver::fetch()" << llendl;
	for(folder_ref_t::const_iterator fit = folder_ids.begin(); fit != folder_ids.end(); ++fit)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*fit);
		if(!cat) continue;
		if(!gInventory.isCategoryComplete(*fit))
		{
			cat->fetchDescendents();
			lldebugs << "fetching folder " << *fit <<llendl;
			mIncompleteFolders.push_back(*fit);
		}
		else
		{
			mCompleteFolders.push_back(*fit);
			lldebugs << "completing folder " << *fit <<llendl;
		}
	}

	// Now for the items - we fetch everything which is not a direct
	// descendent of an incomplete folder because the item will show
	// up in an inventory descendents message soon enough so we do not
	// have to fetch it individually.
	LLSD items_llsd;
	LLUUID owner_id;
	for(item_ref_t::const_iterator iit = item_ids.begin(); iit != item_ids.end(); ++iit)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*iit);
		if(!item)
		{
			lldebugs << "uanble to find item " << *iit << llendl;
			continue;
		}
		if(item->isComplete())
		{
			// It's complete, so put it on the complete container.
			mCompleteItems.push_back(*iit);
			lldebugs << "completing item " << *iit << llendl;
			continue;
		}
		else
		{
			mIncompleteItems.push_back(*iit);
			owner_id = item->getPermissions().getOwner();
		}
		if(std::find(mIncompleteFolders.begin(), mIncompleteFolders.end(), item->getParentUUID()) == mIncompleteFolders.end())
		{
			LLSD item_entry;
			item_entry["owner_id"] = owner_id;
			item_entry["item_id"] = (*iit);
			items_llsd.append(item_entry);
		}
		else
		{
			lldebugs << "not worrying about " << *iit << llendl;
		}
	}
	fetch_items_from_llsd(items_llsd);
}

void LLInventoryExistenceObserver::watchItem(const LLUUID& id)
{
	if(id.notNull())
	{
		mMIA.push_back(id);
	}
}

void LLInventoryExistenceObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mMIA.empty())
	{
		for(uuid_vec_t::iterator it = mMIA.begin(); it < mMIA.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				++it;
				continue;
			}
			mExist.push_back(*it);
			it = mMIA.erase(it);
		}
		if(mMIA.empty())
		{
			done();
		}
	}
}
void LLInventoryAddedObserver::changed(U32 mask)
{
	if(!(mask & LLInventoryObserver::ADD))
	{
		return;
	}

	// *HACK: If this was in response to a packet off
	// the network, figure out which item was updated.
	LLMessageSystem* msg = gMessageSystem;

	std::string msg_name = msg->getMessageName();
	if (msg_name.empty())
	{
		return;
	}
	
	// We only want newly created inventory items. JC
	if ( msg_name != "UpdateCreateInventoryItem")
	{
		return;
	}

	LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < num_blocks; ++i)
	{
		titem->unpackMessage(msg, _PREHASH_InventoryData, i);
		if (!(titem->getUUID().isNull()))
		{
			//we don't do anything with null keys
			mAdded.push_back(titem->getUUID());
		}
	}
	if (!mAdded.empty())
	{
		done();
	}
}

LLInventoryTransactionObserver::LLInventoryTransactionObserver(const LLTransactionID& transaction_id) :
	mTransactionID(transaction_id)
{
}

void LLInventoryTransactionObserver::changed(U32 mask)
{
	if(mask & LLInventoryObserver::ADD)
	{
		// This could be it - see if we are processing a bulk update
		LLMessageSystem* msg = gMessageSystem;
		if(msg->getMessageName()
		   && (0 == strcmp(msg->getMessageName(), "BulkUpdateInventory")))
		{
			// we have a match for the message - now check the
			// transaction id.
			LLUUID id;
			msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, id);
			if(id == mTransactionID)
			{
				// woo hoo, we found it
				uuid_vec_t folders;
				uuid_vec_t items;
				S32 count;
				count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
				S32 i;
				for(i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, id, i);
					if(id.notNull())
					{
						folders.push_back(id);
					}
				}
				count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
				for(i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_ItemData, _PREHASH_ItemID, id, i);
					if(id.notNull())
					{
						items.push_back(id);
					}
				}

				// call the derived class the implements this method.
				done(folders, items);
			}
		}
	}
}