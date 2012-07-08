/**
 * @file lldroptarget.cpp
 * @Class LLDropTarget
 *
 * This handy class is a simple way to drop something on another
 * view. It handles drop events, always setting itself to the size of
 * its parent.
 *
 * Altered to support a callback so it can return the item
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Pulled into its own file for more widespread use
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "lldroptarget.h"
#include "lltooldraganddrop.h"
#include "llinventorymodel.h"

LLDropTarget::LLDropTarget(const std::string& name, const LLRect& rect, const LLUUID& agent_id) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mAgentID(agent_id),
	mDownCallback(NULL)
{
}

LLDropTarget::LLDropTarget(const std::string& name, const LLRect& rect, void (*callback)(LLViewerInventoryItem*)) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mDownCallback(callback)
{
}

LLDropTarget::~LLDropTarget()
{
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg)
{
	if(!getParent())	return false;
	if(!mDownCallback)		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop, cargo_type, cargo_data, accept);
	else
	{
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if (gInventory.getItem(inv_item->getUUID()))
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if (drop)	mDownCallback(inv_item);
		}
		else	*accept = ACCEPT_NO;
	}
	return true;
}
