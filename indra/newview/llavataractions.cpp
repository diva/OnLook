/**
 * @file llavataractions.cpp
 * @brief Friend-related actions (add, remove, offer teleport, etc)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h"

#include "llavataractions.h"

#include "llavatarnamecache.h"	// IDEVO
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "roles_constants.h"    // for GP_MEMBER_INVITE

#include "llagent.h"
#include "llcallingcard.h"		// for LLAvatarTracker
#include "llfloateravatarinfo.h"
#include "llfloateravatarpicker.h"	// for LLFloaterAvatarPicker
#include "llfloaterchatterbox.h"
#include "llfloatergroupbulkban.h"
#include "llfloatergroupinvite.h"
#include "llfloatergroups.h"
#include "llfloaterwebprofile.h"
#include "llfloaterworldmap.h"
#include "llgiveinventory.h"
#include "llgivemoney.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llimview.h"			// for gIMMgr
#include "llmutelist.h"
#include "llpanelprofile.h"
#include "lltrans.h"
#include "llvoiceclient.h"
#include "llweb.h"
#include "llslurl.h"			// IDEVO
#include "llpanelmaininventory.h"
#include "llavatarname.h"
#include "llagentui.h"
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
#include "rlvactions.h"
#include "rlvcommon.h"
// [/RLVa:KB]

#include "llviewerwindow.h"
#include "llwindow.h"

extern const S32 TRANS_GIFT;
void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group = FALSE, S32 trx_type = TRANS_GIFT, const std::string& desc = LLStringUtil::null);
void handle_lure(const uuid_vec_t& ids);
void send_improved_im(const LLUUID& to_id, const std::string& name, const std::string& message, U8 offline, EInstantMessage dialog, const LLUUID& id, U32 timestamp = NO_TIMESTAMP, const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET, S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);

// static
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id, const std::string& name)
{
	if(id == gAgentID)
	{
		LLNotificationsUtil::add("AddSelfFriend");
		return;
	}

	LLSD args;
	args["NAME"] = name;
	LLSD payload;
	payload["id"] = id;
	payload["name"] = name;

	LLNotificationsUtil::add("AddFriendWithMessage", args, payload, &callbackAddFriendWithMessage);

	// add friend to recent people list
	//LLRecentPeople::instance().add(id);
}

void on_avatar_name_friendship(const LLUUID& id, const LLAvatarName av_name)
{
	LLAvatarActions::requestFriendshipDialog(id, av_name.getCompleteName());
}

// static
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id)
{
	if(id.isNull())
	{
		return;
	}

	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_friendship, _1, _2));
}

// static
void LLAvatarActions::removeFriendDialog(const LLUUID& id)
{
	if (id.isNull())
		return;

	uuid_vec_t ids;
	ids.push_back(id);
	removeFriendsDialog(ids);
}

// static
void LLAvatarActions::removeFriendsDialog(const uuid_vec_t& ids)
{
	if(ids.size() == 0)
		return;

	LLSD args;
	std::string msgType;
	if(ids.size() == 1)
	{
		LLUUID agent_id = ids[0];
		std::string av_name;
		if(LLAvatarNameCache::getPNSName(agent_id, av_name))
		{
			args["NAME"] = av_name;
		}

		msgType = "RemoveFromFriends";
	}
	else
	{
		msgType = "RemoveMultipleFromFriends";
	}

	LLSD payload;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		payload["ids"].append(*it);
	}

	LLNotificationsUtil::add(msgType,
		args,
		payload,
		&handleRemove);
}

// static
void LLAvatarActions::offerTeleport(const LLUUID& invitee)
{
	if (invitee.isNull())
		return;

	LLDynamicArray<LLUUID> ids;
	ids.push_back(invitee);
	offerTeleport(ids);
}

// static
void LLAvatarActions::offerTeleport(const uuid_vec_t& ids)
{
	if (ids.size() == 0)
		return;

	handle_lure(ids);
}

static void on_avatar_name_cache_start_im(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	static LLCachedControl<bool> tear_off("OtherChatsTornOff");
	if (!tear_off) gIMMgr->setFloaterOpen(true);
	gIMMgr->addSession(LLCacheName::cleanFullName(av_name.getLegacyName()), IM_NOTHING_SPECIAL, agent_id);
	make_ui_sound("UISndStartIM");
}

// static
void LLAvatarActions::startIM(const LLUUID& id)
{
	if (id.isNull() || gAgentID == id)
		return;

// [RLVa:KB] - Checked: 2013-05-09 (RLVa-1.4.9)
	if ( (!RlvActions::canStartIM(id)) && (!RlvActions::hasOpenP2PSession(id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
// [/RLVa:KB]

	LLAvatarName av_name;
	if (LLAvatarNameCache::get(id, &av_name)) // Bypass expiration, open NOW!
		on_avatar_name_cache_start_im(id, av_name);
	else
		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_im, _1, _2));
}

// static
void LLAvatarActions::endIM(const LLUUID& id)
{
	if (id.isNull())
		return;

	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	if (session_id.notNull())
	{
		gIMMgr->removeSession(session_id);
	}
}

static void on_avatar_name_cache_start_call(const LLUUID& agent_id,
											const LLAvatarName& av_name)
{
	LLUUID session_id = gIMMgr->addSession(LLCacheName::cleanFullName(av_name.getLegacyName()), IM_NOTHING_SPECIAL, agent_id);
	if (session_id.notNull())
	{
		gIMMgr->startCall(session_id);
	}
	make_ui_sound("UISndStartIM");
}

// static
void LLAvatarActions::startCall(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}

// [RLVa:KB] - Checked: 2013-05-09 (RLVa-1.4.9)
	if ( (!RlvActions::canStartIM(id)) && (!RlvActions::hasOpenP2PSession(id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
// [/RLVa:KB]

	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_call, _1, _2));
}

// static
void LLAvatarActions::startAdhocCall(const uuid_vec_t& ids)
{
	if (ids.size() == 0)
	{
		return;
	}

	// convert vector into LLDynamicArray for addSession
	LLDynamicArray<LLUUID> id_array;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
		id_array.push_back(idAgent);
// [/RLVa:KB]
//		id_array.push_back(*it);
	}

	// create the new ad hoc voice session
	const std::string title = LLTrans::getString("conference-title");
	LLUUID session_id = gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START,
										   ids[0], id_array);
	if (session_id.isNull())
	{
		return;
	}

	gIMMgr->autoStartCallOnStartup(session_id);

	make_ui_sound("UISndStartIM");
}

/* AD *TODO: Is this function needed any more?
	I fixed it a bit(added check for canCall), but it appears that it is not used
	anywhere. Maybe it should be removed?
// static
bool LLAvatarActions::isCalling(const LLUUID &id)
{
	if (id.isNull() || !canCall())
	{
		return false;
	}

	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	return (LLIMModel::getInstance()->findIMSession(session_id) != NULL);
}*/

