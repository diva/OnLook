/** 
 * @file llnameeditor.cpp
 * @brief Name Editor to refresh a name.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
 
#include "llnameeditor.h"
#include "llcachename.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llagent.h"

#include "llfontgl.h"

#include "lluuid.h"
#include "llrect.h"
#include "llstring.h"
#include "llui.h"

static LLRegisterWidget<LLNameEditor> r("name_editor");

// statics
std::set<LLNameEditor*> LLNameEditor::sInstances;

LLNameEditor::LLNameEditor(const std::string& name, const LLRect& rect,
		const LLUUID& name_id, 
		BOOL is_group,
		const LLFontGL* glfont,
		S32 max_text_length,
		void (*commit_callback)(LLUICtrl* caller, void* user_data),
		void (*keystroke_callback)(LLLineEditor* caller, void* user_data),
		void (*focus_lost_callback)(LLFocusableElement* caller, void* user_data),
		void* userdata,
		LLLinePrevalidateFunc prevalidate_func)
:	LLLineEditor(name, rect, 
				 std::string("(retrieving)"), 
				 glfont, 
				 max_text_length, 
				 commit_callback, 
				 keystroke_callback,
				 focus_lost_callback,
				 userdata,
				 prevalidate_func),
	mNameID(name_id)
{
	LLNameEditor::sInstances.insert(this);
	if(!name_id.isNull())
	{
		setNameID(name_id, is_group);
	}
}


LLNameEditor::~LLNameEditor()
{
	LLNameEditor::sInstances.erase(this);
}

void LLNameEditor::on_avatar_name_response(const LLUUID& agent_id, const LLAvatarName& av_name, void *userdata){
	LLNameEditor* self = (LLNameEditor*)userdata;
	if(self->mNameID == agent_id) self->setText(av_name.getCompleteName());
}

void LLNameEditor::setNameID(const LLUUID& name_id, BOOL is_group)
{
	mNameID = name_id;

	std::string name;

	if (!is_group)
	{
		/* Phoenix: Wolfspirit: Check if we already have the name in Cache.
			This will (if DN is enabled) return the full name (Displayname (username)).
			If DN is diabled it will return the old "Firstname Lastname" style.
			If it is not in cache, then add the LegacyName until we received the name from the callback.
			Do the Request only, if DN is enabled. */

		LLAvatarName av_name;
		if(LLAvatarNameCache::get(name_id, &av_name)){
			name = av_name.getCompleteName();
		}
		else
		{
			gCacheName->getFullName(name_id,name);
			if(LLAvatarNameCache::useDisplayNames()) LLAvatarNameCache::get(name_id, boost::bind(&LLNameEditor::on_avatar_name_response, _1, _2, this));
		}
	}
	else
	{
		gCacheName->getGroupName(name_id, name);
	}

	setText(name);
}

void LLNameEditor::refresh(const LLUUID& id, const std::string& firstname,
						   const std::string& lastname, BOOL is_group)
{
	if (id == mNameID)
	{
		std::string name;
		if (!is_group)
		{
			/* Phoenix: Wolfspirit: Use DN Cache first */
			LLAvatarName av_name;
			if(LLAvatarNameCache::get(id, &av_name)){
				name = av_name.getCompleteName();
			}
			else
			{
				gCacheName->getFullName(id,name);
				if(LLAvatarNameCache::useDisplayNames()) LLAvatarNameCache::get(id, boost::bind(&LLNameEditor::on_avatar_name_response, _1, _2, this));
			}
		}
		else
		{
			name = firstname;
		}
		setText(name);
	}
}

void LLNameEditor::refreshAll(const LLUUID& id, const std::string& firstname,
							  const std::string& lastname, BOOL is_group)
{
	std::set<LLNameEditor*>::iterator it;
	for (it = LLNameEditor::sInstances.begin();
		 it != LLNameEditor::sInstances.end();
		 ++it)
	{
		LLNameEditor* box = *it;
		box->refresh(id, firstname, lastname, is_group);
	}
}

void LLNameEditor::setValue( const LLSD& value )
{
	setNameID(value.asUUID(), FALSE);
}

LLSD LLNameEditor::getValue() const
{
	return LLSD(mNameID);
}

// virtual
LLXMLNodePtr LLNameEditor::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLLineEditor::getXML();

	node->setName(LL_NAME_EDITOR_TAG);

	return node;
}

LLView* LLNameEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("name_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	S32 max_text_length = 128;
	node->getAttributeS32("max_length", max_text_length);

	LLFontGL* font = LLView::selectFont(node);

	LLUICtrlCallback commit_callback = NULL;

	LLNameEditor* line_editor = new LLNameEditor(name,
								rect, 
								LLUUID::null, FALSE,
								font,
								max_text_length,
								commit_callback);

	std::string label;
	if(node->getAttributeString("label", label))
	{
		line_editor->setLabel(label);
	}
	line_editor->setColorParameters(node);
	line_editor->initFromXML(node, parent);
	
	return line_editor;
}
