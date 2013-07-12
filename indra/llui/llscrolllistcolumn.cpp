/**
 * @file llscrollcolumnheader.cpp
 * @brief Scroll lists are composed of rows (items), each of which
 * contains columns (cells).
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"

#include "llscrolllistcolumn.h"

#include "llbutton.h"
#include "llkeyboard.h" // For gKeyboard
#include "llresizebar.h"
#include "lluictrlfactory.h"

const S32 MIN_COLUMN_WIDTH = 20;

//---------------------------------------------------------------------------
// LLScrollColumnHeader
//---------------------------------------------------------------------------
LLScrollColumnHeader::LLScrollColumnHeader(const std::string& label, const LLRect& rect, LLScrollListColumn* column, const LLFontGL* fontp)
:	LLComboBox(label, rect, label),
	mColumn(column),
	mOrigLabel(label),
	mHasResizableElement(FALSE)
{
	mListPosition = LLComboBox::ABOVE;
	setCommitCallback(boost::bind(&LLScrollColumnHeader::onSelectSort, this));
	mButton->setTabStop(FALSE);
	mButton->setClickedCallback(boost::bind(&LLScrollColumnHeader::onClick, this, _2));

	mButton->setToolTip(label);

	mAscendingText = std::string("[LOW]...[HIGH](Ascending)"); // *TODO: Translate
	mDescendingText = std::string("[HIGH]...[LOW](Descending)"); // *TODO: Translate

	mList->reshape(llmax(mList->getRect().getWidth(), 110, getRect().getWidth()), mList->getRect().getHeight());

	// resize handles on left and right
	const S32 RESIZE_BAR_THICKNESS = 3;
	LLResizeBar::Params resize_bar_p;
	resize_bar_p.resizing_view(this);
	resize_bar_p.rect(LLRect(getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0));
	resize_bar_p.min_size(MIN_COLUMN_WIDTH);
	resize_bar_p.side(LLResizeBar::RIGHT);
	resize_bar_p.enabled(false);
	mResizeBar = LLUICtrlFactory::create<LLResizeBar>(resize_bar_p);
	addChild(mResizeBar);

	mImageOverlayAlignment = LLFontGL::HCENTER;
	mImageOverlayColor = LLColor4::white;
}

LLScrollColumnHeader::~LLScrollColumnHeader()
{}

void LLScrollColumnHeader::draw()
{
	std::string sort_column = mColumn->mParentCtrl->getSortColumnName();
	BOOL draw_arrow = !mColumn->mLabel.empty()
			&& mColumn->mParentCtrl->isSorted()
			// check for indirect sorting column as well as column's sorting name
			&& (sort_column == mColumn->mSortingColumn || sort_column == mColumn->mName);

	BOOL is_ascending = mColumn->mParentCtrl->getSortAscending();
	if (draw_arrow)
	{
		mButton->setImageOverlay(is_ascending ? "up_arrow.tga" : "down_arrow.tga", LLFontGL::RIGHT, LLColor4::white);
	}
	else
	{
		mButton->setImageOverlay(LLUUID::null);
	}
	mArrowImage = mButton->getImageOverlay();

	// Draw children
	LLComboBox::draw();

	if (mImageOverlay.notNull()) //Ugly dupe code from llbutton...
	{
		BOOL pressed_by_keyboard = FALSE;
		if (mButton->hasFocus())
		{
			pressed_by_keyboard = gKeyboard->getKeyDown(' ') || (mButton->getCommitOnReturn() && gKeyboard->getKeyDown(KEY_RETURN));
		}

		// Unselected image assignments
		S32 local_mouse_x;
		S32 local_mouse_y;
		LLUI::getMousePositionLocal(mButton, &local_mouse_x, &local_mouse_y);

		BOOL pressed = pressed_by_keyboard
					|| (mButton->hasMouseCapture() && mButton->pointInView(local_mouse_x, local_mouse_y))
					|| mButton->getToggleState();

		// Now draw special overlay..
		// let overlay image and text play well together
		S32 button_width = mButton->getRect().getWidth();
		S32 button_height = mButton->getRect().getHeight();
		S32 text_left = mButton->getLeftHPad();
		S32 text_right = button_width - mButton->getRightHPad();
		S32 text_width = text_right - text_left;

		// draw overlay image

		// get max width and height (discard level 0)
		S32 overlay_width = mImageOverlay->getWidth();
		S32 overlay_height = mImageOverlay->getHeight();

		F32 scale_factor = llmin((F32)button_width / (F32)overlay_width, (F32)button_height / (F32)overlay_height, 1.f);
		overlay_width = llround((F32)overlay_width * scale_factor);
		overlay_height = llround((F32)overlay_height * scale_factor);

		S32 center_x = mButton->getLocalRect().getCenterX();
		S32 center_y = mButton->getLocalRect().getCenterY();

		//FUGLY HACK FOR "DEPRESSED" BUTTONS
		if (pressed)
		{
			center_y--;
			center_x++;
		}

		// fade out overlay images on disabled buttons
		LLColor4 overlay_color = mImageOverlayColor;
		if (!mButton->getEnabled())
		{
			overlay_color.mV[VALPHA] = 0.5f;
		}

		switch(mImageOverlayAlignment)
		{
		case LLFontGL::LEFT:
			text_left += overlay_width + 1;
			text_width -= overlay_width + 1;
			mImageOverlay->draw(
				text_left,
				center_y - (overlay_height / 2),
				overlay_width,
				overlay_height,
				overlay_color);
			break;
		case LLFontGL::HCENTER:
			mImageOverlay->draw(
				center_x - (overlay_width / 2),
				center_y - (overlay_height / 2),
				overlay_width,
				overlay_height,
				overlay_color);
			break;
		case LLFontGL::RIGHT:
			text_right -= overlay_width + 1;
			text_width -= overlay_width + 1;
			mImageOverlay->draw(
				text_right - overlay_width,
				center_y - (overlay_height / 2),
				overlay_width,
				overlay_height,
				overlay_color);
			break;
		default:
			// draw nothing
			break;
		}
	}


	if (mList->getVisible())
	{
		// sync sort order with list selection every frame
		mColumn->mParentCtrl->sortByColumn(mColumn->mSortingColumn, getCurrentIndex() == 0);
	}
}

BOOL LLScrollColumnHeader::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (canResize() && mResizeBar->getRect().pointInRect(x, y))
	{
		// reshape column to max content width
		mColumn->mParentCtrl->calcMaxContentWidth();
		LLRect column_rect = getRect();
		column_rect.mRight = column_rect.mLeft + mColumn->mMaxContentWidth;
		setShape(column_rect, true);
	}
	else
	{
		onClick(LLSD());
	}
	return TRUE;
}

void LLScrollColumnHeader::setImage(const std::string& image_name)
{
	if (mButton)
	{
		mButton->setImageSelected(LLUI::getUIImage(image_name));
		mButton->setImageUnselected(LLUI::getUIImage(image_name));
	}
}

void LLScrollColumnHeader::setImageOverlay(const std::string& image_name, LLFontGL::HAlign alignment, const LLColor4& color)
{
	if (image_name.empty())
	{
		mImageOverlay = NULL;
	}
	else
	{
		mImageOverlay = LLUI::getUIImage(image_name);
		mImageOverlayAlignment = alignment;
		mImageOverlayColor = color;
	}
}

void LLScrollColumnHeader::onClick(const LLSD& data)
{
	if (mColumn)
	{
		if (mList->getVisible()) hideList();
		LLScrollListCtrl::onClickColumn(mColumn);

		// propagate new sort order to sort order list
		mList->selectNthItem(mColumn->mParentCtrl->getSortAscending() ? 0 : 1);
		mList->setFocus(true);
	}
}

void LLScrollColumnHeader::onSelectSort()
{
	if (!mColumn) return;
	LLScrollListCtrl* parent = mColumn->mParentCtrl;
	if (!parent) return;
	parent->sortByColumn(mColumn->mSortingColumn, getCurrentIndex() == 0);

	// restore original column header
	setLabel(mOrigLabel);
}

LLView*	LLScrollColumnHeader::findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding)
{
	// this logic assumes dragging on right
	llassert(snap_edge == SNAP_RIGHT);

	// use higher snap threshold for column headers
	threshold = llmin(threshold, 10);

	LLRect snap_rect = getSnapRect();

	S32 snap_delta = mColumn->mMaxContentWidth - snap_rect.getWidth();

	// x coord growing means column growing, so same signs mean we're going in right direction
	if (llabs(snap_delta) <= threshold && mouse_dir.mX * snap_delta > 0 )
	{
		new_edge_val = snap_rect.mRight + snap_delta;
	}
	else
	{
		LLScrollListColumn* next_column = mColumn->mParentCtrl->getColumn(mColumn->mIndex + 1);
		while (next_column)
		{
			if (next_column->mHeader)
			{
				snap_delta = (next_column->mHeader->getSnapRect().mRight - next_column->mMaxContentWidth) - snap_rect.mRight;
				if (llabs(snap_delta) <= threshold && mouse_dir.mX * snap_delta > 0 )
				{
					new_edge_val = snap_rect.mRight + snap_delta;
				}
				break;
			}
			next_column = mColumn->mParentCtrl->getColumn(next_column->mIndex + 1);
		}
	}

	return this;
}

void LLScrollColumnHeader::handleReshape(const LLRect& new_rect, bool by_user)
{
	S32 new_width = new_rect.getWidth();
	S32 delta_width = new_width - (getRect().getWidth() /*+ mColumn->mParentCtrl->getColumnPadding()*/);

	if (delta_width != 0)
	{
		S32 remaining_width = -delta_width;
		S32 col;
		for (col = mColumn->mIndex + 1; col < mColumn->mParentCtrl->getNumColumns(); col++)
		{
			LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
			if (!columnp) continue;

			if (columnp->mHeader && columnp->mHeader->canResize())
			{
				// how many pixels in width can this column afford to give up?
				S32 resize_buffer_amt = llmax(0, columnp->getWidth() - MIN_COLUMN_WIDTH);

				// user shrinking column, need to add width to other columns
				if (delta_width < 0)
				{
					if (columnp->getWidth() > 0)
					{
						// statically sized column, give all remaining width to this column
						columnp->setWidth(columnp->getWidth() + remaining_width);
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->getWidth() / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
						// all padding went to this widget, we're done
						break;
					}
				}
				else
				{
					// user growing column, need to take width from other columns
					remaining_width += resize_buffer_amt;

					if (columnp->getWidth() > 0)
					{
						columnp->setWidth(columnp->getWidth() - llmin(columnp->getWidth() - MIN_COLUMN_WIDTH, delta_width));
						if (columnp->mRelWidth > 0.f)
						{
							columnp->mRelWidth = (F32)columnp->getWidth() / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
						}
					}

					if (remaining_width >= 0)
					{
						// width sucked up from neighboring columns, done
						break;
					}
				}
			}
		}

		// clamp resize amount to maximum that can be absorbed by other columns
		if (delta_width > 0)
		{
			delta_width += llmin(remaining_width, 0);
		}

		// propagate constrained delta_width to new width for this column
		new_width = getRect().getWidth() + delta_width - mColumn->mParentCtrl->getColumnPadding();

		// use requested width
		mColumn->setWidth(new_width);

		// update proportional spacing
		if (mColumn->mRelWidth > 0.f)
		{
			mColumn->mRelWidth = (F32)new_width / (F32)mColumn->mParentCtrl->getItemListRect().getWidth();
		}

		// tell scroll list to layout columns again
		// do immediate update to get proper feedback to resize handle
		// which needs to know how far the resize actually went
		mColumn->mParentCtrl->dirtyColumns();	//Must flag as dirty, else updateColumns will probably be a noop.
		mColumn->mParentCtrl->updateColumns();
	}
}

