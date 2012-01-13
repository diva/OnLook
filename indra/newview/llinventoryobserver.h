/** 
 * @file llinventoryobserver.h
 * @brief LLInventoryObserver class header file
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

#ifndef LL_LLINVENTORYOBSERVERS_H
#define LL_LLINVENTORYOBSERVERS_H

#include "lluuid.h"
#include "llmd5.h"
#include <string>
#include <vector>

class LLViewerInventoryCategory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObserver
//
//   A simple abstract base class that can relay messages when the inventory 
//   changes.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryObserver
{
public:
	// This enumeration is a way to refer to what changed in a more
	// human readable format. You can mask the value provided by
	// chaged() to see if the observer is interested in the change.
	enum 
	{
		NONE = 0,
		LABEL = 1,			// name changed
		INTERNAL = 2,		// internal change, eg, asset uuid different
		ADD = 4,			// something added
		REMOVE = 8,			// something deleted
		STRUCTURE = 16,		// structural change, eg, item or folder moved
		CALLING_CARD = 32,	// online, grant status, cancel, etc change
		ALL = 0xffffffff
	};
	virtual ~LLInventoryObserver() {};
	virtual void changed(U32 mask) = 0;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles all the the fetching necessary. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryFetchObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchObserver() {}
	virtual void changed(U32 mask);

	bool isEverythingComplete() const;
	void fetchItems(const uuid_vec_t& ids);
	virtual void done() = 0;

protected:
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};

class LLInventoryFetchItemsObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchItemsObserver(const LLUUID& item_id = LLUUID::null); 
	LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids); 

	BOOL isFinished() const;

	virtual void startFetch();
	virtual void changed(U32 mask);
	virtual void done() {};
protected:
	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
	uuid_vec_t mIDs;
private:
	LLTimer mFetchingPeriod;

	/**
	 * Period of waiting a notification when requested items get added into inventory.
	 */
	static const F32 FETCH_TIMER_EXPIRY;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchDescendentsObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles fetching based on category. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchDescendentsObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchDescendentsObserver() {}
	virtual void changed(U32 mask);

	typedef std::vector<LLUUID> folder_ref_t;
	void fetchDescendents(const folder_ref_t& ids);
	bool isEverythingComplete() const;
	virtual void done() = 0;

protected:
	bool isComplete(LLViewerInventoryCategory* cat);
	folder_ref_t mIncompleteFolders;
	folder_ref_t mCompleteFolders;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchComboObserver
//
// This class does an appropriate combination of fetch descendents and
// item fetches based on completion of categories and items. Much like
// the fetch and fetch descendents, this will call done() when everything
// has arrived.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchComboObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchComboObserver() : mDone(false) {}
	virtual void changed(U32 mask);

	typedef std::vector<LLUUID> folder_ref_t;
	typedef std::vector<LLUUID> item_ref_t;
	void fetch(const folder_ref_t& folder_ids, const item_ref_t& item_ids);

	virtual void done() = 0;

protected:
	bool mDone;
	folder_ref_t mCompleteFolders;
	folder_ref_t mIncompleteFolders;
	item_ref_t mCompleteItems;
	item_ref_t mIncompleteItems;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryExistenceObserver
//
// This class is used as a base class for doing somethign when all the
// observed item ids exist in the inventory somewhere. You can derive
// a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryExistenceObserver : public LLInventoryObserver
{
public:
	LLInventoryExistenceObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	uuid_vec_t mExist;
	uuid_vec_t mMIA;
};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryAddedObserver
//
// This class is used as a base class for doing something when 
// a new item arrives in inventory.
// It does not watch for a certain UUID, rather it acts when anything is added
// Derive a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryAddedObserver : public LLInventoryObserver
{
public:
	LLInventoryAddedObserver() : mAdded() {}
	virtual void changed(U32 mask);

protected:
	virtual void done() = 0;

	typedef std::vector<LLUUID> item_ref_t;
	item_ref_t mAdded;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryTransactionObserver
//
// Class which can be used as a base class for doing something when an
// inventory transaction completes.
//
// *NOTE: This class is not quite complete. Avoid using unless you fix up it's
// functionality gaps.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryTransactionObserver : public LLInventoryObserver
{
public:
	LLInventoryTransactionObserver(const LLTransactionID& transaction_id);
	virtual void changed(U32 mask);

protected:
	typedef std::vector<LLUUID> folder_ref_t;
	typedef std::vector<LLUUID> item_ref_t;
	virtual void done(const folder_ref_t& folders, const item_ref_t& items) = 0;

	LLTransactionID mTransactionID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCompletionObserver
//
// Class which can be used as a base class for doing something when
// when all observed items are locally complete. This class implements
// the changed() method of LLInventoryObserver and declares a new
// method named done() which is called when all watched items have
// complete information in the inventory model.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCompletionObserver : public LLInventoryObserver
{
public:
	LLInventoryCompletionObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	uuid_vec_t mComplete;
	uuid_vec_t mIncomplete;
};

#endif // LL_LLINVENTORYOBSERVERS_H
