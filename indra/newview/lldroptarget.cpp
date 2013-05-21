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

#include "llviewerprecompiledheaders.h"

#include "lldroptarget.h"

#include "llinventorymodel.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"

static LLRegisterWidget<LLDropTarget> r("drop_target");

static std::string currently_set_to(const LLViewerInventoryItem* item)
{
	LLStringUtil::format_map_t args;
	args["[ITEM]"] = item->getName();
	return LLTrans::getString("CurrentlySetTo", args);
}

LLDropTarget::LLDropTarget(const LLDropTarget::Params& p)
:	LLView(p)
{
	setToolTip(std::string(p.tool_tip));

	mText = new LLTextBox("drop_text", p.rect, p.label);
	addChild(mText);

	setControlName(p.control_name, NULL);
	mText->setOrigin(0, 0);
	mText->setFollows(FOLLOWS_NONE);
	mText->setHAlign(LLFontGL::HCENTER);
	mText->setBorderVisible(true);
	mText->setBorderColor(LLUI::sColorsGroup->getColor("DefaultHighlightLight"));

	if (p.fill_parent) fillParent(getParent());
}

LLDropTarget::~LLDropTarget()
{
	delete mText;
}

// static
LLView* LLDropTarget::fromXML(LLXMLNodePtr node, LLView* parent, LLUICtrlFactory* factory)
{
	LLDropTarget* target = new LLDropTarget();
	target->initFromXML(node, parent);
	return target;
}

// virtual
void LLDropTarget::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	LLView::initFromXML(node, parent);

	const LLRect& rect = getRect();
	mText->setRect(LLRect(0, rect.getHeight(), rect.getWidth(), 0));

	if (node->hasAttribute("name")) // Views can't have names, but drop targets can
	{
		std::string name;
		node->getAttributeString("name", name);
		setName(name);
	}

	if (node->hasAttribute("label"))
	{
		std::string label;
		node->getAttributeString("label", label);
		mText->setText(label);
	}

	if (node->hasAttribute("fill_parent"))
	{
		bool fill;
		node->getAttribute_bool("fill_parent", fill);
		if (fill) fillParent(parent);
	}
}

// virtual
void LLDropTarget::setControlName(const std::string& control_name, LLView* context)
{
	if (control_name.empty()) // The "empty set"
	{
		mControl = NULL;
		return; // This DropTarget never changes text, it isn't tied to a control
	}

	std::string text;
	if (LLStartUp::getStartupState() != STATE_STARTED) // Too early for PerAccount
	{
		text = LLTrans::getString("NotLoggedIn");
	}
	else
	{
		mControl = gSavedPerAccountSettings.getControl(control_name);
		const LLUUID id(mControl->getValue().asString());
		if (id.isNull())
			text = LLTrans::getString("CurrentlyNotSet");
		else if (LLViewerInventoryItem* item = gInventory.getItem(id))
			text = currently_set_to(item);
		else
			text = LLTrans::getString("CurrentlySetToAnItemNotOnThisAccount");
	}

	mText->setText(text);
}

void LLDropTarget::fillParent(const LLView* parent)
{
	if (!parent) return; // No parent to fill

	const std::string& tool_tip = getToolTip();
	if (!tool_tip.empty()) // Don't tool_tip the entire parent
	{
		mText->setToolTip(tool_tip);
		setToolTip(LLStringExplicit(""));
	}

	// The following block enlarges the target, but maintains the desired size for the text and border
	mText->setRect(getRect()); // mText takes over the old rectangle, since the position will now be relative to the parent's rectangle for the text.
	const LLRect& parent_rect = parent->getRect();
	setRect(LLRect(0, parent_rect.getHeight(), parent_rect.getWidth(), 0));
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg)
{
	if (mEntityID.isNull())
	{
		if (LLViewerInventoryItem* inv_item = static_cast<LLViewerInventoryItem*>(cargo_data))
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if (drop)
			{
				mText->setText(currently_set_to(inv_item));
				if (mControl) mControl->setValue(inv_item->getUUID().asString());
			}
		}
		else
		{
			*accept = ACCEPT_NO;
		}
		return true;
	}
	return getParent() ? LLToolDragAndDrop::handleGiveDragAndDrop(mEntityID, LLUUID::null, drop, cargo_type, cargo_data, accept) : false;
}