//static
bool LLAvatarActions::canCall()
{
	return LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
}

// static
void LLAvatarActions::startConference(const uuid_vec_t& ids)
{
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
// [RLVa:KB] - Checked: 2011-04-11 (RLVa-1.3.0)
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
// [/RLVa:KB]
	}
	static LLCachedControl<bool> tear_off("OtherChatsTornOff");
	if (!tear_off) gIMMgr->setFloaterOpen(true);
	const std::string title = LLTrans::getString("conference-title");
	gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START, ids[0], ids);
	make_ui_sound("UISndStartIM");
}

static const char* get_profile_floater_name(const LLUUID& avatar_id)
{
	// Use different floater XML for our profile to be able to save its rect.
	return avatar_id == gAgentID ? "my_profile" : "profile";
}

static void on_avatar_name_show_profile(const LLUUID& agent_id, const LLAvatarName& av_name, bool web)
{
	if (gSavedSettings.getString("WebProfileURL").empty() || !(web || gSavedSettings.getBOOL("UseWebProfiles")))
	{
		LLFloaterAvatarInfo* floater = LLFloaterAvatarInfo::getInstance(agent_id);
		if(!floater)
		{
			floater = new LLFloaterAvatarInfo(av_name.getCompleteName()+" - "+LLTrans::getString("Command_Profile_Label"), agent_id);
			floater->center();
		}

		// ...bring that window to front
		floater->open();	/*Flawfinder: ignore*/
	}
	else
	{
		std::string username = av_name.mUsername;
		if (username.empty())
		{
			username = LLCacheName::buildUsername(av_name.mDisplayName);
		}

		llinfos << "opening web profile for " << username << llendl;
		std::string url = getProfileURL(username);

		// PROFILES: open in webkit window
		LLFloaterWebContent::Params p;
		p.url(url).
			id(agent_id.asString());
		LLFloaterWebProfile::showInstance(get_profile_floater_name(agent_id), p);
	}
}

