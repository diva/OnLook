/** 
 * @file llradiogroup.cpp
 * @brief LLRadioGroup base class
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

#include "llboost.h"

#include "llradiogroup.h"
#include "indra_constants.h"

#include "lluiimage.h"
#include "llviewborder.h"
#include "llcontrol.h"
#include "llui.h"
#include "llfocusmgr.h"

static LLRegisterWidget<LLRadioGroup> r("radio_group");

/*
 * A checkbox control with use_radio_style == true.
 */
class LLRadioCtrl : public LLCheckBoxCtrl
{
public:
	LLRadioCtrl(const std::string& name, const LLRect& rect, const std::string& label, const std::string& value = "", const LLFontGL* font = NULL, commit_callback_t commit_callback = NULL);
	/*virtual*/ ~LLRadioCtrl();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ void setValue(const LLSD& value);

	LLSD getPayload() { return mPayload; }
protected:
	friend class LLUICtrlFactory;

	LLSD mPayload;	// stores data that this item represents in the radio group
};

LLRadioGroup::LLRadioGroup(const std::string& name, const LLRect& rect,
			   S32 initial_index, commit_callback_t commit_callback,
			   bool border, bool allow_deselect) :
	LLUICtrl(name, rect, TRUE, commit_callback, FOLLOWS_LEFT | FOLLOWS_TOP),
	mSelectedIndex(initial_index),
	mAllowDeselect(allow_deselect)
{
	init(border);
}

void LLRadioGroup::init(bool border)
{
	if (border)
	{
		addChild( new LLViewBorder( std::string("radio group border"), 
					LLRect(0, getRect().getHeight(), getRect().getWidth(), 0), 
					LLViewBorder::BEVEL_NONE, LLViewBorder::STYLE_LINE, 1 ));
	}
	mHasBorder = border;
}


LLRadioGroup::~LLRadioGroup()
{
}

// virtual
BOOL LLRadioGroup::postBuild()
{
	if (!mRadioButtons.empty())
	{
		mRadioButtons[0]->setTabStop(true);
	}
	return TRUE;
}

// virtual
void LLRadioGroup::setEnabled(BOOL enabled)
{
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *child = *child_iter;
		child->setEnabled(enabled);
	}
	LLView::setEnabled(enabled);
}

void LLRadioGroup::setIndexEnabled(S32 index, BOOL enabled)
{
	S32 count = 0;
	for (button_list_t::iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* child = *iter;
		if (count == index)
		{
			child->setEnabled(enabled);
			if (index == mSelectedIndex && enabled == FALSE)
			{
				setSelectedIndex(-1);
			}
			break;
		}
		count++;
	}
	count = 0;
	if (mSelectedIndex < 0)
	{
		// Set to highest enabled value < index,
		// or lowest value above index if none lower are enabled
		// or 0 if none are enabled
		for (button_list_t::iterator iter = mRadioButtons.begin();
			 iter != mRadioButtons.end(); ++iter)
		{
			LLRadioCtrl* child = *iter;
			if (count >= index && mSelectedIndex >= 0)
			{
				break;
			}
			if (child->getEnabled())
			{
				setSelectedIndex(count);
			}
			count++;
		}
		if (mSelectedIndex < 0)
		{
			setSelectedIndex(0);
		}
	}
}

BOOL LLRadioGroup::setSelectedIndex(S32 index, BOOL from_event)
{
	if ((S32)mRadioButtons.size() <= index )
	{
		return FALSE;
	}

	if (mSelectedIndex >= 0)
	{
		LLRadioCtrl* old_radio_item = mRadioButtons[mSelectedIndex];
		old_radio_item->setTabStop(false);
		old_radio_item->setValue( FALSE );
	}
	else
	{
		mRadioButtons[0]->setTabStop(false);
	}

	mSelectedIndex = index;

	if (mSelectedIndex >= 0)
	{
		LLRadioCtrl* radio_item = mRadioButtons[mSelectedIndex];
		radio_item->setTabStop(true);
		radio_item->setValue( TRUE );

		if (hasFocus())
		{
			radio_item->focusFirstItem(FALSE, FALSE);
		}
	}

	if (!from_event)
	{
		setControlValue(getValue());
	}

	return TRUE;
}

