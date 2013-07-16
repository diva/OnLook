/**
 * @file llscrolllistcell.cpp
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

#include "llscrolllistcell.h"

#include "llcheckboxctrl.h"


LLScrollListCell::LLScrollListCell(S32 width)
:	mWidth(width),
	mToolTip()
{}

// virtual
const LLSD LLScrollListCell::getValue() const
{
	return LLStringUtil::null;
}

//
// LLScrollListIcon
//
LLScrollListIcon::LLScrollListIcon(const LLSD& value, S32 width)
:	LLScrollListCell(width),
	// <edit>
	mCallback(NULL),
	// </edit>
	mColor(LLColor4::white)
{
	setValue(value);
}

LLScrollListIcon::~LLScrollListIcon()
{
}

/*virtual*/
S32		LLScrollListIcon::getHeight() const
{ return mIcon ? mIcon->getHeight() : 0; }

/*virtual*/
const LLSD		LLScrollListIcon::getValue() const
{ return mIcon.isNull() ? LLStringUtil::null : mIcon->getName(); }

void LLScrollListIcon::setValue(const LLSD& value)
{
	if (value.isUUID())
	{
		// don't use default image specified by LLUUID::null, use no image in that case
		LLUUID image_id = value.asUUID();
		mIcon = image_id.notNull() ? LLUI::getUIImageByID(image_id) : LLUIImagePtr(NULL);
	}
	else
	{
		std::string value_string = value.asString();
		if (LLUUID::validate(value_string))
		{
			setValue(LLUUID(value_string));
		}
		else if (!value_string.empty())
		{
			mIcon = LLUI::getUIImage(value.asString());
		}
		else
		{
			mIcon = NULL;
		}
	}
}


void LLScrollListIcon::setColor(const LLColor4& color)
{
	mColor = color;
}

S32	LLScrollListIcon::getWidth() const
{
	// if no specified fix width, use width of icon
	if (LLScrollListCell::getWidth() == 0 && mIcon.notNull())
	{
		return mIcon->getWidth();
	}
	return LLScrollListCell::getWidth();
}


void LLScrollListIcon::draw(const LLColor4& color, const LLColor4& highlight_color)	 const
{
	if (mIcon)
	{
		mIcon->draw(0, 0, mColor);
	}
}

//
// LLScrollListText
//
U32 LLScrollListText::sCount = 0;

LLScrollListText::LLScrollListText(const std::string& text, const LLFontGL* font, S32 width, U8 font_style, LLFontGL::HAlign font_alignment, LLColor4& color, BOOL use_color, BOOL visible)
:	LLScrollListCell(width),
	mText(text),
	mFont(font),
	mColor(color),
	mUseColor(use_color),
	mFontStyle(font_style),
	mFontAlignment(font_alignment),
	mVisible(visible),
	mHighlightCount( 0 ),
	mHighlightOffset( 0 )
{
	sCount++;

	mTextWidth = getWidth();

	// initialize rounded rect image
	if (!mRoundedRectImage)
	{
		mRoundedRectImage = LLUI::getUIImage("rounded_square.tga");
	}
}

//virtual
void LLScrollListText::highlightText(S32 offset, S32 num_chars)
{
	mHighlightOffset = offset;
	mHighlightCount = num_chars;
}

//virtual
BOOL LLScrollListText::isText() const
{
	return TRUE;
}

//virtual
const std::string &LLScrollListText::getToolTip() const
{
	// If base class has a tooltip, return that
	if (! LLScrollListCell::getToolTip().empty())
		return LLScrollListCell::getToolTip();

	// ...otherwise, return the value itself as the tooltip
	return mText.getString();
}

// virtual
BOOL LLScrollListText::needsToolTip() const
{
	// If base class has a tooltip, return that
	if (LLScrollListCell::needsToolTip())
		return LLScrollListCell::needsToolTip();

	// ...otherwise, show tooltips for truncated text
	return mFont->getWidth(mText.getString()) > getWidth();
}

//virtual
BOOL LLScrollListText::getVisible() const
{
	return mVisible;
}

//virtual
S32 LLScrollListText::getHeight() const
{
	return llround(mFont->getLineHeight());
}


LLScrollListText::~LLScrollListText()
{
	sCount--;
}

S32	LLScrollListText::getContentWidth() const
{
	return mFont->getWidth(mText.getString());
}


void LLScrollListText::setColor(const LLColor4& color)
{
	mColor = color;
	mUseColor = TRUE;
}

