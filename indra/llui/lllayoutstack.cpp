/** 
 * @file lllayoutstack.cpp
 * @brief LLLayout class - dynamic stacking of UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// Opaque view with a background and a border.  Can contain LLUICtrls.

#include "linden_common.h"

#include "lllayoutstack.h"

#include "lllocalcliprect.h"
#include "llpanel.h"
#include "llresizebar.h"
#include "lluictrlfactory.h"
#include "llcriticaldamp.h"
#include "boost/foreach.hpp"

static const F32 MIN_FRACTIONAL_SIZE = 0.0f;
static const F32 MAX_FRACTIONAL_SIZE = 1.f;
static const S32 RESIZE_BAR_OVERLAP = 1;
static const S32 RESIZE_BAR_HEIGHT = 3;

//
// LLLayoutPanel
//
LLLayoutPanel::LLLayoutPanel(LLPanel* panelp, LLLayoutStack::eLayoutOrientation orientation, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize) : 
	mPanel(panelp), 
	mMinWidth(min_width), 
	mMinHeight(min_height),
	mAutoResize(auto_resize),
	mUserResize(user_resize),
	mCollapsed(FALSE),
	mCollapseAmt(0.f),
	mVisibleAmt(1.f), // default to fully visible
	mResizeBar(NULL),
	mOrientation(orientation)
{
	LLResizeBar::Side side = (orientation == LLLayoutStack::HORIZONTAL) ? LLResizeBar::RIGHT : LLResizeBar::BOTTOM;

	S32 min_dim;
	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		min_dim = mMinHeight;
	}
	else
	{
		min_dim = mMinWidth;
	}
	LLResizeBar::Params p;
	p.name = "resizer";
	p.resizing_view = mPanel;
	p.min_size = min_dim;
	p.max_size = S32_MAX;
	p.side = side;
	mResizeBar = LLUICtrlFactory::create<LLResizeBar>(p);
	mResizeBar->setEnableSnapping(FALSE);
	// panels initialized as hidden should not start out partially visible
	if (!mPanel->getVisible())
	{
			mVisibleAmt = 0.f;
	}
}

LLLayoutPanel::~LLLayoutPanel()
{
	// probably not necessary, but...
	delete mResizeBar;
	mResizeBar = NULL;
}

F32 LLLayoutPanel::getCollapseFactor()
{
	if (mOrientation == LLLayoutStack::HORIZONTAL)
	{
		F32 collapse_amt = 
			clamp_rescale(mCollapseAmt, 0.f, 1.f, 1.f, (F32)mMinWidth / (F32)llmax(1, mPanel->getRect().getWidth()));
		return mVisibleAmt * collapse_amt;
	}
	else
	{
		F32 collapse_amt = 
			clamp_rescale(mCollapseAmt, 0.f, 1.f, 1.f, llmin(1.f, (F32)mMinHeight / (F32)llmax(1, mPanel->getRect().getHeight())));
		return mVisibleAmt * collapse_amt;
	}
}


static LLRegisterWidget<LLLayoutStack> r2("layout_stack");

LLLayoutStack::LLLayoutStack(eLayoutOrientation orientation) : 
		mOrientation(orientation),
		mMinWidth(0),
		mMinHeight(0),
		mPanelSpacing(RESIZE_BAR_HEIGHT)
{
}

LLLayoutStack::~LLLayoutStack()
{
	e_panel_list_t panels = mPanels; // copy list of panel pointers
	mPanels.clear(); // clear so that removeChild() calls don't cause trouble
	std::for_each(panels.begin(), panels.end(), DeletePointer());
}

void LLLayoutStack::draw()
{
	updateLayout();

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		// clip to layout rectangle, not bounding rectangle
		LLRect clip_rect = (*panel_it)->mPanel->getRect();
		// scale clipping rectangle by visible amount
		if (mOrientation == HORIZONTAL)
		{
			clip_rect.mRight = clip_rect.mLeft + llround((F32)clip_rect.getWidth() * (*panel_it)->getCollapseFactor());
		}
		else
		{
			clip_rect.mBottom = clip_rect.mTop - llround((F32)clip_rect.getHeight() * (*panel_it)->getCollapseFactor());
		}

		LLPanel* panelp = (*panel_it)->mPanel;

		LLLocalClipRect clip(clip_rect);
		// only force drawing invisible children if visible amount is non-zero
		drawChild(panelp, 0, 0, clip_rect.notEmpty());
	}
}

void LLLayoutStack::removeChild(LLView* ctrl)
{
	LLView::removeChild(ctrl);
	LLPanel* panel = dynamic_cast<LLPanel*>(ctrl);
	if(!panel)
		return;
	LLLayoutPanel* embedded_panelp = findEmbeddedPanel(panel);

	if (embedded_panelp)
	{
		mPanels.erase(std::find(mPanels.begin(), mPanels.end(), embedded_panelp));
		delete embedded_panelp;
	}

	// need to update resizebars
	
	calcMinExtents();
}

LLXMLNodePtr LLLayoutStack::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();
	node->setName(LL_LAYOUT_STACK_TAG);

	if (mOrientation == HORIZONTAL)
	{
		node->createChild("orientation", TRUE)->setStringValue("horizontal");
	}
	else
	{
		node->createChild("orientation", TRUE)->setStringValue("vertical");
	}

	if (save_children)
	{
		LLView::child_list_const_reverse_iter_t rit;
		for (rit = getChildList()->rbegin(); rit != getChildList()->rend(); ++rit)
		{
			LLView* childp = *rit;

			if (childp->getSaveToXML())
			{
				LLXMLNodePtr xml_node = childp->getXML();

				if (xml_node->hasName(LL_PANEL_TAG))
				{
					xml_node->setName(LL_LAYOUT_PANEL_TAG);
				}

				node->addChild(xml_node);
			}
		}
	}

	return node;
}

//static 
LLView* LLLayoutStack::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string orientation_string("vertical");
	node->getAttributeString("orientation", orientation_string);

	eLayoutOrientation orientation = VERTICAL;

	if (orientation_string == "horizontal")
	{
		orientation = HORIZONTAL;
	}
	else if (orientation_string == "vertical")
	{
		orientation = VERTICAL;
	}
	else
	{
		llwarns << "Unknown orientation " << orientation_string << ", using vertical" << llendl;
	}

	LLLayoutStack* layout_stackp = new LLLayoutStack(orientation);

	node->getAttributeS32("border_size", layout_stackp->mPanelSpacing);
	// don't allow negative spacing values
	layout_stackp->mPanelSpacing = llmax(layout_stackp->mPanelSpacing, 0);

	std::string name("stack");
	node->getAttributeString("name", name);

	layout_stackp->setName(name);
	layout_stackp->initFromXML(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		S32 min_width = 0;
		S32 min_height = 0;
		BOOL auto_resize = TRUE;

		child->getAttributeS32("min_width", min_width);
		child->getAttributeS32("min_height", min_height);
		child->getAttributeBOOL("auto_resize", auto_resize);

		if (child->hasName("layout_panel"))
		{
			BOOL user_resize = TRUE;
			child->getAttributeBOOL("user_resize", user_resize);
			LLPanel* panelp = (LLPanel*)LLPanel::fromXML(child, layout_stackp, factory);
			if (panelp)
			{
				panelp->setFollowsNone();
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize, user_resize);
			}
		}
		else
		{
			BOOL user_resize = FALSE;
			child->getAttributeBOOL("user_resize", user_resize);

			LLPanel* panelp = new LLPanel(std::string("auto_panel"));
			LLView* new_child = factory->createWidget(panelp, child);
			if (new_child)
			{
				// put child in new embedded panel
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize, user_resize);
				// resize panel to contain widget and move widget to be contained in panel
				panelp->setRect(new_child->getRect());
				new_child->setOrigin(0, 0);
			}
			else
			{
				panelp->die();
			}
		}
	}
	layout_stackp->updateLayout();

	return layout_stackp;
}

S32 LLLayoutStack::getDefaultHeight(S32 cur_height)
{
	// if we are spanning our children (crude upward propagation of size)
	// then don't enforce our size on our children
	if (mOrientation == HORIZONTAL)
	{
		cur_height = llmax(mMinHeight, getRect().getHeight());
	}

	return cur_height;
}

S32 LLLayoutStack::getDefaultWidth(S32 cur_width)
{
	// if we are spanning our children (crude upward propagation of size)
	// then don't enforce our size on our children
	if (mOrientation == VERTICAL)
	{
		cur_width = llmax(mMinWidth, getRect().getWidth());
	}

	return cur_width;
}

void LLLayoutStack::addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize, EAnimate animate, S32 index)
{
	// panel starts off invisible (collapsed)
	if (animate == ANIMATE)
	{
		panel->setVisible(FALSE);
	}
	LLLayoutPanel* embedded_panel = new LLLayoutPanel(panel, mOrientation, min_width, min_height, auto_resize, user_resize);
	
	mPanels.insert(mPanels.begin() + llclamp(index, 0, (S32)mPanels.size()), embedded_panel);
	
	addChild(panel);
	addChild(embedded_panel->mResizeBar);

	// bring all resize bars to the front so that they are clickable even over the panels
	// with a bit of overlap
	for (e_panel_list_t::iterator panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLResizeBar* resize_barp = (*panel_it)->mResizeBar;
		sendChildToFront(resize_barp);
	}

	// start expanding panel animation
	if (animate == ANIMATE)
	{
		panel->setVisible(TRUE);
	}
}

void LLLayoutStack::removePanel(LLPanel* panel)
{
	removeChild(panel);
}

void LLLayoutStack::collapsePanel(LLPanel* panel, BOOL collapsed)
{
	LLLayoutPanel* panel_container = findEmbeddedPanel(panel);
	if (!panel_container) return;

	panel_container->mCollapsed = collapsed;
}

void LLLayoutStack::updateLayout(BOOL force_resize)
{
	calcMinExtents();

	// calculate current extents
	S32 total_width = 0;
	S32 total_height = 0;

	const F32 ANIM_OPEN_TIME = 0.02f;
	const F32 ANIM_CLOSE_TIME = 0.03f;

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end();	++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;
		if (panelp->getVisible()) 
		{
			(*panel_it)->mVisibleAmt = lerp((*panel_it)->mVisibleAmt, 1.f, LLCriticalDamp::getInterpolant(ANIM_OPEN_TIME));
			if ((*panel_it)->mVisibleAmt > 0.99f)
			{
				(*panel_it)->mVisibleAmt = 1.f;
			}
		}
		else // not visible
		{
			(*panel_it)->mVisibleAmt = lerp((*panel_it)->mVisibleAmt, 0.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
			if ((*panel_it)->mVisibleAmt < 0.001f)
			{
				(*panel_it)->mVisibleAmt = 0.f;
			}
		}

		if ((*panel_it)->mCollapsed)
		{
			(*panel_it)->mCollapseAmt = lerp((*panel_it)->mCollapseAmt, 1.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
		}
		else
		{
			(*panel_it)->mCollapseAmt = lerp((*panel_it)->mCollapseAmt, 0.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
		}

		if (mOrientation == HORIZONTAL)
		{
			// enforce minimize size constraint by default
			if (panelp->getRect().getWidth() < (*panel_it)->mMinWidth)
			{
				panelp->reshape((*panel_it)->mMinWidth, panelp->getRect().getHeight());
			}
        	total_width += llround(panelp->getRect().getWidth() * (*panel_it)->getCollapseFactor());
        	// want n-1 panel gaps for n panels
			if (panel_it != mPanels.begin())
			{
				total_width += mPanelSpacing;
			}
		}
		else //VERTICAL
		{
			// enforce minimize size constraint by default
			if (panelp->getRect().getHeight() < (*panel_it)->mMinHeight)
			{
				panelp->reshape(panelp->getRect().getWidth(), (*panel_it)->mMinHeight);
			}
			total_height += llround(panelp->getRect().getHeight() * (*panel_it)->getCollapseFactor());
			if (panel_it != mPanels.begin())
			{
				total_height += mPanelSpacing;
			}
		}
	}

	S32 num_resizable_panels = 0;
	S32 shrink_headroom_available = 0;
	S32 shrink_headroom_total = 0;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		// panels that are not fully visible do not count towards shrink headroom
		if ((*panel_it)->getCollapseFactor() < 1.f) 
		{
			continue;
		}

		// if currently resizing a panel or the panel is flagged as not automatically resizing
		// only track total available headroom, but don't use it for automatic resize logic
		if ((*panel_it)->mResizeBar->hasMouseCapture() 
			|| (!(*panel_it)->mAutoResize 
				&& !force_resize))
		{
			if (mOrientation == HORIZONTAL)
			{
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
			}
			else //VERTICAL
			{
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
			}
		}
		else
		{
			num_resizable_panels++;
			if (mOrientation == HORIZONTAL)
			{
				shrink_headroom_available += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
			}
			else //VERTICAL
			{
				shrink_headroom_available += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
			}
		}
	}

	// calculate how many pixels need to be distributed among layout panels
	// positive means panels need to grow, negative means shrink
	S32 pixels_to_distribute;
	if (mOrientation == HORIZONTAL)
	{
		pixels_to_distribute = getRect().getWidth() - total_width;
	}
	else //VERTICAL
	{
		pixels_to_distribute = getRect().getHeight() - total_height;
	}

	// now we distribute the pixels...
	S32 cur_x = 0;
	S32 cur_y = getRect().getHeight();

	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		S32 cur_width = panelp->getRect().getWidth();
		S32 cur_height = panelp->getRect().getHeight();
		S32 new_width = llmax((*panel_it)->mMinWidth, cur_width);
		S32 new_height = llmax((*panel_it)->mMinHeight, cur_height); 

		S32 delta_size = 0;

		// if panel can automatically resize (not animating, and resize flag set)...
		if ((*panel_it)->getCollapseFactor() == 1.f 
			&& (force_resize || (*panel_it)->mAutoResize) 
			&& !(*panel_it)->mResizeBar->hasMouseCapture()) 
		{
			if (mOrientation == HORIZONTAL)
			{
				// if we're shrinking
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					// so we can do this in one pass
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * ((F32)(cur_width - (*panel_it)->mMinWidth) / (F32)shrink_headroom_available)) : 0;
					shrink_headroom_available -= (cur_width - (*panel_it)->mMinWidth);
				}
				else
				{
					// grow all elements equally
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
					num_resizable_panels--;
				}
				pixels_to_distribute -= delta_size;
				new_width = llmax((*panel_it)->mMinWidth, cur_width + delta_size);
			}
			else
			{
				new_width = getDefaultWidth(new_width);
			}

			if (mOrientation == VERTICAL)
			{
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					// so we can do this in one pass
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * ((F32)(cur_height - (*panel_it)->mMinHeight) / (F32)shrink_headroom_available)) : 0;
					shrink_headroom_available -= (cur_height - (*panel_it)->mMinHeight);
				}
				else
				{
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
					num_resizable_panels--;
				}
				pixels_to_distribute -= delta_size;
				new_height = llmax((*panel_it)->mMinHeight, cur_height + delta_size);
			}
			else
			{
				new_height = getDefaultHeight(new_height);
			}
		}
		else
		{
			if (mOrientation == HORIZONTAL)
			{
				new_height = getDefaultHeight(new_height);
			}
			else // VERTICAL
			{
				new_width = getDefaultWidth(new_width);
			}
		}

		// adjust running headroom count based on new sizes
		shrink_headroom_total += delta_size;

		panelp->reshape(new_width, new_height);
		panelp->setOrigin(cur_x, cur_y - new_height);

		LLRect panel_rect = panelp->getRect();
		LLRect resize_bar_rect = panel_rect;
		if (mOrientation == HORIZONTAL)
		{
			resize_bar_rect.mLeft = panel_rect.mRight - RESIZE_BAR_OVERLAP;
			resize_bar_rect.mRight = panel_rect.mRight + mPanelSpacing + RESIZE_BAR_OVERLAP;
		}
		else
		{
			resize_bar_rect.mTop = panel_rect.mBottom + RESIZE_BAR_OVERLAP;
			resize_bar_rect.mBottom = panel_rect.mBottom - mPanelSpacing - RESIZE_BAR_OVERLAP;
		}
		(*panel_it)->mResizeBar->setRect(resize_bar_rect);

		if (mOrientation == HORIZONTAL)
		{
			cur_x += llround(new_width * (*panel_it)->getCollapseFactor()) + mPanelSpacing;
		}
		else //VERTICAL
		{
			cur_y -= llround(new_height * (*panel_it)->getCollapseFactor()) + mPanelSpacing;
		}
	}

	// update resize bars with new limits
	LLResizeBar* last_resize_bar = NULL;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		if (mOrientation == HORIZONTAL)
		{
			(*panel_it)->mResizeBar->setResizeLimits(
				(*panel_it)->mMinWidth, 
				(*panel_it)->mMinWidth + shrink_headroom_total);
		}
		else //VERTICAL
		{
			(*panel_it)->mResizeBar->setResizeLimits(
				(*panel_it)->mMinHeight, 
				(*panel_it)->mMinHeight + shrink_headroom_total);
		}

		// toggle resize bars based on panel visibility, resizability, etc
		BOOL resize_bar_enabled = panelp->getVisible() && (*panel_it)->mUserResize;
		(*panel_it)->mResizeBar->setVisible(resize_bar_enabled);

		if (resize_bar_enabled)
		{
			last_resize_bar = (*panel_it)->mResizeBar;
		}
	}

	// hide last resize bar as there is nothing past it
	// resize bars need to be in between two resizable panels
	if (last_resize_bar)
	{
		last_resize_bar->setVisible(FALSE);
	}

	// not enough room to fit existing contents
	if (force_resize == FALSE
		// layout did not complete by reaching target position
		&& ((mOrientation == VERTICAL && cur_y != -mPanelSpacing)
			|| (mOrientation == HORIZONTAL && cur_x != getRect().getWidth() + mPanelSpacing)))
	{
		// do another layout pass with all stacked elements contributing
		// even those that don't usually resize
		llassert_always(force_resize == FALSE);
		updateLayout(TRUE);
	}
} // end LLLayoutStack::updateLayout


LLLayoutPanel* LLLayoutStack::findEmbeddedPanel(LLPanel* panelp) const
{
	e_panel_list_t::const_iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		if ((*panel_it)->mPanel == panelp)
		{
			return *panel_it;
		}
	}
	return NULL;
}

void LLLayoutStack::calcMinExtents()
{
	mMinWidth = 0;
	mMinHeight = 0;

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		if (mOrientation == HORIZONTAL)
		{
			mMinHeight = llmax(	mMinHeight, 
								(*panel_it)->mMinHeight);
            mMinWidth += (*panel_it)->mMinWidth;
			if (panel_it != mPanels.begin())
			{
				mMinWidth += mPanelSpacing;
			}
		}
		else //VERTICAL
		{
	        mMinWidth = llmax(	mMinWidth, 
								(*panel_it)->mMinWidth);
			mMinHeight += (*panel_it)->mMinHeight;
			if (panel_it != mPanels.begin())
			{
				mMinHeight += mPanelSpacing;
			}
		}
	}
}