void LLScrollColumnHeader::setHasResizableElement(BOOL resizable)
{
	if (mHasResizableElement != resizable)
	{
		mColumn->mParentCtrl->dirtyColumns();
		mHasResizableElement = resizable;
	}
}

void LLScrollColumnHeader::updateResizeBars()
{
	S32 num_resizable_columns = 0;
	S32 col;
	for (col = 0; col < mColumn->mParentCtrl->getNumColumns(); col++)
	{
		LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
		if (columnp->mHeader && columnp->mHeader->canResize())
		{
			num_resizable_columns++;
		}
	}

	S32 num_resizers_enabled = 0;

	// now enable/disable resize handles on resizable columns if we have at least two
	for (col = 0; col < mColumn->mParentCtrl->getNumColumns(); col++)
	{
		LLScrollListColumn* columnp = mColumn->mParentCtrl->getColumn(col);
		if (!columnp->mHeader) continue;
		BOOL enable = num_resizable_columns >= 2 && num_resizers_enabled < (num_resizable_columns - 1) && columnp->mHeader->canResize();
		columnp->mHeader->enableResizeBar(enable);
		if (enable)
		{
			num_resizers_enabled++;
		}
	}
}

void LLScrollColumnHeader::enableResizeBar(BOOL enable)
{
	mResizeBar->setEnabled(enable);
}

