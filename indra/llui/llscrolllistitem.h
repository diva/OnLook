/**
 * @file llscrolllistitem.h
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

#ifndef LLSCROLLLISTITEM_H
#define LLSCROLLLISTITEM_H

#include "llrefcount.h"
#include "llhandle.h"
#include "llsd.h"
#include "llscrolllistcell.h"

//---------------------------------------------------------------------------
// LLScrollListItem
//---------------------------------------------------------------------------
class LLScrollListItem
 : public LLHandleProvider<LLScrollListItem> // Singu TODO: Break out into LLNameListItem
{
	friend class LLScrollListCtrl;
public:

	virtual ~LLScrollListItem();

	void	setSelected( BOOL b )			{ mSelected = b; }
	BOOL	getSelected() const				{ return mSelected; }

	void	setEnabled( BOOL b )			{ mEnabled = b; }
	BOOL	getEnabled() const 				{ return mEnabled; }

	void	setUserdata( void* userdata )	{ mUserdata = userdata; }
	void*	getUserdata() const 			{ return mUserdata; }

	void setToolTip(const std::string tool_tip) { mToolTip=tool_tip; }
	std::string getToolTip() 				{ return mToolTip; }

	virtual LLUUID	getUUID() const			{ return mItemValue.asUUID(); }
	LLSD	getValue() const				{ return mItemValue; }

	void	setRect(LLRect rect)			{ mRectangle = rect; }
	LLRect	getRect() const					{ return mRectangle; }

	void	addColumn( const std::string& text, const LLFontGL* font, S32 width = 0, U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, bool visible = true );

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );

	S32		getNumColumns() const;

	LLScrollListCell *getColumn(const S32 i) const;

	std::string getContentsCSV() const;

	virtual void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);

protected:
	LLScrollListItem( bool enabled = true, const LLSD& value = LLSD(), void* userdata = NULL );

private:
	BOOL	mSelected;
	BOOL	mEnabled;
	void*	mUserdata;
	LLSD	mItemValue;
	std::string mToolTip;
	std::vector<LLScrollListCell *> mColumns;
	LLRect  mRectangle;
};

#endif