BOOL LLRadioGroup::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	// do any of the tab buttons have keyboard focus?
	if (mask == MASK_NONE)
	{
		switch(key)
		{
		case KEY_DOWN:
			if (!setSelectedIndex((getSelectedIndex() + 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_UP:
			if (!setSelectedIndex((getSelectedIndex() - 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_LEFT:
			if (!setSelectedIndex((getSelectedIndex() - 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_RIGHT:
			if (!setSelectedIndex((getSelectedIndex() + 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		default:
			break;
		}
	}
	return handled;
}

BOOL LLRadioGroup::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// grab focus preemptively, before child button takes mousecapture
	//
	if (hasTabStop())
	{
		focusFirstItem(FALSE, FALSE);
	}

	return LLUICtrl::handleMouseDown(x, y, mask);
}

// When adding a button, we need to ensure that the radio
// group gets a message when the button is clicked.
LLRadioCtrl* LLRadioGroup::addRadioButton(const std::string& name, const std::string& label, const LLRect& rect, const LLFontGL* font, const std::string& payload)
{
	LLRadioCtrl* radio = new LLRadioCtrl(name, rect, label, payload, font, boost::bind(&LLRadioGroup::onClickButton, this, _1));
	addChild(radio);
	mRadioButtons.push_back(radio);
	return radio;
}

// Handle one button being clicked.  All child buttons must have this
// function as their callback function.

void LLRadioGroup::onClickButton(LLUICtrl* ctrl)
{
	// LL_INFOS() << "LLRadioGroup::onClickButton" << LL_ENDL;
	LLRadioCtrl* clicked_radio = dynamic_cast<LLRadioCtrl*>(ctrl);
	if (!clicked_radio)
	    return;
	S32 index = 0;
	for (button_list_t::iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		if (radio == clicked_radio)
		{
			if (index == mSelectedIndex && mAllowDeselect)
			{
				// don't select anything
				setSelectedIndex(-1);
			}
			else
			{
				setSelectedIndex(index);
			}
			
			// BUG: Calls click callback even if button didn't actually change
			onCommit();

			return;
		}

		index++;
	}

	llwarns << "LLRadioGroup::onClickButton - clicked button that isn't a child" << llendl;
}

void LLRadioGroup::setValue( const LLSD& value )
{
	int idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		if (radio->getPayload().asString() == value.asString())
		{
			setSelectedIndex(idx);
			idx = -1;
			break;
		}
		++idx;
	}
	if (idx != -1)
	{
		// string not found, try integer
		if (value.isInteger())
		{
			setSelectedIndex((S32) value.asInteger(), TRUE);
		}
		else
		{
			setSelectedIndex(-1, TRUE);
		}
	}
}

LLSD LLRadioGroup::getValue() const
{
	int index = getSelectedIndex();
	int idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if (idx == index) return LLSD((*iter)->getPayload());
		++idx;
	}
	return LLSD();
}

// virtual
LLXMLNodePtr LLRadioGroup::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->setName(LL_RADIO_GROUP_TAG);

	// Attributes

	node->createChild("draw_border", TRUE)->setBoolValue(mHasBorder);
	node->createChild("allow_deselect", TRUE)->setBoolValue(mAllowDeselect);

	// Contents

	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;

		LLXMLNodePtr child_node = radio->getXML();

		node->addChild(child_node);
	}

	return node;
}

// static
LLView* LLRadioGroup::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	U32 initial_value = 0;
	node->getAttributeU32("initial_value", initial_value);

	bool draw_border = true;
	node->getAttribute_bool("draw_border", draw_border);

	bool allow_deselect = false;
	node->getAttribute_bool("allow_deselect", allow_deselect);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLRadioGroup* radio_group = new LLRadioGroup("radio_group",
		rect,
		initial_value,
		NULL,
		draw_border,
		allow_deselect);

	const std::string& contents = node->getValue();

	LLRect group_rect = radio_group->getRect();

	LLFontGL *font = LLView::selectFont(node);

	if (contents.find_first_not_of(" \n\t") != contents.npos)
	{
		// ...old school default vertical layout
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("\t\n");
		tokenizer tokens(contents, sep);
		tokenizer::iterator token_iter = tokens.begin();
	
		const S32 HPAD = 4, VPAD = 4;
		S32 cur_y = group_rect.getHeight() - VPAD;
	
		while(token_iter != tokens.end())
		{
			const std::string& line = *token_iter;
			LLRect rect(HPAD, cur_y, group_rect.getWidth() - (2 * HPAD), cur_y - 15);
			cur_y -= VPAD + 15;
			radio_group->addRadioButton(std::string("radio"), line, rect, font);
			++token_iter;
		}
		llwarns << "Legacy radio group format used! Please convert to use <radio_item> tags!" << llendl;
	}
	else
	{
		// ...per pixel layout
		LLXMLNodePtr child;
		for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
		{
			if (child->hasName("radio_item"))
			{
				LLRect item_rect;
				createRect(child, item_rect, radio_group, rect);

				std::string item_label = child->getTextContents();
				child->getAttributeString("label", item_label);
				std::string item_name("radio");
				child->getAttributeString("name", item_name);
				std::string payload(item_name); // Support old-style name as payload
				child->getAttributeString("value", payload);
				child->getAttributeString("initial_value", payload); // Synonym
				LLRadioCtrl* radio = radio_group->addRadioButton(item_name, item_label, item_rect, font, payload);

				radio->initFromXML(child, radio_group);
			}
		}
	}

	radio_group->initFromXML(node, parent);

	return radio_group;
}

// LLCtrlSelectionInterface functions
BOOL	LLRadioGroup::setCurrentByID( const LLUUID& id )
{
	return FALSE;
}

LLUUID	LLRadioGroup::getCurrentID() const
{
	return LLUUID::null;
}

BOOL	LLRadioGroup::setSelectedByValue(const LLSD& value, BOOL selected)
{
	S32 idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if((*iter)->getPayload().asString() == value.asString())
		{
			setSelectedIndex(idx);
			return TRUE;
		}
		idx++;
	}

	return FALSE;
}

LLSD	LLRadioGroup::getSelectedValue()
{
	return getValue();	
}

BOOL	LLRadioGroup::isSelected(const LLSD& value) const
{
	S32 idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if((*iter)->getPayload().asString() == value.asString())
		{
			if (idx == mSelectedIndex) 
			{
				return TRUE;
			}
		}
		idx++;
	}
	return FALSE;
}

BOOL	LLRadioGroup::operateOnSelection(EOperation op)
{
	return FALSE;
}

BOOL	LLRadioGroup::operateOnAll(EOperation op)
{
	return FALSE;
}

LLRadioCtrl::LLRadioCtrl(const std::string& name, const LLRect& rect, const std::string& label, const std::string& value, const LLFontGL* font, commit_callback_t commit_callback)
:	LLCheckBoxCtrl(name, rect, label, font, commit_callback, FALSE, RADIO_STYLE),
	mPayload(value)
{
	setTabStop(FALSE);
	// use name as default "Value" for backwards compatibility
	if (value.empty())
	{
		mPayload = name;
	}
}

LLRadioCtrl::~LLRadioCtrl()
{
}

void LLRadioCtrl::setValue(const LLSD& value)
{
	LLCheckBoxCtrl::setValue(value);
	mButton->setTabStop(value.asBoolean());
}

// virtual
LLXMLNodePtr LLRadioCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLCheckBoxCtrl::getXML();

	node->setName(LL_RADIO_ITEM_TAG);

	node->createChild("value", TRUE)->setStringValue(mPayload);

	return node;
}