BOOL LLScrollColumnHeader::canResize()
{
	return getVisible() && (mHasResizableElement || mColumn->mDynamicWidth);
}

//
// LLScrollListColumn
//
// Default constructor
LLScrollListColumn::LLScrollListColumn() : mName(), mSortingColumn(), mSortDirection(ASCENDING), mLabel(), mWidth(-1), mRelWidth(-1.0), mDynamicWidth(false), mMaxContentWidth(0), mIndex(-1), mParentCtrl(NULL), mHeader(NULL), mFontAlignment(LLFontGL::LEFT)
{
}


LLScrollListColumn::LLScrollListColumn(const LLSD& sd, LLScrollListCtrl* parent)
:	mWidth(0),
	mIndex (-1),
	mParentCtrl(parent),
	mName(sd.get("name").asString()),
	mLabel(sd.get("label").asString()),
	mHeader(NULL),
	mMaxContentWidth(0),
	mDynamicWidth(sd.has("dynamicwidth") && sd.get("dynamicwidth").asBoolean()),
	mRelWidth(-1.f),
	mFontAlignment(LLFontGL::LEFT),
	mSortingColumn(sd.has("sort") ? sd.get("sort").asString() : mName)
{
	if (sd.has("sort_ascending"))
	{
		mSortDirection = sd.get("sort_ascending").asBoolean() ? ASCENDING : DESCENDING;
	}
	else
	{
		mSortDirection = ASCENDING;
	}

	if (sd.has("relwidth") && sd.get("relwidth").asFloat() > 0)
	{
		mRelWidth = sd.get("relwidth").asFloat();
		if (mRelWidth > 1) mRelWidth = 1;
		mDynamicWidth = false;
	}
	else if (!mDynamicWidth)
	{
		setWidth(sd.get("width").asInteger());
	}

	if (sd.has("halign"))
	{
		mFontAlignment = (LLFontGL::HAlign)llclamp(sd.get("halign").asInteger(), (S32)LLFontGL::LEFT, (S32)LLFontGL::HCENTER);
	}
}

void LLScrollListColumn::setWidth(S32 width)
{
	if (!mDynamicWidth && mRelWidth <= 0.f)
	{
		mParentCtrl->updateStaticColumnWidth(this, width);
	}
	mWidth = width;
}
