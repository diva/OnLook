/** 
 * @file llfloateravatarpicker.cpp
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

#include "llfloateravatarpicker.h"

// Viewer includes
#include "llagent.h"
#include "llfocusmgr.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llworld.h"

// Linden libraries
#include "llavatarnamecache.h"	// IDEVO
#include "llbutton.h"
#include "llcachename.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "message.h"

// [RLVa:KB]
#include "rlvhandler.h"
// [/RLVa:KB]

const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 340;
const LLRect FLOATER_RECT(0, 380, 240, 0);
const std::string FLOATER_TITLE = "Choose Resident";

// static
LLFloaterAvatarPicker* LLFloaterAvatarPicker::sInstance = NULL;
static std::map<LLUUID, LLAvatarName> sAvatarNameMap;

LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(select_callback_t callback,
												   BOOL allow_multiple,
												   BOOL closeOnSelect)
{
	// TODO: This class should not be a singleton as it's used in multiple places
	// and therefore can't be used simultaneously. -MG
	
	LLFloaterAvatarPicker* floater = sInstance ? sInstance : new LLFloaterAvatarPicker();
	floater->open();	/* Flawfinder: ignore */
	if(!sInstance)
	{
		sInstance = floater;
		floater->center();
	}
	
	floater->mSelectionCallback = callback;
	floater->setAllowMultiple(allow_multiple);
	floater->mNearMeListComplete = FALSE;
	floater->mCloseOnSelect = closeOnSelect;
	return floater;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker() :
	LLFloater(std::string("avatarpicker"), FLOATER_RECT, FLOATER_TITLE, TRUE, MIN_WIDTH, MIN_HEIGHT),
	mNumResultsReturned(0),
	mNearMeListComplete(FALSE),
	mCloseOnSelect(FALSE)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_avatar_picker.xml", NULL);
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	childSetKeystrokeCallback("Edit", editKeystroke, this);
	childSetKeystrokeCallback("EditUUID", editKeystroke, this);

	childSetAction("Find", boost::bind(&LLFloaterAvatarPicker::onBtnFind, this));
	getChildView("Find")->setEnabled(FALSE);
	childSetAction("Refresh", boost::bind(&LLFloaterAvatarPicker::onBtnRefresh, this));
	getChild<LLUICtrl>("near_me_range")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onRangeAdjust, this));

	LLScrollListCtrl* searchresults = getChild<LLScrollListCtrl>("SearchResults");
	searchresults->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	searchresults->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
	getChildView("SearchResults")->setEnabled(FALSE);

	LLScrollListCtrl* nearme = getChild<LLScrollListCtrl>("NearMe");
	nearme->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	nearme->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

	childSetAction("Select", boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	getChildView("Select")->setEnabled(FALSE);
	childSetAction("Cancel", boost::bind(&LLFloaterAvatarPicker::onBtnClose, this));

	getChild<LLUICtrl>("Edit")->setFocus(TRUE);

	LLPanel* search_panel = getChild<LLPanel>("SearchPanel");
	if (search_panel)
	{
		// Start searching when Return is pressed in the line editor.
		search_panel->setDefaultBtn("Find");
	}

	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("no_results"));

	LLInventoryPanel* inventory_panel = getChild<LLInventoryPanel>("InventoryPanel");
	inventory_panel->setFilterTypes(0x1 << LLInventoryType::IT_CALLINGCARD);
	inventory_panel->setFollowsAll();
	inventory_panel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	inventory_panel->openDefaultFolderForType(LLAssetType::AT_CALLINGCARD);
	inventory_panel->setSelectCallback(boost::bind(&LLFloaterAvatarPicker::onCallingCardSelectionChange, this, _1, _2));

	getChild<LLTabContainer>("ResidentChooserTabs")->setCommitCallback(
		boost::bind(&LLFloaterAvatarPicker::onTabChanged,this));

	setAllowMultiple(FALSE);

	return TRUE;
}

