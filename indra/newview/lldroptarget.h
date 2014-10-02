/**
 * @file lldroptarget.h
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

#ifndef LLDROPTARGET_H
#define LLDROPTARGET_H

#include "stdtypes.h"
#include "llview.h"

class LLDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<bool> border_visible; // Whether or not to display the border
		Optional<std::string> control_name; // Control to change on item drop (Per Account only)
		Optional<std::string> label; // Label for the LLTextBox, used when label doesn't dynamically change on drop
		Optional<bool> show_reset; // Whether or not to show the reset button
		Optional<bool> fill_parent; // Whether or not to fill the direct parent, to have a larger drop target.  If true, the next sibling must explicitly define its rect without deltas.
		Params()
		:	border_visible("border_visible", true)
		,	control_name("control_name", "")
		,	label("label", "")
		,	show_reset("show_reset", true)
		,	fill_parent("fill_parent", false)
		{
			changeDefault(mouse_opaque, false);
			changeDefault(follows.flags, FOLLOWS_ALL);
		}
	};

	LLDropTarget(const Params& p = Params());
	~LLDropTarget();

	virtual void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept, std::string& tooltip_msg);
	static LLView* fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory);
	virtual void initFromXML(LLXMLNodePtr node, LLView* parent);
	virtual void setControlName(const std::string& control, LLView* context);
	virtual void setValue(const LLSD& value);

	void setChildRects(LLRect rect);
	void fillParent(const LLView* parent);
	void setEntityID(const LLUUID& id) { mEntityID = id;}

protected:
	virtual void setItem(const class LLInventoryItem* item);
	void setControlValue(const std::string& val);

	LLUUID mEntityID;
	class LLViewBorder* mBorder;
	LLControlVariable* mControl;
	boost::signals2::scoped_connection mConnection;
	class LLTextBox* mText;
	class LLButton* mReset;
};

#endif // LLDROPTARGET_H