// static
void LLAvatarActions::showProfile(const LLUUID& id, bool web)
{
	if (id.notNull())
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(id, &av_name)) // Bypass expiration, open NOW!
			on_avatar_name_show_profile(id, av_name, web);
		else
			LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_show_profile, _1, _2, web));
	}
}

// static
void LLAvatarActions::showProfiles(const uuid_vec_t& ids, bool web)
{
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
		showProfile(*it, web);
}

//static
bool LLAvatarActions::profileVisible(const LLUUID& id)
{
	LLFloater* browser = getProfileFloater(id);
	return browser && browser->getVisible();
}

//static
LLFloater* LLAvatarActions::getProfileFloater(const LLUUID& id)
{
	LLFloater* browser;
	if (gSavedSettings.getString("WebProfileURL").empty() || !gSavedSettings.getBOOL("UseWebProfiles"))
		browser = LLFloaterAvatarInfo::getInstance(id);
	else
		browser =
			LLFloaterWebProfile::getInstance(id.asString());
	return browser;
}

//static
void LLAvatarActions::hideProfile(const LLUUID& id)
{
	LLFloater* browser = getProfileFloater(id);
	if (browser)
	{
		browser->close();
	}
}

// static
void LLAvatarActions::showOnMap(const LLUUID& id)
{
	std::string av_name;
	if (!LLAvatarNameCache::getPNSName(id, av_name))
	{
		LLAvatarNameCache::get(id, boost::bind(&LLAvatarActions::showOnMap, id));
		return;
	}

	gFloaterWorldMap->trackAvatar(id, av_name);
	LLFloaterWorldMap::show(true);
}

// static
void LLAvatarActions::pay(const LLUUID& id)
{
	LLNotification::Params params("BusyModePay");
	params.functor(boost::bind(&LLAvatarActions::handlePay, _1, _2, id));

	if (gAgent.getBusy())
	{
		// warn users of being in busy mode during a transaction
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 1);
	}
}

void LLAvatarActions::teleport_request_callback(const LLSD& notification, const LLSD& response)
{
	S32 option;
	if (response.isInteger())
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}

	if (0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_MessageBlock);
		msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
		msg->addUUIDFast(_PREHASH_ToAgentID, notification["substitutions"]["uuid"] );
		msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
		msg->addU8Fast(_PREHASH_Dialog, IM_TELEPORT_REQUEST);
		msg->addUUIDFast(_PREHASH_ID, LLUUID::null);
		msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

		std::string name;
		LLAgentUI::buildFullname(name);

		msg->addStringFast(_PREHASH_FromAgentName, name);
		msg->addStringFast(_PREHASH_Message, response["message"]);
		msg->addU32Fast(_PREHASH_ParentEstateID, 0);
		msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
		msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());

		gMessageSystem->addBinaryDataFast(
				_PREHASH_BinaryBucket,
				EMPTY_BINARY_BUCKET,
				EMPTY_BINARY_BUCKET_SIZE);

		gAgent.sendReliableMessage();
	}
}

// static
void LLAvatarActions::teleportRequest(const LLUUID& id)
{
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(id, &av_name)) // Bypass expiration, open NOW!
		on_avatar_name_cache_teleport_request(id, av_name);
	else
		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_teleport_request, _1, _2));
}

