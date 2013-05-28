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

#ifndef LLDROPTARGET_H
#define LLDROPTARGET_H

#include "stdtypes.h"
#include "llview.h"

class LLDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string> control_name; // Control to change on item drop (Per Account only)
		Optional<std::string> label; // Label for the LLTextBox, used when label doesn't dynamically change on drop
		Optional<bool> fill_parent; // Whether or not to fill the direct parent, to have a larger drop target.  If true, the next sibling must explicitly define its rect without deltas.
		Params()
		:	control_name("control_name", "")
		,	label("label", "")
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

	void fillParent(const LLView* parent);
	void setEntityID(const LLUUID& id) { mEntityID = id;}
protected:
	LLUUID mEntityID;
private:
	LLControlVariable* mControl;
	class LLTextBox* mText;
};

#endif // LLDROPTARGET_H