void LLFloaterAvatarPicker::onTabChanged()
{
	childSetEnabled("Select", visibleItemsSelected());
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

void LLFloaterAvatarPicker::onBtnFind()
{
	find();
}

static void getSelectedAvatarData(const LLScrollListCtrl* from, uuid_vec_t& avatar_ids, std::vector<LLAvatarName>& avatar_names)
{
	std::vector<LLScrollListItem*> items = from->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID().notNull())
		{
			avatar_ids.push_back(item->getUUID());

			std::map<LLUUID, LLAvatarName>::iterator iter = sAvatarNameMap.find(item->getUUID());
			if (iter != sAvatarNameMap.end())
			{
				avatar_names.push_back(iter->second);
			}
			else
			{
				// the only case where it isn't in the name map is friends
				// but it should be in the name cache
				LLAvatarName av_name;
				LLAvatarNameCache::get(item->getUUID(), &av_name);
				avatar_names.push_back(av_name);
			}
		}
	}
}

void LLFloaterAvatarPicker::onBtnSelect()
{

	// If select btn not enabled then do not callback
	if (!visibleItemsSelected())
		return;

	if(mSelectionCallback)
	{
		std::vector<LLAvatarName>	avatar_names;
		std::vector<LLUUID>			avatar_ids;
		std::string active_panel_name;
		LLScrollListCtrl* list =  NULL;
		LLPanel* active_panel = childGetVisibleTab("ResidentChooserTabs");
		if(active_panel)
		{
			active_panel_name = active_panel->getName();
		}
		if(active_panel_name == "CallingCardsPanel")
		{
			avatar_ids = mSelectedInventoryAvatarIDs;
			for(std::vector<std::string>::size_type i = 0; i < avatar_ids.size(); ++i)
			{
				std::map<LLUUID, LLAvatarName>::iterator iter = sAvatarNameMap.find(avatar_ids[i]);
				LLAvatarName av_name;
				if (iter != sAvatarNameMap.end())
				{
					avatar_names.push_back(iter->second);
				}
				else if(LLAvatarNameCache::get(avatar_ids[i], &av_name))
				{
					avatar_names.push_back(av_name);
				}
				else
				{
					std::string name = gCacheName->buildLegacyName(mSelectedInventoryAvatarNames[i]);
					std::string::size_type pos = name.find(' ');
					av_name.mLegacyFirstName = name.substr(pos);
					av_name.mLegacyLastName = pos!=std::string::npos ? name.substr(pos+1) : "Resident";
					av_name.mDisplayName = name;
					av_name.mUsername = "";
					avatar_names.push_back(av_name);
				}
			}
		}
		else if(active_panel_name == "SearchPanel")
		{
			list = getChild<LLScrollListCtrl>("SearchResults");
		}
		else if(active_panel_name == "NearMePanel")
		{
			list = getChild<LLScrollListCtrl>("NearMe");
		}
		else if(active_panel_name == "KeyPanel")
		{
			LLUUID specified = getChild<LLLineEditor>("EditUUID")->getValue().asUUID();
			if(specified.isNull())
				return;
			avatar_ids.push_back(specified);

			std::map<LLUUID, LLAvatarName>::iterator iter = sAvatarNameMap.find(specified);
			if (iter != sAvatarNameMap.end())
			{
				avatar_names.push_back(iter->second);
			}
			else
			{
				LLAvatarName av_name;
				LLAvatarNameCache::get(specified, &av_name);
				avatar_names.push_back(av_name);
			}
		}

		if(list)
		{
			getSelectedAvatarData(list, avatar_ids, avatar_names);
		}
		if(!avatar_names.empty() && !avatar_ids.empty())
		{
			mSelectionCallback(avatar_ids, avatar_names);
		}
	}
	getChild<LLInventoryPanel>("InventoryPanel")->setSelection(LLUUID::null, FALSE);
	getChild<LLScrollListCtrl>("SearchResults")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("NearMe")->deselectAllItems(TRUE);
	if(mCloseOnSelect)
	{
		mCloseOnSelect = FALSE;
		close();		
	}
}