// static
void LLAvatarActions::on_avatar_name_cache_teleport_request(const LLUUID& id, const LLAvatarName& av_name)
{
	LLSD notification;
	notification["uuid"] = id;
	std::string name;
// [RLVa:KB] - Checked: 2014-03-31 (Catznip-3.6)
	if (!RlvActions::canShowName(RlvActions::SNC_TELEPORTREQUEST))
		name = RlvStrings::getAnonym(av_name.getLegacyName());
	else
// [RLVa:KB]
	LLAvatarNameCache::getPNSName(av_name, name);
	notification["NAME"] = name;
	LLSD payload;

	LLNotificationsUtil::add("TeleportRequestPrompt", notification, payload, teleport_request_callback);
}

// static
void LLAvatarActions::kick(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("KickUser", LLSD(), payload, handleKick);
}

// static
void LLAvatarActions::freeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("FreezeUser", LLSD(), payload, handleFreeze);
}

// static
void LLAvatarActions::unfreeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("UnFreezeUser", LLSD(), payload, handleUnfreeze);
}

//static
void LLAvatarActions::csr(const LLUUID& id)
{
	std::string name;
	if (!gCacheName->getFullName(id, name)) return;

	std::string url = "http://csr.lindenlab.com/agent/";

	// slow and stupid, but it's late
	S32 len = name.length();
	for (S32 i = 0; i < len; i++)
	{
		if (name[i] == ' ')
		{
			url += "%20";
		}
		else
		{
			url += name[i];
		}
	}

	LLWeb::loadURL(url);
}

//static
void LLAvatarActions::share(const LLUUID& id)
{
	/*
	LLSD key;
	LLFloaterSidePanelContainer::showPanel("inventory", key);
	LLFloaterReg::showInstance("im_container");
	*/
	LLInventoryView::getActiveInventory()->setVisible(true);

	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);

	if (!gIMMgr->hasSession(session_id))
	{
		startIM(id);
	}

	if (gIMMgr->hasSession(session_id))
	{
		// we should always get here, but check to verify anyways
		LLIMMgr::getInstance()->addMessage(session_id, LLUUID::null, SYSTEM_FROM, LLTrans::getString("share_alert"));
		LLFloaterChatterBox::showInstance(session_id);
	}
}

namespace action_give_inventory
{
	/**
	 * Returns a pointer to 'Add More' inventory panel of Edit Outfit SP.
	 *
	static LLInventoryPanel* get_outfit_editor_inventory_panel()
	{
		LLPanelOutfitEdit* panel_outfit_edit = dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
		if (NULL == panel_outfit_edit) return NULL;

		LLInventoryPanel* inventory_panel = panel_outfit_edit->findChild<LLInventoryPanel>("folder_view");
		return inventory_panel;
	}*/

	/**
	 * @return active inventory panel, or NULL if there's no such panel
	 */
	static LLInventoryPanel* get_active_inventory_panel()
	{
		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
		/*if (!active_panel)
		{
			active_panel = get_outfit_editor_inventory_panel();
		}*/

		return active_panel;
	}

	/**
	 * Checks My Inventory visibility.
	 */

	static bool is_give_inventory_acceptable()
	{
		// check selection in the panel
		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty()) return false; // nothing selected

		bool acceptable = false;
		std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();
		const std::set<LLUUID>::const_iterator it_end = inventory_selected_uuids.end();
		for (; it != it_end; ++it)
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			// any category can be offered.
			if (inv_cat)
			{
				acceptable = true;
				continue;
			}

			LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
			// check if inventory item can be given
			if (LLGiveInventory::isInventoryGiveAcceptable(inv_item))
			{
				acceptable = true;
				continue;
			}

