/**
 * @file llscrollcolumnheader.h
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

#ifndef LLSCROLLLISTCOLUMN_H
#define LLSCROLLLISTCOLUMN_H

#include "llrect.h"
#include "lluistring.h"
#include "llcombobox.h"

class LLScrollListColumn;
class LLResizeBar;
class LLScrollListCtrl;

class LLScrollColumnHeader : public LLComboBox
{
public:
	LLScrollColumnHeader(const std::string& label, const LLRect& rect, LLScrollListColumn* column, const LLFontGL* font = NULL);
	~LLScrollColumnHeader();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

	/*virtual*/ void showList() {}; // block the normal showList() behavior
	/*virtual*/ LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding);
	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);

	void setImage(const std::string& image_name);
	void setImageOverlay(const std::string& overlay_image, LLFontGL::HAlign alignment = LLFontGL::HCENTER, const LLColor4& color = LLColor4::white);
	LLScrollListColumn* getColumn() { return mColumn; }
	void setHasResizableElement(BOOL resizable);
	void updateResizeBars();
	BOOL canResize();
	void enableResizeBar(BOOL enable);
	std::string getLabel() { return mOrigLabel; }

	void onSelectSort();
	void onClick(const LLSD& data);

private:
	LLScrollListColumn* mColumn;
	LLResizeBar*		mResizeBar;
	std::string			mOrigLabel;
	LLUIString			mAscendingText;
	LLUIString			mDescendingText;
	BOOL				mHasResizableElement;

	LLPointer<LLUIImage>	mImageOverlay;
	LLFontGL::HAlign		mImageOverlayAlignment;
	LLColor4				mImageOverlayColor;
};

/*
 * A simple data class describing a column within a scroll list.
 */
class LLScrollListColumn
{
public:
	typedef enum e_sort_direction
	{
		DESCENDING,
		ASCENDING
	} ESortDirection;

	//NOTE: this is default constructible so we can store it in a map.
	LLScrollListColumn();
	LLScrollListColumn(const LLSD& sd, LLScrollListCtrl* parent = NULL);

	void setWidth(S32 width);
	S32 getWidth() const { return mWidth; }

public:
	// Public data is fine so long as this remains a simple struct-like data class.
	// If it ever gets any smarter than that, these should all become private
	// with protected or public accessor methods added as needed. -MG
	std::string				mName;
	std::string				mSortingColumn;
	ESortDirection			mSortDirection;
	LLUIString				mLabel;
	F32						mRelWidth;
	BOOL					mDynamicWidth;
	S32						mMaxContentWidth;
	S32						mIndex;
	LLScrollListCtrl*		mParentCtrl;
	LLScrollColumnHeader*	mHeader;
	LLFontGL::HAlign		mFontAlignment;

private:
	S32						mWidth;
};

#endif
