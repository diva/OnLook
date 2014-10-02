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
 * Rewritten by Liru Færs to act as its own ui element and use a control
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "lldroptarget.h"

#include <boost/signals2/shared_connection_block.hpp>

#include "llbutton.h"
#include "llinventorymodel.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llviewborder.h"

static LLRegisterWidget<LLDropTarget> r("drop_target");

std::string currently_set_to(const LLInventoryItem* item)
{
	if (!item) return LLTrans::getString("CurrentlyNotSet");
	LLStringUtil::format_map_t args;
	args["[ITEM]"] = item->getName();
	return LLTrans::getString("CurrentlySetTo", args);
}

LLDropTarget::LLDropTarget(const LLDropTarget::Params& p)
:	LLView(p)
,	mReset(NULL)
{
	setToolTip(std::string(p.tool_tip));

	mText = new LLTextBox("drop_text", LLRect(), p.label);
	addChild(mText);

	setControlName(p.control_name, NULL);
	mText->setFollows(FOLLOWS_NONE);
	mText->setMouseOpaque(false);
	mText->setHAlign(LLFontGL::HCENTER);
	mText->setVPad(1);

	mBorder = new LLViewBorder("drop_border", LLRect(), LLViewBorder::BEVEL_IN);
	addChild(mBorder);

	mBorder->setMouseOpaque(false);
	if (!p.border_visible) mBorder->setBorderWidth(0);

	if (p.show_reset)
	{
		addChild(mReset = new LLButton("reset", LLRect(), "icn_clear_lineeditor.tga", "icn_clear_lineeditor.tga", "", boost::bind(&LLDropTarget::setValue, this, _2)));
	}

	// Now set the rects of the children
	p.fill_parent ? fillParent(getParent()) : setChildRects(p.rect);
}

LLDropTarget::~LLDropTarget()
{
}

// static
LLView* LLDropTarget::fromXML(LLXMLNodePtr node, LLView* parent, LLUICtrlFactory* factory)
{
	LLDropTarget* target = new LLDropTarget;
	target->initFromXML(node, parent);
	return target;
}

// virtual
void LLDropTarget::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	LLView::initFromXML(node, parent);

	const LLRect& rect = getRect();
	if (node->hasAttribute("show_reset"))
	{
		bool show;
		node->getAttribute_bool("show_reset", show);
		if (!show)
		{
			delete mReset;
			mReset = NULL;
		}
	}
	setChildRects(LLRect(0, rect.getHeight(), rect.getWidth(), 0));

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

	if (node->hasAttribute("border_visible"))
	{
		bool border_visible;
		node->getAttribute_bool("border_visible", border_visible);
		if (!border_visible) mBorder->setBorderWidth(0);
	}
}

// virtual
void LLDropTarget::setControlName(const std::string& control_name, LLView* context)
{
	if (control_name.empty()) // The "empty set"
	{
		mControl = NULL;
		mConnection.disconnect();
		return; // This DropTarget never changes text, it isn't tied to a control
	}

	bool none(true);
	std::string text;
	if (LLStartUp::getStartupState() != STATE_STARTED) // Too early for PerAccount
	{
		text = LLTrans::getString("NotLoggedIn");
	}
	else
	{
		mControl = gSavedPerAccountSettings.getControl(control_name);
		if (!mControl)
		{
			llerrs << "Could not find control \"" << control_name << "\" in gSavedPerAccountSettings" << llendl;
			return; // Though this should never happen.
		}
		const LLUUID id(mControl->getValue().asString());
		none = id.isNull();
		if (none)
			text = LLTrans::getString("CurrentlyNotSet");
		else if (LLViewerInventoryItem* item = gInventory.getItem(id))
			text = currently_set_to(item);
		else
			text = LLTrans::getString("CurrentlySetToAnItemNotOnThisAccount");
	}
	if (mControl)
		mConnection = mControl->getSignal()->connect(boost::bind(&LLView::setValue, this, _2));
	else
		mConnection.disconnect();

	mText->setText(text);
	if (mReset) mReset->setVisible(!none);
}

void LLDropTarget::setChildRects(LLRect rect)
{
	mBorder->setRect(rect);
	if (mReset)
	{
		// Reset button takes rightmost part of the text area.
		S32 height(rect.getHeight());
		rect.mRight -= height;
		mText->setRect(rect);
		rect.mLeft = rect.mRight;
		rect.mRight += height;
		mReset->setRect(rect);
	}
	else
	{
		mText->setRect(rect);
	}
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
	setChildRects(getRect()); // Children maintain the old rectangle
	const LLRect& parent_rect = parent->getRect();
	setRect(LLRect(0, parent_rect.getHeight(), parent_rect.getWidth(), 0));
}

void LLDropTarget::setControlValue(const std::string& val)
{
	if (mControl)
	{
		boost::signals2::shared_connection_block block(mConnection);
		mControl->setValue(val);
	}
}

void LLDropTarget::setItem(const LLInventoryItem* item)
{
	if (mReset) mReset->setVisible(!!item);
	mText->setText(currently_set_to(item));
	setControlValue(item ? item->getUUID().asString() : "");
}

void LLDropTarget::setValue(const LLSD& value)
{
	const LLUUID& id(value.asUUID());
	setItem(id.isNull() ? NULL : gInventory.getItem(id));
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg)
{
	if (mEntityID.notNull())
		return getParent() ? LLToolDragAndDrop::handleGiveDragAndDrop(mEntityID, LLUUID::null, drop, cargo_type, cargo_data, accept) : false;

	if (LLViewerInventoryItem* inv_item = static_cast<LLViewerInventoryItem*>(cargo_data))
	{
		*accept = ACCEPT_YES_COPY_SINGLE;
		if (drop) setItem(inv_item);
	}
	return true;
}