			// there are neither item nor category in inventory
			acceptable = false;
			break;
		}
		return acceptable;
	}

	static void build_items_string(const std::set<LLUUID>& inventory_selected_uuids , std::string& items_string)
	{
		llassert(inventory_selected_uuids.size() > 0);

		const std::string& separator = LLTrans::getString("words_separator");
		for (std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin(); ; )
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			if (NULL != inv_cat)
			{
				items_string = inv_cat->getName();
				break;
			}
			LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
			if (NULL != inv_item)
			{
				items_string.append(inv_item->getName());
			}
			if(++it == inventory_selected_uuids.end())
			{
				break;
			}
			items_string.append(separator);
		}
	}

	struct LLShareInfo : public LLSingleton<LLShareInfo>
	{
		std::vector<LLAvatarName> mAvatarNames;
		uuid_vec_t mAvatarUuids;
	};

	static void give_inventory_cb(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		// if Cancel pressed
		if (option == 1)
		{
			return;
		}

		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty())
		{
			return;
		}

		S32 count = LLShareInfo::instance().mAvatarNames.size();
		bool shared = count && !inventory_selected_uuids.empty();

		// iterate through avatars
		for(S32 i = 0; i < count; ++i)
		{
			const LLUUID& avatar_uuid = LLShareInfo::instance().mAvatarUuids[i];

			// We souldn't open IM session, just calculate session ID for logging purpose. See EXT-6710
			const LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, avatar_uuid);

			std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();
			const std::set<LLUUID>::const_iterator it_end = inventory_selected_uuids.end();

			const std::string& separator = LLTrans::getString("words_separator");
			std::string noncopy_item_names;
			LLSD noncopy_items = LLSD::emptyArray();
			// iterate through selected inventory objects
			for (; it != it_end; ++it)
			{
				LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
				if (inv_cat)
				{
					if (!LLGiveInventory::doGiveInventoryCategory(avatar_uuid, inv_cat, session_id, "ItemsShared"))
					{
						shared = false;
					}
					break;
				}
				LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
				if (!inv_item->getPermissions().allowCopyBy(gAgentID))
				{
					if (!noncopy_item_names.empty())
					{
						noncopy_item_names.append(separator);
					}
					noncopy_item_names.append(inv_item->getName());
					noncopy_items.append(*it);
				}
				else
				{
					if (!LLGiveInventory::doGiveInventoryItem(avatar_uuid, inv_item, session_id))
					{
						shared = false;
					}
				}
			}
			if (noncopy_items.beginArray() != noncopy_items.endArray())
			{
				LLSD substitutions;
				substitutions["ITEMS"] = noncopy_item_names;
				LLSD payload;
				payload["agent_id"] = avatar_uuid;
				payload["items"] = noncopy_items;
				payload["success_notification"] = "ItemsShared";
				LLNotificationsUtil::add("CannotCopyWarning", substitutions, payload,
					&LLGiveInventory::handleCopyProtectedItem);
				shared = false;
				break;
			}
		}
		if (shared)
		{
			if (LLFloaterAvatarPicker::instanceExists()) LLFloaterAvatarPicker::instance().close();
			LLNotificationsUtil::add("ItemsShared");
		}
	}

	/**
	 * Performs "give inventory" operations for provided avatars.
	 *
	 * Sends one requests to give all selected inventory items for each passed avatar.
	 * Avatars are represent by two vectors: names and UUIDs which must be sychronized with each other.
	 *
	 * @param avatar_names - avatar names request to be sent.
	 * @param avatar_uuids - avatar names request to be sent.
	 */
	static void give_inventory(const uuid_vec_t& avatar_uuids, const std::vector<LLAvatarName> avatar_names)
	{
		llassert(avatar_names.size() == avatar_uuids.size());

		const std::set<LLUUID> inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty())
		{
			return;
		}

		std::string residents;
		LLAvatarActions::buildResidentsString(avatar_names, residents);

		std::string items;
		build_items_string(inventory_selected_uuids, items);

		int folders_count = 0;
		std::set<LLUUID>::const_iterator it = inventory_selected_uuids.begin();

		//traverse through selected inventory items and count folders among them
		for ( ; it != inventory_selected_uuids.end() && folders_count <=1 ; ++it)
		{
			LLViewerInventoryCategory* inv_cat = gInventory.getCategory(*it);
			if (NULL != inv_cat)
			{
				folders_count++;
			}
		}

		// EXP-1599
		// In case of sharing multiple folders, make the confirmation
		// dialog contain a warning that only one folder can be shared at a time.
		std::string notification = (folders_count > 1) ? "ShareFolderConfirmation" : "ShareItemsConfirmation";
		LLSD substitutions;
		substitutions["RESIDENTS"] = residents;
		substitutions["ITEMS"] = items;
		LLShareInfo::instance().mAvatarNames = avatar_names;
		LLShareInfo::instance().mAvatarUuids = avatar_uuids;
		LLNotificationsUtil::add(notification, substitutions, LLSD(), &give_inventory_cb);
	}
}

