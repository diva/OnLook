/** 
 * @file llmenucommands.cpp
 * @brief Implementations of menu commands.
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

#include "llmenucommands.h"

#include "aihttpview.h"
#include "floaterao.h"
#include "floaterlocalassetbrowse.h"
#include "hbfloatergrouptitles.h"
#include "jcfloaterareasearch.h"
#include "llagentcamera.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "lldebugview.h"
#include "llfasttimerview.h"
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloaterautoreplacesettings.h"
#include "llfloateravatar.h"
#include "llfloateravatarlist.h"
#include "llfloaterbeacons.h"
#include "llfloaterblacklist.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloaterbuycurrency.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloatercustomize.h"
#include "llfloaterdaycycle.h"
#include "llfloaterdestinations.h"
#include "llfloaterdisplayname.h"
#include "llfloatereditui.h"
#include "llfloaterenvsettings.h"
#include "llfloaterexploreanimations.h"
#include "llfloaterexploresounds.h"
#include "llfloaterfonttest.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloaterhud.h"
#include "llfloaterinspect.h"
#include "llfloaterinventory.h"
#include "llfloaterjoystick.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermediafilter.h"
#include "llfloatermemleak.h"
#include "llfloatermessagelog.h"
#include "llfloatermute.h"
#include "llfloaternotificationsconsole.h"
#include "llfloateroutbox.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterperms.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloaterscriptlimits.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersnapshot.h"
#include "llfloaterstats.h"
#include "llfloaterteleporthistory.h"
#include "llfloatertest.h"
#include "llfloatervoiceeffect.h"
#include "llfloaterwater.h"
#include "llfloaterwebcontent.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llframestatview.h"
#include "llmakeoutfitdialog.h"
#include "llmoveview.h" // LLFloaterMove
#include "lltextureview.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lluictrlfactory.h"
#include "llvelocitybar.h"
#include "llviewerparcelmgr.h"
// [RLVa:LF]
#include "rlvfloaters.h"
// [/RLVa:LF]
#include "shfloatermediaticker.h"

void handle_chat()
{
	// give focus to chatbar if it's open but not focused
	static const LLCachedControl<bool> chat_visible("ChatVisible",true);
	(chat_visible && gFocusMgr.childHasKeyboardFocus(gChatBar)) ? LLChatBar::stopChat() : LLChatBar::startChat(NULL);
}
void handle_debug_avatar_textures(void*);
template<typename T> void handle_singleton_toggle(void*);
void show_outfit_dialog() { new LLMakeOutfitDialog(false); }
void toggle_build() { LLToolMgr::getInstance()->toggleBuildMode(); }
void toggle_control(const std::string& name) { if (LLControlVariable* control = gSavedSettings.getControl(name)) control->set(!control->get()); }
void toggle_search_floater();
void toggle_always_run() { gAgent.getAlwaysRun() ? gAgent.clearAlwaysRun() : gAgent.setAlwaysRun(); }
void toggle_sit();
void toggle_mouselook() { gAgentCamera.cameraMouselook() ? gAgentCamera.changeCameraToDefault() : gAgentCamera.changeCameraToMouselook(); }

bool is_visible_view(boost::function<LLView* ()> get)
{
	if (LLView* v = get())
		return v->getVisible();
	return false;
}

struct CommWrapper
{
	static bool only_comm()
	{
		static const LLCachedControl<bool> only("CommunicateSpecificShortcut");
		return only || LLFloaterChatterBox::getInstance()->getFloaterCount();
	}
	static bool instanceVisible(const LLSD& key) { return only_comm() ? LLFloaterChatterBox::instanceVisible(key) : LLFloaterMyFriends::instanceVisible(key); }
	static void toggleInstance(const LLSD& key) { only_comm() ? LLFloaterChatterBox::toggleInstance(key) : LLFloaterMyFriends::toggleInstance(key); }
};

struct MenuFloaterDict : public LLSingleton<MenuFloaterDict>
{
	typedef std::map<const std::string, std::pair<boost::function<void ()>, boost::function<bool ()> > > menu_floater_map_t;
	menu_floater_map_t mEntries;

	MenuFloaterDict()
	{
		registerConsole("debug console", gDebugView->mDebugConsolep);
		registerConsole("fast timers", gDebugView->mFastTimerView);
		registerConsole("frame console", gDebugView->mFrameStatView);
		registerConsole("http console", gHttpView);
		registerConsole("texture console", gTextureView);
		if (gAuditTexture)
		{
			registerConsole("texture category console", gTextureCategoryView);
			registerConsole("texture size console", gTextureSizeView);
		}
		registerConsole("velocity", gVelocityBar);
		registerFloater("about", boost::bind(&LLFloaterAbout::show,(void*)NULL));
		registerFloater("always run", boost::bind(toggle_always_run), boost::bind(&LLAgent::getAlwaysRun, &gAgent));
		registerFloater("anims_explorer", boost::bind(LLFloaterExploreAnimations::show));
		registerFloater("ao", boost::bind(LLFloaterAO::show, (void*)NULL));
		registerFloater("appearance", boost::bind(LLFloaterCustomize::show));
		registerFloater("asset_blacklist", boost::bind(LLFloaterBlacklist::toggle), boost::bind(LLFloaterBlacklist::visible));
		registerFloater("build", boost::bind(toggle_build));
		registerFloater("buy currency", boost::bind(LLFloaterBuyCurrency::buyCurrency));
		registerFloater("buy land", boost::bind(&LLViewerParcelMgr::startBuyLand, boost::bind(LLViewerParcelMgr::getInstance), false));
		registerFloater("chatbar", boost::bind(handle_chat));
		registerFloater("complaint reporter", boost::bind(LLFloaterReporter::showFromMenu, COMPLAINT_REPORT));
		registerFloater("DayCycle", boost::bind(LLFloaterDayCycle::show), boost::bind(LLFloaterDayCycle::isOpen));
		registerFloater("debug avatar", boost::bind(handle_debug_avatar_textures, (void*)NULL));
		registerFloater("debug settings", boost::bind(handle_singleton_toggle<LLFloaterSettingsDebug>, (void*)NULL));
		registerFloater("displayname", boost::bind(LLFloaterDisplayName::show));
		registerFloater("edit ui", boost::bind(LLFloaterEditUI::show, (void*)NULL));
		registerFloater("EnvSettings", boost::bind(LLFloaterEnvSettings::show), boost::bind(LLFloaterEnvSettings::isOpen));
		registerFloater("fly", boost::bind(LLAgent::toggleFlying));
		registerFloater("font test", boost::bind(LLFloaterFontTest::show, (void*)NULL));
		registerFloater("god tools", boost::bind(LLFloaterGodTools::show, (void*)NULL));
		registerFloater("grid options", boost::bind(LLFloaterBuildOptions::show, (void*)NULL));
		registerFloater("group titles", boost::bind(HBFloaterGroupTitles::toggle));
		//Singu TODO: Re-implement f1 help.
		//registerFloater("help f1", boost::bind(/*gViewerHtmlHelp.show*/));
		registerFloater("help tutorial", boost::bind(LLFloaterHUD::showHUD));
		registerFloater("inventory", boost::bind(LLInventoryView::toggleVisibility, (void*)NULL), boost::bind(is_visible_view, static_cast<boost::function<LLView* ()> >(LLInventoryView::getActiveInventory)));
		registerFloater("local assets", boost::bind(FloaterLocalAssetBrowser::show, (void*)0));
		registerFloater("mean events", boost::bind(LLFloaterBump::show, (void*)NULL));
		registerFloater("media ticker", boost::bind(handle_ticker_toggle, (void*)NULL), boost::bind(SHFloaterMediaTicker::instanceExists));
		registerFloater("memleak", boost::bind(LLFloaterMemLeak::show, (void*)NULL));
		registerFloater("messagelog", boost::bind(LLFloaterMessageLog::show));
		registerFloater("mouselook", boost::bind(toggle_mouselook));
		registerFloater("my land", boost::bind(LLFloaterLandHoldings::show, (void*)NULL));
		registerFloater("outfit", boost::bind(show_outfit_dialog));
		registerFloater("PostProcess", boost::bind(LLFloaterPostProcess::show));
		registerFloater("preferences", boost::bind(LLFloaterPreference::show, (void*)NULL));
		registerFloater("quit", boost::bind(&LLAppViewer::userQuit, LLAppViewer::instance()));
		registerFloater("RegionDebugConsole", boost::bind(handle_singleton_toggle<LLFloaterRegionDebugConsole>, (void*)NULL), boost::bind(LLFloaterRegionDebugConsole::instanceExists));
		registerFloater("script errors", boost::bind(LLFloaterScriptDebug::show, LLUUID::null));
		registerFloater("search", boost::bind(toggle_search_floater));
		registerFloater("show inspect", boost::bind(LLFloaterInspect::showInstance, LLSD()));
		registerFloater("sit", boost::bind(toggle_sit));
		registerFloater("snapshot", boost::bind(LLFloaterSnapshot::show, (void*)NULL));
		registerFloater("sound_explorer", boost::bind(LLFloaterExploreSounds::toggle), boost::bind(LLFloaterExploreSounds::visible));
		registerFloater("test", boost::bind(LLFloaterTest::show, (void*)NULL));
		// Phoenix: Wolfspirit: Enabled Show Floater out of viewer menu
		registerFloater("WaterSettings", boost::bind(LLFloaterWater::show), boost::bind(LLFloaterWater::isOpen));
		registerFloater("web", boost::bind(LLFloaterWebContent::showInstance, "dict web", LLFloaterWebContent::Params()));
		registerFloater("Windlight", boost::bind(LLFloaterWindLight::show), boost::bind(LLFloaterWindLight::isOpen));
		registerFloater("world map", boost::bind(LLFloaterWorldMap::toggle));
		registerFloater<LLFloaterLand>					("about land");
		registerFloater<LLFloaterRegionInfo>			("about region");
		registerFloater<LLFloaterActiveSpeakers>		("active speakers");
		registerFloater<JCFloaterAreaSearch>			("areasearch");
		registerFloater<LLFloaterAutoReplaceSettings>	("autoreplace");
		registerFloater<LLFloaterAvatar>				("avatar");
		registerFloater<LLFloaterBeacons>				("beacons");
		registerFloater<LLFloaterCamera>				("camera controls");
		registerFloater<LLFloaterChat>					("chat history");
		registerFloater<LLFloaterChatterBox>			("communicate");
		registerFloater<LLFloaterDestinations>			("destinations");
		registerFloater<LLFloaterMyFriends>				("friends", 0);
		registerFloater<LLFloaterGesture>				("gestures");
		registerFloater<LLFloaterMyFriends>				("groups", 1);
		registerFloater<CommWrapper>					("im");
		registerFloater<LLFloaterInspect>				("inspect");
		registerFloater<LLFloaterJoystick>				("joystick");
		registerFloater<LLFloaterLagMeter>				("lag meter");
		registerFloater<LLFloaterMediaFilter>			("media filter");
		registerFloater<LLFloaterMap>					("mini map");
		registerFloater<LLFloaterMove>					("movement controls");
		registerFloater<LLFloaterMute>					("mute list");
		registerFloater<LLFloaterNotificationConsole>	("notifications console");
		registerFloater<LLFloaterOutbox>				("outbox");
		registerFloater<LLFloaterPathfindingCharacters>	("pathfinding_characters");
		registerFloater<LLFloaterPathfindingLinksets>	("pathfinding_linksets");
		registerFloater<LLFloaterPermsDefault>			("perm prefs");
		registerFloater<LLFloaterAvatarList>			("radar");
		registerFloater<LLFloaterScriptLimits>			("script info");
		registerFloater<LLFloaterStats>					("stat bar");
		registerFloater<LLFloaterTeleportHistory>		("teleport history");
		registerFloater<LLFloaterVoiceEffect>			("voice effect");
		// [RLVa:LF]
		registerFloater<RlvFloaterBehaviours>("rlv restrictions");
		registerFloater<RlvFloaterLocks>("rlv locks");
		registerFloater<RlvFloaterStrings>("rlv strings");
		// [/RLVa:LF]
	}
	template <typename T>
	void registerConsole(const std::string& name, T* console)
	{
		registerFloater(name, boost::bind(&T::setVisible, console, !boost::bind(&T::getVisible, console)), boost::bind(&T::getVisible, console));
	}
	void registerFloater(const std::string& name, boost::function<void ()> show, boost::function<bool ()> visible = NULL)
	{
		mEntries.insert( std::make_pair( name, std::make_pair( show, visible ) ) );
	}
	template <typename T>
	void registerFloater(const std::string& name, const LLSD& key = LLSD())
	{
		registerFloater(name, boost::bind(&T::toggleInstance,key), boost::bind(&T::instanceVisible,key));
	}
};

void show_floater(const std::string& floater_name)
{
	if (floater_name.empty()) return;

	MenuFloaterDict::menu_floater_map_t::iterator it = MenuFloaterDict::instance().mEntries.find(floater_name);
	if (it == MenuFloaterDict::instance().mEntries.end()) // Simple codeless floater
	{
		if (LLFloater* floater = LLUICtrlFactory::getInstance()->getBuiltFloater(floater_name))
			floater->isFrontmost() ? floater->close() : gFloaterView->bringToFront(floater);
		else
			LLUICtrlFactory::getInstance()->buildFloater(new LLFloater(), floater_name);
	}
	else if (it->second.first)
	{
		it->second.first();
	}
}

// Singu TODO: It'd be reallllly nice if we could use this as a control for buttons too.
bool floater_visible(const std::string& floater_name)
{
	MenuFloaterDict::menu_floater_map_t::iterator it = MenuFloaterDict::instance().mEntries.find(floater_name);
	return it != MenuFloaterDict::instance().mEntries.end() && it->second.second && it->second.second();
}
