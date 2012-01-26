/** 
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llagent.h"
#include "llgesturemgr.h"
#include "llviewerinventory.h"
#include "llvoavatar.h"
#include "cofmgr.h"
#include "rlvviewer2.h"

// ============================================================================
// From lloutfitobserver.cpp

LLCOFObserver::LLCOFObserver() :
	mCOFLastVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
{
	mItemNameHash.finalize();
	gInventory.addObserver(this);
}

LLCOFObserver::~LLCOFObserver()
{
	if (gInventory.containsObserver(this))
	{
		gInventory.removeObserver(this);
	}
}

void LLCOFObserver::changed(U32 mask)
{
	if (!gInventory.isInventoryUsable())
		return;

	checkCOF();
}

// static
S32 LLCOFObserver::getCategoryVersion(const LLUUID& cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (!cat)
		return LLViewerInventoryCategory::VERSION_UNKNOWN;

	return cat->getVersion();
}

// static
const std::string& LLCOFObserver::getCategoryName(const LLUUID& cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (!cat)
		return LLStringUtil::null;

	return cat->getName();
}

bool LLCOFObserver::checkCOF()
{
	LLUUID cof = LLCOFMgr::getInstance()->getCOF();
	if (cof.isNull())
		return false;

	bool cof_changed = false;
	LLMD5 item_name_hash = hashDirectDescendentNames(cof);
	if (item_name_hash != mItemNameHash)
	{
		cof_changed = true;
		mItemNameHash = item_name_hash;
	}

	S32 cof_version = getCategoryVersion(cof);
	if (cof_version != mCOFLastVersion)
	{
		cof_changed = true;
		mCOFLastVersion = cof_version;
	}

	if (!cof_changed)
		return false;
	
	mCOFChanged();

	return true;
}

LLMD5 LLCOFObserver::hashDirectDescendentNames(const LLUUID& cat_id)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);
	LLMD5 item_name_hash;
	if (!item_array)
	{
		item_name_hash.finalize();
		return item_name_hash;
	}
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array->begin();
		 iter != item_array->end();
		 iter++)
	{
		const LLViewerInventoryItem *item = (*iter);
		if (!item)
			continue;
		item_name_hash.update(item->getName());
	}
	item_name_hash.finalize();
	return item_name_hash;
}

// ============================================================================