// static
void LLAvatarActions::buildResidentsString(std::vector<LLAvatarName> avatar_names, std::string& residents_string)
{
	llassert(avatar_names.size() > 0);

	std::sort(avatar_names.begin(), avatar_names.end());
	const std::string& separator = LLTrans::getString("words_separator");
	for (std::vector<LLAvatarName>::const_iterator it = avatar_names.begin(); ; )
	{
		residents_string.append((*it).getCompleteName());
		if	(++it == avatar_names.end())
		{
			break;
		}
		residents_string.append(separator);
	}
}

// static
void LLAvatarActions::buildResidentsString(const uuid_vec_t& avatar_uuids, std::string& residents_string)
{
	std::vector<LLAvatarName> avatar_names;
	uuid_vec_t::const_iterator it = avatar_uuids.begin();
	for (; it != avatar_uuids.end(); ++it)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(*it, &av_name))
		{
			avatar_names.push_back(av_name);
		}
	}

	// We should check whether the vector is not empty to pass the assertion
	// that avatar_names.size() > 0 in LLAvatarActions::buildResidentsString.
	if (!avatar_names.empty())
	{
		LLAvatarActions::buildResidentsString(avatar_names, residents_string);
	}
}

//static
std::set<LLUUID> LLAvatarActions::getInventorySelectedUUIDs()
{
	LLInventoryPanel* active_panel = action_give_inventory::get_active_inventory_panel();
	return active_panel ? active_panel->getRootFolder()->getSelectionList() : std::set<LLUUID>(); 
	/*std::set<LLFolderViewItem*> inventory_selected;

	LLInventoryPanel* active_panel = action_give_inventory::get_active_inventory_panel();
	if (active_panel)
	{
		inventory_selected= active_panel->getRootFolder()->getSelectionList();
	}

	if (inventory_selected.empty())
	{
		LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
		if (sidepanel_inventory)
		{
			inventory_selected= sidepanel_inventory->getInboxSelectionList();
		}
	}

	std::set<LLUUID> inventory_selected_uuids;
	for (std::set<LLFolderViewItem*>::iterator it = inventory_selected.begin(), end_it = inventory_selected.end();
		it != end_it;
		++it)
	{
		inventory_selected_uuids.insert(static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem())->getUUID());
	}
	return inventory_selected_uuids;*/
}

//static
void LLAvatarActions::shareWithAvatars(LLView * panel)
{
	using namespace action_give_inventory;

	LLFloater* root_floater = gFloaterView->getParentFloater(panel);
	LLFloaterAvatarPicker* picker =
		LLFloaterAvatarPicker::show(boost::bind(give_inventory, _1, _2), TRUE, FALSE, FALSE, root_floater->getName());
	if (!picker)
	{
		return;
	}

	picker->setOkBtnEnableCb(boost::bind(is_give_inventory_acceptable));
	picker->openFriendsTab();

	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
	LLNotificationsUtil::add("ShareNotification");
}


