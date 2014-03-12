/**
 * @file llpanelmarketplaceoutboxinventory.cpp
 * @brief LLOutboxInventoryPanel  class definition
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

#include "llviewerprecompiledheaders.h"

#include "llpanelmarketplaceoutboxinventory.h"

//#include "llfolderview.h"
//#include "llfoldervieweventlistener.h"
#include "llinventorybridge.h"
//#include "llinventoryfunctions.h"
#include "lltrans.h"
//#include "llviewerfoldertype.h"


//
// statics
//

static LLRegisterWidget<LLOutboxInventoryPanel> r1("outbox_inventory_panel");
//static LLRegisterWidget<LLOutboxFolderViewFolder> r2("outbox_folder_view_folder");

//
// LLOutboxInventoryPanel Implementation
//

LLOutboxInventoryPanel::LLOutboxInventoryPanel(const std::string& name,
								    const std::string& sort_order_setting,
								    const std::string& start_folder,
									const LLRect& rect,
									LLInventoryModel* inventory,
									BOOL allow_multi_select,
									LLView* parent_view)
	: LLInventoryPanel(name, sort_order_setting, start_folder, rect, inventory, allow_multi_select, parent_view)
{
}

LLOutboxInventoryPanel::~LLOutboxInventoryPanel()
{
}

// virtual
void LLOutboxInventoryPanel::buildFolderView(/*const LLInventoryPanel::Params& params*/)
{
	// Determine the root folder in case specified, and
	// build the views starting with that folder.

	LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);

	if (root_id == LLUUID::null)
	{
		llwarns << "Outbox inventory panel has no root folder!" << llendl;
		root_id = LLUUID::generateNewID();
	}

	LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(LLAssetType::AT_CATEGORY,
																	LLAssetType::AT_CATEGORY,
																	LLInventoryType::IT_CATEGORY,
																	this,
																	NULL,
																	root_id);

	mFolderRoot = createFolderView(new_listener, true/*params.use_label_suffix()*/)->getHandle();
}

LLFolderViewFolder * LLOutboxInventoryPanel::createFolderViewFolder(LLInvFVBridge * bridge)
{
	return new LLOutboxFolderViewFolder(
		bridge->getDisplayName(),
		bridge->getIcon(),
		bridge->getOpenIcon(),
		LLUI::getUIImage("inv_link_overlay.tga"),
		mFolderRoot.get(),
		bridge);
}

LLFolderViewItem * LLOutboxInventoryPanel::createFolderViewItem(LLInvFVBridge * bridge)
{
	return new LLOutboxFolderViewItem(
		bridge->getDisplayName(),
		bridge->getIcon(),
		bridge->getOpenIcon(),
		LLUI::getUIImage("inv_link_overlay.tga"),
		bridge->getCreationDate(),
		mFolderRoot.get(),
		bridge);
}

//
// LLOutboxFolderViewFolder Implementation
//

LLOutboxFolderViewFolder::LLOutboxFolderViewFolder(const std::string& name, LLUIImagePtr icon,
										LLUIImagePtr icon_open,
										LLUIImagePtr icon_link,
										LLFolderView* root,
										LLFolderViewEventListener* listener)
	: LLFolderViewFolder(name, icon, icon_open, icon_link, root, listener)
{
}

//
// LLOutboxFolderViewItem Implementation
//

LLOutboxFolderViewItem::LLOutboxFolderViewItem(const std::string& name, LLUIImagePtr icon,
										LLUIImagePtr icon_open,
										LLUIImagePtr icon_overlay,
										S32 creation_date,
										LLFolderView* root,
										LLFolderViewEventListener* listener)
	: LLFolderViewItem(name, icon, icon_open, icon_overlay, creation_date, root, listener)
{
}

BOOL LLOutboxFolderViewItem::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

void LLOutboxFolderViewItem::openItem()
{
	// Intentionally do nothing to block attaching items from the outbox
}

// eof
