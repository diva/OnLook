/** 
 * @file lllayoutstack.h
 * @author Richard Nelson
 * @brief LLLayout class - dynamic stacking of UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Reshasearch, Inc.
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

#ifndef LL_LLLAYOUTSTACK_H
#define LL_LLLAYOUTSTACK_H

#include "llpanel.h"


class LLLayoutPanel;


class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:
	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} eLayoutOrientation;

	LLLayoutStack(eLayoutOrientation orientation);
	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ void removeChild(LLView* ctrl);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	S32 getMinWidth() const { return mMinWidth; }
	S32 getMinHeight() const { return mMinHeight; }
	
	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize, EAnimate animate = NO_ANIMATE, S32 index = S32_MAX);
	void removePanel(LLPanel* panel);
	void collapsePanel(LLPanel* panel, BOOL collapsed = TRUE);
	S32 getNumPanels() { return mPanels.size(); }

	void updateLayout(BOOL force_resize = FALSE);

	S32 getPanelSpacing() const { return mPanelSpacing; }
private:
	
	void calcMinExtents();
	S32 getDefaultHeight(S32 cur_height);
	S32 getDefaultWidth(S32 cur_width);

	const eLayoutOrientation mOrientation;

	typedef std::vector<LLLayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;
	LLLayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;

	S32 mMinWidth;
	S32 mMinHeight;
	S32 mPanelSpacing;
}; // end class LLLayoutStack

class LLLayoutPanel
{
	friend class LLLayoutStack;
	friend class LLUICtrlFactory;
	friend struct DeletePointer;
	LLLayoutPanel(LLPanel* panelp, LLLayoutStack::eLayoutOrientation orientation, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize);

	~LLLayoutPanel();

	F32 getCollapseFactor();
	LLPanel* mPanel;
	S32 mMinWidth;
	S32 mMinHeight;
	bool mAutoResize;
	bool mUserResize;
	bool mCollapsed;


	F32 mVisibleAmt;
	F32 mCollapseAmt;
	LLLayoutStack::eLayoutOrientation mOrientation;
	class LLResizeBar* mResizeBar;
};

#endif //LL_LLLAYOUTSTACK_H