// static
bool LLAvatarActions::canShareSelectedItems(LLInventoryPanel* inv_panel /* = NULL*/)
{
	using namespace action_give_inventory;

	if (!inv_panel)
	{
		LLInventoryPanel* active_panel = get_active_inventory_panel();
		if (!active_panel) return false;
		inv_panel = active_panel;
	}

	// check selection in the panel
	LLFolderView* root_folder = inv_panel->getRootFolder();
    if (!root_folder)
    {
        return false;
    }
	const std::set<LLUUID> inventory_selected = root_folder->getSelectionList();
	if (inventory_selected.empty()) return false; // nothing selected

	bool can_share = true;
	const LLUUID& trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);  // <alchemy/>
	std::set<LLUUID>::const_iterator it = inventory_selected.begin();
	const std::set<LLUUID>::const_iterator it_end = inventory_selected.end();
	for (; it != it_end; ++it)
	{
		const LLUUID id(*it);
		LLViewerInventoryCategory* inv_cat = gInventory.getCategory(id);
		// any category can be offered.
		if (inv_cat && !gInventory.isObjectDescendentOf(inv_cat->getUUID(), trash_id)) // <alchemy/>
		{
			continue;
		}

		// check if inventory item can be given
		else if (!inv_cat && gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID())) // Singu Note: These three ifs comprise the item half of LLInvFVBridge::canShare, which LL calls here.
			if (LLViewerInventoryItem* item = gInventory.getItem(id))
				if (LLInventoryCollectFunctor::itemTransferCommonlyAllowed(item) && LLGiveInventory::isInventoryGiveAcceptable(item))
		/*
		LLFolderViewItem* item = *it;
		if (!item) return false;
		LLInvFVBridge* bridge = dynamic_cast<LLInvFVBridge*>(item->getViewModelItem());
		if (bridge && bridge->canShare())
		*/
		{
			continue;
		}

		// there are neither item nor category in inventory
		can_share = false;
		break;
	}

	return can_share;
}

// static
void LLAvatarActions::toggleBlock(const LLUUID& id)
{
	std::string name;

	gCacheName->getFullName(id, name); // needed for mute
	LLMute mute(id, name, LLMute::AGENT);

	if (LLMuteList::getInstance()->isMuted(mute.mID, mute.mName))
	{
		LLMuteList::getInstance()->remove(mute);
	}
	else
	{
		LLMuteList::getInstance()->add(mute);
	}
}

// static
void LLAvatarActions::toggleMuteVoice(const LLUUID& id)
{
	std::string name;
	gCacheName->getFullName(id, name); // needed for mute

	LLMuteList* mute_list = LLMuteList::getInstance();
	bool is_muted = mute_list->isMuted(id, LLMute::flagVoiceChat);

	LLMute mute(id, name, LLMute::AGENT);
	if (!is_muted)
	{
		mute_list->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		mute_list->remove(mute, LLMute::flagVoiceChat);
	}
}

// static
bool LLAvatarActions::canOfferTeleport(const LLUUID& id)
{
	// First use LLAvatarTracker::isBuddy()
	// If LLAvatarTracker::instance().isBuddyOnline function only is used
	// then for avatars that are online and not a friend it will return false.
	// But we should give an ability to offer a teleport for such avatars.
	if(LLAvatarTracker::instance().isBuddy(id))
	{
		return LLAvatarTracker::instance().isBuddyOnline(id);
	}

	return true;
}

// static
bool LLAvatarActions::canOfferTeleport(const uuid_vec_t& ids)
{
	// We can't send more than 250 lures in a single message, so disable this
	// button when there are too many id's selected.
	if(ids.size() > 250) return false;

	bool result = true;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		if(!canOfferTeleport(*it))
		{
			result = false;
			break;
		}
	}
	return result;
}

void LLAvatarActions::inviteToGroup(const LLUUID& id)
{
	inviteToGroup(uuid_vec_t(1, id));
}

void LLAvatarActions::inviteToGroup(const uuid_vec_t& ids)
{
	LLFloaterGroupPicker* widget = LLFloaterGroupPicker::showInstance(LLSD(ids.front()));
	if (widget)
	{
		widget->center();
		widget->setPowersMask(GP_MEMBER_INVITE);
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(callback_invite_to_group, _1, ids));
	}
}

//== private methods ========================================================================================

