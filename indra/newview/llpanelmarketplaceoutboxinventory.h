/**
 * @file llpanelmarketplaceoutboxinventory.h
 * @brief LLOutboxInventoryPanel class declaration
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_OUTBOXINVENTORYPANEL_H
#define LL_OUTBOXINVENTORYPANEL_H


#include "llinventorypanel.h"
#include "llfolderviewitem.h"


class LLOutboxInventoryPanel : public LLInventoryPanel
{
public:
	LLOutboxInventoryPanel(const std::string& name,
			const std::string& sort_order_setting,
			const std::string& start_folder,
			const LLRect& rect,
			LLInventoryModel* inventory,
			BOOL allow_multi_select,
			LLView *parent_view = NULL);
	~LLOutboxInventoryPanel();

	// virtual
	void buildFolderView(/*const LLInventoryPanel::Params& params*/);

	// virtual
	LLFolderViewFolder *	createFolderViewFolder(LLInvFVBridge * bridge);
	LLFolderViewItem *		createFolderViewItem(LLInvFVBridge * bridge);
};


class LLOutboxFolderViewFolder : public LLFolderViewFolder
{
public:
	LLOutboxFolderViewFolder(const std::string& name, LLUIImagePtr icon, LLUIImagePtr icon_open, LLUIImagePtr icon_link, LLFolderView* root, LLFolderViewEventListener* listener);
};


class LLOutboxFolderViewItem : public LLFolderViewItem
{
public:
	LLOutboxFolderViewItem(const std::string& name, LLUIImagePtr icon, LLUIImagePtr icon_open, LLUIImagePtr icon_overlay, S32 creation_date, LLFolderView* root, LLFolderViewEventListener* listener);

	// virtual
	BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	void openItem();
};


#endif //LL_OUTBOXINVENTORYPANEL_H