void LLFloaterAvatarPicker::onBtnRefresh()
{
	getChild<LLScrollListCtrl>("NearMe")->deleteAllItems();
	getChild<LLScrollListCtrl>("NearMe")->setCommentText(getString("searching"));
	mNearMeListComplete = FALSE;
}

void LLFloaterAvatarPicker::onBtnClose()
{
	close();
}

void LLFloaterAvatarPicker::onRangeAdjust()
{
	onBtnRefresh();
}

void LLFloaterAvatarPicker::onList()
{
	getChildView("Select")->setEnabled(visibleItemsSelected());
}

// Callback for inventory picker (select from calling cards)
void LLFloaterAvatarPicker::onCallingCardSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	bool panel_active = (childGetVisibleTab("ResidentChooserTabs") == getChild<LLPanel>("CallingCardsPanel"));
	
	mSelectedInventoryAvatarIDs.clear();
	mSelectedInventoryAvatarNames.clear();
	
	if (panel_active)
	{
		childSetEnabled("Select", FALSE);
	}

	std::deque<LLFolderViewItem*>::const_iterator item_it;
	for (item_it = items.begin(); item_it != items.end(); ++item_it)
	{
		LLFolderViewEventListener* listenerp = (*item_it)->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			LLInventoryItem* item = gInventory.getItem(listenerp->getUUID());
			if (item)
			{
				mSelectedInventoryAvatarIDs.push_back(item->getCreatorUUID());
				mSelectedInventoryAvatarNames.push_back(listenerp->getName());
			}
		}
	}

	if (panel_active)
	{
		childSetEnabled("Select", visibleItemsSelected());
	}
}

void LLFloaterAvatarPicker::populateNearMe()
{
	BOOL all_loaded = TRUE;
	BOOL empty = TRUE;
	LLScrollListCtrl* near_me_scroller = getChild<LLScrollListCtrl>("NearMe");
	near_me_scroller->deleteAllItems();

	uuid_vec_t avatar_ids;
	LLWorld::getInstance()->getAvatars(&avatar_ids, NULL, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
	for(U32 i=0; i<avatar_ids.size(); i++)
	{
		LLUUID& av = avatar_ids[i];
		if(av == gAgent.getID()) continue;
		LLSD element;
		element["id"] = av; // value
		LLAvatarName av_name;

		if (!LLAvatarNameCache::get(av, &av_name))
		{
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = LLCacheName::getDefaultName();
			all_loaded = FALSE;
		}			
		else
		{
			element["columns"][0]["column"] = "name";
			element["columns"][0]["value"] = av_name.mDisplayName;
			element["columns"][1]["column"] = "username";
			element["columns"][1]["value"] = av_name.mUsername;

			sAvatarNameMap[av] = av_name;
		}
		near_me_scroller->addElement(element);
		empty = FALSE;
	}

	if (empty)
	{
		getChildView("NearMe")->setEnabled(FALSE);
		getChildView("Select")->setEnabled(FALSE);
		near_me_scroller->setCommentText(getString("no_one_near"));
	}
	else 
	{
		getChildView("NearMe")->setEnabled(TRUE);
		getChildView("Select")->setEnabled(TRUE);
		near_me_scroller->selectFirstItem();
		onList();
		near_me_scroller->setFocus(TRUE);
	}

	if (all_loaded)
	{
		mNearMeListComplete = TRUE;
	}
}

void LLFloaterAvatarPicker::draw()
{
	LLFloater::draw();

// [RLVa:KB] - Version: 1.23.4 | Checked: 2009-07-08 (RLVa-1.0.0e) | Added: RLVa-1.0.0e
	// TODO-RLVa: this code needs revisiting
	if (rlv_handler_t::isEnabled())
	{
		LLPanel* pNearMePanel = getChild<LLPanel>("NearMePanel");
		if ( (pNearMePanel) && (childGetVisibleTab("ResidentChooserTabs") == pNearMePanel) )
		{
			if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
			{
				if (mNearMeListComplete)
				{
					getChild<LLScrollListCtrl>("NearMe")->deleteAllItems();
					childSetEnabled("Select", false);
				}
				mNearMeListComplete = FALSE;
				pNearMePanel->setCtrlsEnabled(FALSE);
				return;
			}
			pNearMePanel->setCtrlsEnabled(TRUE);
		}
	}
// [/RLVa:KB]

	if (!mNearMeListComplete && childGetVisibleTab("ResidentChooserTabs") == getChild<LLPanel>("NearMePanel"))
	{
		populateNearMe();
	}
}

BOOL LLFloaterAvatarPicker::visibleItemsSelected() const
{
	LLPanel* active_panel = childGetVisibleTab("ResidentChooserTabs");

	if(active_panel == getChild<LLPanel>("SearchPanel"))
	{
		return getChild<LLScrollListCtrl>("SearchResults")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("CallingCardsPanel"))
	{
		return !mSelectedInventoryAvatarIDs.empty();
	}
	else if(active_panel == getChild<LLPanel>("NearMePanel"))
	{
		return getChild<LLScrollListCtrl>("NearMe")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("KeyPanel"))
	{
		LLUUID specified = getChild<LLLineEditor>("EditUUID")->getValue().asUUID();
		return !specified.isNull();
	}
	return FALSE;
}

extern AIHTTPTimeoutPolicy avatarPickerResponder_timeout;
class LLAvatarPickerResponder : public LLHTTPClient::ResponderWithCompleted
{
public:
	LLUUID mQueryID;

	LLAvatarPickerResponder(const LLUUID& id) : mQueryID(id) { }
	
	/*virtual*/ void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		//std::ostringstream ss;
		//LLSDSerialize::toPrettyXML(content, ss);
		//llinfos << ss.str() << llendl;

		// in case of invalid characters, the avatar picker returns a 400
		// just set it to process so it displays 'not found'
		if ((200 <= status && status < 300) || status == 400)
		{
			LLFloaterAvatarPicker* floater = LLFloaterAvatarPicker::sInstance;
			if (floater)
			{
				floater->processResponse(mQueryID, content);
			}
		}
		else
		{
			llinfos << "avatar picker failed " << status
					<< " reason " << reason << llendl;
			
		}
	}

	const AIHTTPTimeoutPolicy &getHTTPTimeoutPolicy(void) const { return avatarPickerResponder_timeout; }
};