// static
bool LLAvatarActions::handleRemove(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	const LLSD& ids = notification["payload"]["ids"];
	for (LLSD::array_const_iterator itr = ids.beginArray(); itr != ids.endArray(); ++itr)
	{
		LLUUID id = itr->asUUID();
		const LLRelationship* ip = LLAvatarTracker::instance().getBuddyInfo(id);
		if (ip)
		{
			switch (option)
			{
			case 0: // YES
				if( ip->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
				{
					LLAvatarTracker::instance().empower(id, FALSE);
					LLAvatarTracker::instance().notifyObservers();
				}
				LLAvatarTracker::instance().terminateBuddy(id);
				LLAvatarTracker::instance().notifyObservers();
				gInventory.addChangedMask(LLInventoryObserver::LABEL | LLInventoryObserver::CALLING_CARD, LLUUID::null);
				gInventory.notifyObservers();
				break;

			case 1: // NO
			default:
				llinfos << "No removal performed." << llendl;
				break;
			}
		}
	}
	return false;
}

// static
bool LLAvatarActions::handlePay(const LLSD& notification, const LLSD& response, LLUUID avatar_id)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		gAgent.clearBusy();
	}

	LLFloaterPay::payDirectly(&give_money, avatar_id, /*is_group=*/false);
	return false;
}

// <singu> Ban from group functions
void callback_ban_from_group(const LLUUID& group, uuid_vec_t& ids)
{
	LLFloaterGroupBulkBan::showForGroup(group, &ids);
}

void ban_from_group(const uuid_vec_t& ids)
{
	if (LLFloaterGroupPicker* widget = LLFloaterGroupPicker::showInstance(ids.front())) // It'd be cool if LLSD could be formed from uuid_vec_t
	{
		widget->center();
		widget->setPowersMask(GP_GROUP_BAN_ACCESS);
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(callback_ban_from_group, _1, ids));
	}
}
// </singu>

// static
void LLAvatarActions::callback_invite_to_group(LLUUID group_id, uuid_vec_t& agent_ids)
{
	LLFloaterGroupInvite::showForGroup(group_id, &agent_ids);
}


// static
bool LLAvatarActions::callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		requestFriendship(notification["payload"]["id"].asUUID(),
		    notification["payload"]["name"].asString(),
		    response["message"].asString());
	}
	return false;
}

// static
bool LLAvatarActions::handleKick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}
bool LLAvatarActions::handleFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_FREEZE );
		msg->addStringFast(_PREHASH_Reason, response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}
bool LLAvatarActions::handleUnfreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_UNFREEZE );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
	return false;
}

// static
void LLAvatarActions::requestFriendship(const LLUUID& target_id, const std::string& target_name, const std::string& message)
{
	const LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
	send_improved_im(target_id,
					 target_name,
					 message,
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED,
					 calling_card_folder_id);

	LLSD args;
	args["TO_NAME"] = target_name;

	LLSD payload;
	payload["from_id"] = target_id;
	LLNotificationsUtil::add("FriendshipOffered", args, payload);
}

//static
bool LLAvatarActions::isFriend(const LLUUID& id)
{
	return ( NULL != LLAvatarTracker::instance().getBuddyInfo(id) );
}

// static
bool LLAvatarActions::isBlocked(const LLUUID& id)
{
	return LLMuteList::getInstance()->isMuted(id);
}

// static
bool LLAvatarActions::isVoiceMuted(const LLUUID& id)
{
	return LLMuteList::getInstance()->isMuted(id, LLMute::flagVoiceChat);
}

// static
bool LLAvatarActions::canBlock(const LLUUID& id)
{
	bool is_linden = LLMuteList::getInstance()->isLinden(id);
	bool is_self = id == gAgentID;
	return !is_self && !is_linden;
}

//static
bool LLAvatarActions::isAgentMappable(const LLUUID& agent_id)
{
	const LLRelationship* buddy_info = NULL;
	bool is_friend = LLAvatarActions::isFriend(agent_id);

	if (is_friend)
		buddy_info = LLAvatarTracker::instance().getBuddyInfo(agent_id);

	return (buddy_info &&
			buddy_info->isOnline() &&
			buddy_info->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)
			);
}

// static
void LLAvatarActions::copyUUIDs(const uuid_vec_t& ids)
{
	std::string ids_string;
	const std::string& separator = LLTrans::getString("words_separator");
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		const LLUUID& id = *it;
		if (id.isNull())
			continue;

		if (!ids_string.empty())
			ids_string.append(separator);

		ids_string.append(id.asString());
	}

	if (!ids_string.empty())
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(ids_string));
}
