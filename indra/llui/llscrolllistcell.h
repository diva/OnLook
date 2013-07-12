/**
 * @file llscrolllistcell.h
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

#ifndef LLSCROLLLISTCELL_H
#define LLSCROLLLISTCELL_H

#include "llfontgl.h"		// HAlign
#include "llpointer.h"		// LLPointer<>
#include "lluistring.h"
#include "v4color.h"
#include "llui.h"

class LLCheckBoxCtrl;
class LLSD;

/*
 * Represents a cell in a scrollable table.
 *
 * Sub-classes must return height and other properties
 * though width accessors are implemented by the base class.
 * It is therefore important for sub-class constructors to call
 * setWidth() with realistic values.
 */
class LLScrollListCell
{
public:
	LLScrollListCell(S32 width = 0);
	virtual ~LLScrollListCell() {};

	virtual void			draw(const LLColor4& color, const LLColor4& highlight_color) const {};		// truncate to given width, if possible
	virtual S32				getWidth() const {return mWidth;}
	virtual S32				getContentWidth() const { return 0; }
	virtual S32				getHeight() const { return 0; }
	virtual const LLSD		getValue() const;
	virtual void			setValue(const LLSD& value) { }
	virtual BOOL			getVisible() const { return TRUE; }
	virtual void			setWidth(S32 width) { mWidth = width; }
	virtual void			highlightText(S32 offset, S32 num_chars) {}
	virtual BOOL			isText() const { return FALSE; }
	virtual void			setColor(const LLColor4&) {}
	virtual void			onCommit() {};

	virtual BOOL			handleClick() { return FALSE; }
	virtual	void			setEnabled(BOOL enable) { }

private:
	S32 mWidth;
};

/*
 * Cell displaying a text label.
 */
class LLScrollListText : public LLScrollListCell
{
public:
	LLScrollListText(const std::string& text, const LLFontGL* font, S32 width = 0, U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, LLColor4& color = LLColor4::black, BOOL use_color = FALSE, BOOL visible = TRUE);
	/*virtual*/ ~LLScrollListText();

	/*virtual*/ void    draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getContentWidth() const;
	/*virtual*/ S32		getHeight() const;
	/*virtual*/ void	setValue(const LLSD& value);
	/*virtual*/ const LLSD getValue() const;
	/*virtual*/ BOOL	getVisible() const;
	/*virtual*/ void	highlightText(S32 offset, S32 num_chars);

	/*virtual*/ void	setColor(const LLColor4&);
	/*virtual*/ BOOL	isText() const;

	S32				getTextWidth() const { return mTextWidth;}
	void			setTextWidth(S32 value) { mTextWidth = value;}
	virtual void	setWidth(S32 width) { LLScrollListCell::setWidth(width); mTextWidth = width; }

	void			setText(const LLStringExplicit& text);
	void			setFontStyle(const U8 font_style);

private:
	LLUIString		mText;
	S32				mTextWidth;
	const LLFontGL*	mFont;
	LLColor4		mColor;
	U8				mUseColor;
	U8				mFontStyle;
	LLFontGL::HAlign mFontAlignment;
	BOOL			mVisible;
	S32				mHighlightCount;
	S32				mHighlightOffset;

	LLPointer<LLUIImage> mRoundedRectImage;

	static U32 sCount;
};

/*
 * Cell displaying an image.
 */
class LLScrollListIcon : public LLScrollListCell
{
public:
	LLScrollListIcon( LLUIImagePtr icon, S32 width = 0);
	LLScrollListIcon(const LLSD& value, S32 width = 0);
	/*virtual*/ ~LLScrollListIcon();
	/*virtual*/ void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getWidth() const;
	/*virtual*/ S32		getHeight() const;
	/*virtual*/ const LLSD		getValue() const;
	/*virtual*/ void	setColor(const LLColor4&);
	/*virtual*/ void	setValue(const LLSD& value);
	// <edit>
	void setClickCallback(boost::function<bool (void)> cb) { mCallback = cb; }
	/*virtual*/ BOOL handleClick() { return mCallback ? mCallback() : false; }
	// </edit>

private:
	LLPointer<LLUIImage>	mIcon;
	LLColor4				mColor;
	// <edit>
	boost::function<bool (void)> mCallback;
	// </edit>
};

/*
 * An interactive cell containing a check box.
 */
class LLScrollListCheck : public LLScrollListCell
{
public:
	LLScrollListCheck( LLCheckBoxCtrl* check_box, S32 width = 0);
	/*virtual*/ ~LLScrollListCheck();
	/*virtual*/ void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	/*virtual*/ S32		getHeight() const			{ return 0; }
	/*virtual*/ const LLSD	getValue() const;
	/*virtual*/ void	setValue(const LLSD& value);
	/*virtual*/ void	onCommit();

	/*virtual*/ BOOL	handleClick();
	/*virtual*/ void	setEnabled(BOOL enable);

	LLCheckBoxCtrl*	getCheckBox()				{ return mCheckBox; }

private:
	LLCheckBoxCtrl* mCheckBox;
};

class LLScrollListDate : public LLScrollListText
{
public:
	LLScrollListDate( const LLDate& date, const LLFontGL* font, S32 width=0, U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, LLColor4& color = LLColor4::black, BOOL use_color = FALSE, BOOL visible = TRUE);
	virtual void	setValue(const LLSD& value);
	virtual const LLSD getValue() const;

private:
	LLDate		mDate;
};

#endif