void LLFloaterAvatarPicker::find()
{
	//clear our stored LLAvatarNames
	sAvatarNameMap.clear();

	const std::string& text = childGetValue("Edit").asString();

	mQueryID.generate();

	std::string url;
	url.reserve(128); // avoid a memory allocation or two

	LLViewerRegion* region = gAgent.getRegion();
	url = region->getCapability("AvatarPickerSearch");
	// Prefer use of capabilities to search on both SLID and display name
	// but allow display name search to be manually turned off for test
	if (!url.empty()
		&& LLAvatarNameCache::useDisplayNames())
	{
		// capability urls don't end in '/', but we need one to parse
		// query parameters correctly
		if (url.size() > 0 && url[url.size()-1] != '/')
		{
			url += "/";
		}
		url += "?page_size=100&names=";
		url += LLURI::escape(text);
		llinfos << "avatar picker " << url << llendl;
		LLHTTPClient::get(url, new LLAvatarPickerResponder(mQueryID));
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("AvatarPickerRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->addUUID("QueryID", mQueryID);	// not used right now
		msg->nextBlock("Data");
		msg->addString("Name", text);
		gAgent.sendReliableMessage();
	}

	getChild<LLScrollListCtrl>("SearchResults")->deleteAllItems();
	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("searching"));
	
	childSetEnabled("Select", FALSE);
	mNumResultsReturned = 0;
}

void LLFloaterAvatarPicker::setAllowMultiple(BOOL allow_multiple)
{
	getChild<LLScrollListCtrl>("SearchResults")->setAllowMultipleSelection(allow_multiple);
	getChild<LLInventoryPanel>("InventoryPanel")->setAllowMultiSelect(allow_multiple);
	getChild<LLScrollListCtrl>("NearMe")->setAllowMultipleSelection(allow_multiple);
}

