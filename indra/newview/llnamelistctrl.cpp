/** 
 * @file llnamelistctrl.cpp
 * @brief A list of names, automatically refreshed from name cache.
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

#include "llnamelistctrl.h"

#include <boost/tokenizer.hpp>

#include "llavatarnamecache.h"

#include "llcachename.h"
#include "llagent.h"
#include "llinventory.h"
#include "lltrans.h"

static LLRegisterWidget<LLNameListCtrl> r("name_list");

// statics
std::set<LLNameListCtrl*> LLNameListCtrl::sInstances;

LLNameListCtrl::LLNameListCtrl(const std::string& name,
							   const LLRect& rect,
							   LLUICtrlCallback cb,
							   void* userdata,
							   BOOL allow_multiple_selection,
							   BOOL draw_border,
							   bool draw_heading,
							   S32 name_column_index,
							   const std::string& tooltip)
:	LLScrollListCtrl(name, rect, cb, userdata, allow_multiple_selection,
					 draw_border,draw_heading),
	mNameColumnIndex(name_column_index),
	mAllowCallingCardDrop(FALSE),
	mShortNames(FALSE)
{
	setToolTip(tooltip);
	LLNameListCtrl::sInstances.insert(this);
}


// virtual
LLNameListCtrl::~LLNameListCtrl()
{
	LLNameListCtrl::sInstances.erase(this);
}


// public
LLScrollListItem* LLNameListCtrl::addNameItem(const LLUUID& agent_id, EAddPosition pos,
								 BOOL enabled, const std::string& suffix)
{
	//llinfos << "LLNameListCtrl::addNameItem " << agent_id << llendl;

	LLSD item;
	item["id"] = agent_id;
	item["enabled"] = enabled;
	item["target"] = INDIVIDUAL;
	item["suffix"] = suffix;
	LLSD& column = item["columns"][0];
	column["value"] = "";
	column["font"] = "SANSSERIF";
	column["column"] = "name";
	
	return addNameItemRow(item, pos);
}

// virtual, public
BOOL LLNameListCtrl::handleDragAndDrop( 
		S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type, void *cargo_data, 
		EAcceptance *accept,
		std::string& tooltip_msg)
{
	if (!mAllowCallingCardDrop)
	{
		return FALSE;
	}

	BOOL handled = FALSE;

	if (cargo_type == DAD_CALLINGCARD)
	{
		if (drop)
		{
			LLInventoryItem* item = (LLInventoryItem *)cargo_data;
			addNameItem(item->getCreatorUUID());
		}

		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
		if (tooltip_msg.empty())
		{
			if (!getToolTip().empty())
			{
				tooltip_msg = getToolTip();
			}
			else
			{
				// backwards compatable English tooltip (should be overridden in xml)
				tooltip_msg.assign("Drag a calling card here\nto add a resident.");
			}
		}
	}

	handled = TRUE;
	lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLNameListCtrl " << getName() << llendl;

	return handled;
}


// public
void LLNameListCtrl::addGroupNameItem(const LLUUID& group_id, EAddPosition pos,
									  BOOL enabled)
{
	LLSD item;
	item["id"] = group_id;
	item["enabled"] = enabled;
	item["target"] = GROUP;
	LLSD& column = item["columns"][0];
	column["value"] = "";
	column["font"] = "SANSSERIF";
	column["column"] = "name";

	addNameItemRow(item, pos);
}

// public
void LLNameListCtrl::addGroupNameItem(LLSD& item, EAddPosition pos)
{
	item["target"] = GROUP;
	addNameItemRow(item, pos);
}

LLScrollListItem* LLNameListCtrl::addNameItem(LLSD& item, EAddPosition pos)
{
	item["target"] = INDIVIDUAL;
	return addNameItemRow(item, pos);
}

LLScrollListItem* LLNameListCtrl::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	return addNameItemRow(value, pos, userdata);
}
LLScrollListItem* LLNameListCtrl::addNameItemRow(const LLSD& value, EAddPosition pos, void* userdata)
{

	LLScrollListItem* item = LLScrollListCtrl::addElement(value, pos, userdata);
	if (!item) return NULL;

	LLUUID id = item->getUUID();

	// use supplied name by default
	std::string fullname = value["name"].asString();
	switch(value["target"].asInteger())
	{
	case GROUP:
		gCacheName->getGroupName(id, fullname);
		// fullname will be "nobody" if group not found
		break;
	case SPECIAL:
		// just use supplied name
		break;
	case INDIVIDUAL:
	{
		LLAvatarName av_name;
		if (id.isNull())
		{
			fullname = LLTrans::getString("AvatarNameNobody");
		}
		else if (LLAvatarNameCache::get(id, &av_name))
		{
			if (mShortNames)
				fullname = av_name.mDisplayName;
			else
				fullname = av_name.getCompleteName();
		}
		else
		{
			fullname = " ( " + LLTrans::getString("LoadingData") + " ) ";
			// ...schedule a callback
			LLAvatarNameCache::get(id,
				boost::bind(&LLNameListCtrl::onAvatarNameCache,
					this, _1, _2, item->getHandle()));
		}
			break;
		}
	default:
		break;
	}
	
	// Append optional suffix.
	std::string suffix = value["suffix"];
	if(!suffix.empty())
	{
		fullname.append(suffix);
	}

	LLScrollListCell* cell = item->getColumn(mNameColumnIndex);
	if (cell && !fullname.empty() && cell->getValue().asString().empty())
	{
		cell->setValue(fullname);
	}

	dirtyColumns();

	// this column is resizable
	LLScrollListColumn* columnp = getColumn(mNameColumnIndex);
	if (columnp && columnp->mHeader)
	{
		columnp->mHeader->setHasResizableElement(TRUE);
	}

	return item;
}

// public
void LLNameListCtrl::removeNameItem(const LLUUID& agent_id)
{
	// Find the item specified with agent_id.
	S32 idx = -1;
	for (item_list::iterator it = getItemList().begin(); it != getItemList().end(); it++)
	{
		LLScrollListItem* item = *it;
		if (item->getUUID() == agent_id)
		{
			idx = getItemIndex(item);
			break;
		}
	}

	// Remove it.
	if (idx >= 0)
	{
		selectNthItem(idx); // not sure whether this is needed, taken from previous implementation
		deleteSingleItem(idx);
	}
}

void LLNameListCtrl::onAvatarNameCache(const LLUUID& agent_id,
									   const LLAvatarName& av_name,
									   LLHandle<LLScrollListItem> item)
{
	std::string name;
	if (mShortNames)
		name = av_name.mDisplayName;
	else
		name = av_name.getCompleteName();

	LLScrollListItem* list_item = item.get();
	if (list_item && list_item->getUUID() == agent_id)
	{
		LLScrollListCell* cell = list_item->getColumn(mNameColumnIndex);
		if (cell)
		{
			cell->setValue(name);
			setNeedsSort();
		}
	}

	dirtyColumns();
}

void LLNameListCtrl::sortByName(BOOL ascending)
{
	sortByColumnIndex(mNameColumnIndex,ascending);
}

// virtual
LLXMLNodePtr LLNameListCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLScrollListCtrl::getXML();

	node->setName(LL_NAME_LIST_CTRL_TAG);

	node->createChild("allow_calling_card_drop", TRUE)->setBoolValue(mAllowCallingCardDrop);

	if (mNameColumnIndex != 0)
	{
		node->createChild("name_column_index", TRUE)->setIntValue(mNameColumnIndex);
	}

	// Don't save contents, probably filled by code

	return node;
}

LLView* LLNameListCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("name_list");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL multi_select = FALSE;
	node->getAttributeBOOL("multi_select", multi_select);

	BOOL draw_border = TRUE;
	node->getAttributeBOOL("draw_border", draw_border);

	BOOL draw_heading = FALSE;
	node->getAttributeBOOL("draw_heading", draw_heading);

	S32 name_column_index = 0;
	node->getAttributeS32("name_column_index", name_column_index);

	LLUICtrlCallback callback = NULL;

	LLNameListCtrl* name_list = new LLNameListCtrl(name,
				   rect,
				   callback,
				   NULL,
				   multi_select,
				   draw_border,
				   draw_heading,
				   name_column_index
				   );
	if (node->hasAttribute("heading_height"))
	{
		S32 heading_height;
		node->getAttributeS32("heading_height", heading_height);
		name_list->setHeadingHeight(heading_height);
	}

	BOOL allow_calling_card_drop = FALSE;
	if (node->getAttributeBOOL("allow_calling_card_drop", allow_calling_card_drop))
	{
		name_list->setAllowCallingCardDrop(allow_calling_card_drop);
	}

	name_list->setScrollListParameters(node);

	name_list->initFromXML(node, parent);

	LLSD columns;
	S32 index = 0;
	//S32 total_static = 0;
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("column"))
		{
			std::string labelname("");
			child->getAttributeString("label", labelname);

			std::string columnname(labelname);
			child->getAttributeString("name", columnname);

			BOOL columndynamicwidth = FALSE;
			child->getAttributeBOOL("dynamicwidth", columndynamicwidth);

			std::string sortname(columnname);
			child->getAttributeString("sort", sortname);
		
			S32 columnwidth = -1;
			if (child->hasAttribute("relwidth"))
			{
				F32 columnrelwidth = 0.f;
				child->getAttributeF32("relwidth", columnrelwidth);
				columns[index]["relwidth"] = columnrelwidth;
			}
			else
			{
				child->getAttributeS32("width", columnwidth);
				columns[index]["width"] = columnwidth;
			}

			LLFontGL::HAlign h_align = LLFontGL::LEFT;
			h_align = LLView::selectFontHAlign(child);

			//if(!columndynamicwidth) total_static += llmax(0, columnwidth);

			columns[index]["name"] = columnname;
			columns[index]["label"] = labelname;
			columns[index]["halign"] = (S32)h_align;
			columns[index]["dynamicwidth"] = columndynamicwidth;
			columns[index]["sort"] = sortname;

			index++;
		}
	}
	name_list->setColumnHeadings(columns);


	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("row"))
		{
			LLUUID id;
			child->getAttributeUUID("id", id);

			LLSD row;

			row["id"] = id;

			S32 column_idx = 0;
			LLXMLNodePtr row_child;
			for (row_child = node->getFirstChild(); row_child.notNull(); row_child = row_child->getNextSibling())
			{
				if (row_child->hasName("column"))
				{
					std::string value = row_child->getTextContents();

					std::string columnname("");
					row_child->getAttributeString("name", columnname);

					std::string font("");
					row_child->getAttributeString("font", font);

					std::string font_style("");
					row_child->getAttributeString("font-style", font_style);

					row["columns"][column_idx]["column"] = columnname;
					row["columns"][column_idx]["value"] = value;
					row["columns"][column_idx]["font"] = font;
					row["columns"][column_idx]["font-style"] = font_style;
					column_idx++;
				}
			}
			name_list->addElement(row);
		}
	}

	std::string contents = node->getTextContents();

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\t\n");
	tokenizer tokens(contents, sep);
	tokenizer::iterator token_iter = tokens.begin();

	while(token_iter != tokens.end())
	{
		const std::string& line = *token_iter;
		name_list->setCommentText(line);
		++token_iter;
	}

	return name_list;
}



