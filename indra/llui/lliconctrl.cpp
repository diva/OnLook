/** 
 * @file lliconctrl.cpp
 * @brief LLIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "linden_common.h"

#include "lliconctrl.h"

// Linden library includes 

// Project includes
#include "llcontrol.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"

static LLRegisterWidget<LLIconCtrl> r("icon");

LLIconCtrl::Params::Params()
:	image("image_name"),
	color("color"),
//	use_draw_context_alpha("use_draw_context_alpha", true),
	scale_image("scale_image"),
	min_width("min_width", 0),
	min_height("min_height", 0)
{}

LLIconCtrl::LLIconCtrl(const LLIconCtrl::Params& p)
:	LLUICtrl(p),
	mColor(p.color()),
	mImagep(p.image),
//	mUseDrawContextAlpha(p.use_draw_context_alpha),
	mPriority(0),
	mMinWidth(p.min_width),
	mMinHeight(p.min_height)
{
	if (mImagep.notNull())
	{
		LLUICtrl::setValue(mImagep->getName());
	}
	setTabStop(false);
}

LLIconCtrl::LLIconCtrl(const std::string& name, const LLRect &rect, const std::string &image_name, const S32& min_width, const S32& min_height)
:	LLUICtrl(name, 
			 rect, 
			 FALSE, // mouse opaque
			 NULL,
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mColor( LLColor4::white ),
	mPriority(0),
	mMinWidth(min_width),
	mMinHeight(min_height)
{
	setValue( image_name );
	setTabStop(FALSE);
}

LLIconCtrl::~LLIconCtrl()
{
	mImagep = NULL;
}


void LLIconCtrl::draw()
{
	if( mImagep.notNull() )
	{
		const F32 alpha = getDrawContext().mAlpha;
		mImagep->draw(getLocalRect(), mColor % alpha );
	}

	LLUICtrl::draw();
}

// virtual 
void LLIconCtrl::setAlpha(F32 alpha)
{
	mColor.setAlpha(alpha);
}

// virtual
// value might be a string or a UUID
void LLIconCtrl::setValue(const LLSD& value )
{
	LLSD tvalue(value);
	if (value.isString() && LLUUID::validate(value.asString()))
	{
		//RN: support UUIDs masquerading as strings
		tvalue = LLSD(LLUUID(value.asString()));
	}
	LLUICtrl::setValue(tvalue);
	if (tvalue.isUUID())
	{
		mImagep = LLUI::getUIImageByID(tvalue.asUUID(), mPriority);
	}
	else
	{
		mImagep = LLUI::getUIImage(tvalue.asString(), mPriority);
	}

	if (mImagep.notNull()
		&& mImagep->getImage().notNull()
		&& mMinWidth
		&& mMinHeight)
	{
		mImagep->getImage()->setKnownDrawSize(llmax(mMinWidth, mImagep->getWidth()), llmax(mMinHeight, mImagep->getHeight()));
	}
}

std::string LLIconCtrl::getImageName() const
{
	if (getValue().isString())
		return getValue().asString();
	else
		return std::string();
}

// virtual
LLXMLNodePtr LLIconCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->setName(LL_ICON_CTRL_TAG);

	if (!getImageName().empty())
	{
		node->createChild("image_name", TRUE)->setStringValue(getImageName());
	}

	if (mMinWidth) node->createChild("min_width", true)->setIntValue(mMinWidth);
	if (mMinHeight) node->createChild("min_height", true)->setIntValue(mMinHeight);

	node->createChild("color", TRUE)->setFloatValue(4, mColor.mV);

	return node;
}

LLView* LLIconCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLRect rect;
	createRect(node, rect, parent, LLRect());

	std::string image_name;
	if (node->hasAttribute("image_name"))
	{
		node->getAttributeString("image_name", image_name);
	}

	LLColor4 color(LLColor4::white);
	LLUICtrlFactory::getAttributeColor(node,"color", color);

	S32 min_width = 0, min_height = 0;
	if (node->hasAttribute("min_width"))
	{
		node->getAttributeS32("min_width", min_width);
	}

	if (node->hasAttribute("min_height"))
	{
		node->getAttributeS32("min_height", min_height);
	}

	LLIconCtrl* icon = new LLIconCtrl("icon", rect, image_name, min_width, min_height);

	icon->setColor(color);

	icon->initFromXML(node, parent);

	return icon;
}