// static 
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	avatar_id;
	std::string first_name;
	std::string last_name;

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "QueryID", query_id);

	// Not for us
	if (agent_id != gAgent.getID()) return;

	// Dialog already closed
	LLFloaterAvatarPicker *floater = sInstance;

	// floater is closed or these are not results from our last request
	if (NULL == floater || query_id != floater->mQueryID)
	{
		return;
	}

	LLScrollListCtrl* search_results = floater->getChild<LLScrollListCtrl>("SearchResults");

	// clear "Searching" label on first results
	if (floater->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}

	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocks("Data");
	for (S32 i = 0; i < num_new_rows; i++)
	{			
		msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID,	avatar_id, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, first_name, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_LastName,	last_name, i);
	
		std::string avatar_name;
		if (avatar_id.isNull())
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = floater->childGetText("Edit");
			avatar_name = floater->getString("not_found", map);
			search_results->setEnabled(FALSE);
			floater->getChildView("Select")->setEnabled(FALSE);
		}
		else
		{
			avatar_name = LLCacheName::buildFullName(first_name, last_name);
			search_results->setEnabled(TRUE);
			found_one = TRUE;

			LLAvatarName av_name;
			av_name.mLegacyFirstName = first_name;
			av_name.mLegacyLastName = last_name;
			av_name.mDisplayName = avatar_name;
			const LLUUID& agent_id = avatar_id;
			sAvatarNameMap[agent_id] = av_name;

		}
		LLSD element;
		element["id"] = avatar_id; // value
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = avatar_name;
		search_results->addElement(element);
	}

	if (found_one)
	{
		floater->getChildView("Select")->setEnabled(TRUE);
		search_results->selectFirstItem();
		floater->onList();
		search_results->setFocus(TRUE);
	}
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id, const LLSD& content)
{
	// Check for out-of-date query
	if (query_id != mQueryID) return;

	LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResults");

	LLSD agents = content["agents"];
	if (agents.size() == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = childGetText("Edit");
		LLSD item;
		item["id"] = LLUUID::null;
		item["columns"][0]["column"] = "name";
		item["columns"][0]["value"] = getString("not_found", map);
		search_results->addElement(item);
		search_results->setEnabled(false);
		getChildView("Select")->setEnabled(false);
		return;
	}

	// clear "Searching" label on first results
	search_results->deleteAllItems();

	LLSD item;
	LLSD::array_const_iterator it = agents.beginArray();
	for ( ; it != agents.endArray(); ++it)
	{
		const LLSD& row = *it;
		item["id"] = row["id"];
		LLSD& columns = item["columns"];
		columns[0]["column"] = "name";
		columns[0]["value"] = row["display_name"];
		columns[1]["column"] = "username";
		columns[1]["value"] = row["username"];
		search_results->addElement(item);

		// add the avatar name to our list
		LLAvatarName avatar_name;
		avatar_name.fromLLSD(row);
		sAvatarNameMap[row["id"].asUUID()] = avatar_name;
	}

	getChildView("Select")->setEnabled(true);
	search_results->setEnabled(true);
	search_results->selectFirstItem();
	onList();
	search_results->setFocus(TRUE);
}

//static
void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)user_data;
	LLPanel* active_panel = self->childGetVisibleTab("ResidentChooserTabs");
	if(active_panel == self->getChild<LLPanel>("SearchPanel"))
		self->childSetEnabled("Find", caller->getText().size() >= 3);
	else if(active_panel == self->getChild<LLPanel>("KeyPanel"))
	{
		LLUUID specified = self->getChild<LLLineEditor>("EditUUID")->getValue().asUUID();
		self->childSetEnabled("Select", specified.notNull());
	}
}

// virtual
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		if (getChild<LLUICtrl>("Edit")->hasFocus())
		{
			onBtnFind();
		}
		else
		{
			onBtnSelect();
		}
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		close();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}