void LLScrollListText::setText(const LLStringExplicit& text)
{
	mText = text;
}

void LLScrollListText::setFontStyle(const U8 font_style)
{
	mFontStyle = font_style;
}

//virtual
void LLScrollListText::setValue(const LLSD& text)
{
	setText(text.asString());
}

//virtual
const LLSD LLScrollListText::getValue() const
{
	return LLSD(mText.getString());
}


void LLScrollListText::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	LLColor4 display_color;
	if (mUseColor)
	{
		display_color = mColor;
	}
	else
	{
		display_color = color;
	}

	if (mHighlightCount > 0)
	{
		S32 left = 0;
		switch(mFontAlignment)
		{
		case LLFontGL::LEFT:
			left = mFont->getWidth(mText.getString(), 0, mHighlightOffset);
			break;
		case LLFontGL::RIGHT:
			left = getWidth() - mFont->getWidth(mText.getString(), mHighlightOffset, S32_MAX);
			break;
		case LLFontGL::HCENTER:
			left = (getWidth() - mFont->getWidth(mText.getString())) / 2;
			break;
		}
		LLRect highlight_rect(left - 2,
				llround(mFont->getLineHeight()) + 1,
				left + mFont->getWidth(mText.getString(), mHighlightOffset, mHighlightCount) + 1,
				1);
		mRoundedRectImage->draw(highlight_rect, highlight_color);
	}

	// Try to draw the entire string
	F32 right_x;
	U32 string_chars = mText.length();
	F32 start_x = 0.f;
	switch(mFontAlignment)
	{
	case LLFontGL::LEFT:
		start_x = (mFontStyle & LLFontGL::ITALIC) ? 2.f : 0.f; //Italic text seems to need a little padding.
		break;
	case LLFontGL::RIGHT:
		start_x = (F32)getWidth();
		break;
	case LLFontGL::HCENTER:
		start_x = (F32)getWidth() * 0.5f;
		break;
	}
	mFont->render(mText.getWString(), 0,
					start_x, 2.f,
					display_color,
					mFontAlignment,
					LLFontGL::BOTTOM,
					mFontStyle,
					LLFontGL::NO_SHADOW,
					string_chars,
					getTextWidth(),
					&right_x,
					FALSE,
					TRUE);
}

//
// LLScrollListCheck
//
LLScrollListCheck::LLScrollListCheck(LLCheckBoxCtrl* check_box, S32 width)
:	LLScrollListCell(width)
{
	mCheckBox = check_box;

	LLRect rect(mCheckBox->getRect());
	if (width)
	{
		rect.mRight = rect.mLeft + width;
		mCheckBox->setRect(rect);
		setWidth(width);
	}
	else
	{
		setWidth(rect.getWidth()); //check_box->getWidth();
	}
}


LLScrollListCheck::~LLScrollListCheck()
{
	delete mCheckBox;
	mCheckBox = NULL;
}

void LLScrollListCheck::draw(const LLColor4& color, const LLColor4& highlight_color) const
{
	mCheckBox->draw();
}

BOOL LLScrollListCheck::handleClick()
{
	if (mCheckBox->getEnabled())
	{
		mCheckBox->toggle();
	}
	// don't change selection when clicking on embedded checkbox
	return TRUE;
}

/*virtual*/
const LLSD LLScrollListCheck::getValue() const
{
	return mCheckBox->getValue();
}

/*virtual*/
void LLScrollListCheck::setValue(const LLSD& value)
{
	mCheckBox->setValue(value);
}

/*virtual*/
void LLScrollListCheck::onCommit()
{
	mCheckBox->onCommit();
}

/*virtual*/
void LLScrollListCheck::setEnabled(BOOL enable)
{
	mCheckBox->setEnabled(enable);
}

//
// LLScrollListDate
//

LLScrollListDate::LLScrollListDate( const LLDate& date, const LLFontGL* font, S32 width, U8 font_style, LLFontGL::HAlign font_alignment, LLColor4& color, BOOL use_color, BOOL visible)
:	LLScrollListText(date.asRFC1123(), font, width, font_style, font_alignment, color, use_color, visible),
	mDate(date)
{}

void LLScrollListDate::setValue(const LLSD& value)
{
	mDate = value.asDate();
	LLScrollListText::setValue(mDate.asRFC1123());
}

const LLSD LLScrollListDate::getValue() const
{
	return mDate;
}
