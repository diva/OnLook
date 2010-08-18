/** 
 * @file hbprefsinert.cpp
 * @author Henri Beauchamp
 * @brief Inertia Viewer preferences panel
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Henri Beauchamp.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "hbprefsinert.h"

#include "llstartup.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llwind.h"
#include "llviewernetwork.h"
#include "pipeline.h"

class LLPrefsInertImpl : public LLPanel
{
public:
	LLPrefsInertImpl();
	/*virtual*/ ~LLPrefsInertImpl() { };

	virtual void refresh();

	void apply();
	void cancel();

private:
	static void onCommitCheckBox(LLUICtrl* ctrl, void* user_data);
	void refreshValues();
	BOOL mSaveScriptsAsMono;
	BOOL mDoubleClickTeleport;
	BOOL mHideNotificationsInChat;
	BOOL mPlayTypingSound;
	BOOL mDisablePointAtAndBeam;
	BOOL mPrivateLookAt;
	BOOL mFetchInventoryOnLogin;
	BOOL mSecondsInChatAndIMs;
	BOOL mPreviewAnimInWorld;
	BOOL mSpeedRez;
	BOOL mRevokePermsOnStandUp;
	BOOL mEnableLLWind;
	BOOL mEnableClouds;
	BOOL mInitialEnableClouds;
	BOOL mBroadcastViewerEffects;
	U32 mSpeedRezInterval;
	U32 mLinksForChattingObjects;
	U32 mTimeFormat;
	U32 mDateFormat;
	U32 mSpoofProtectionAtOpen;
};


LLPrefsInertImpl::LLPrefsInertImpl()
 : LLPanel(std::string("Inert Prefs Panel"))
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_inert.xml");
	childSetCommitCallback("speed_rez_check", onCommitCheckBox, this);
	refresh();
	
	mInitialEnableClouds = mEnableClouds;
}

//static
void LLPrefsInertImpl::onCommitCheckBox(LLUICtrl* ctrl, void* user_data)
{
	LLPrefsInertImpl* self = (LLPrefsInertImpl*)user_data;

	self->childEnable("fetch_inventory_on_login_check");	

	if (self->childGetValue("speed_rez_check").asBoolean())
	{
		self->childEnable("speed_rez_interval");
		self->childEnable("speed_rez_seconds");
	}
	else
	{
		self->childDisable("speed_rez_interval");
		self->childDisable("speed_rez_seconds");
	}
}

void LLPrefsInertImpl::refreshValues()
{
	mSaveScriptsAsMono			= gSavedSettings.getBOOL("SaveScriptsAsMono");
	mDoubleClickTeleport		= gSavedSettings.getBOOL("DoubleClickTeleport");
	mHideNotificationsInChat	= gSavedSettings.getBOOL("HideNotificationsInChat");
	mPlayTypingSound			= gSavedSettings.getBOOL("PlayTypingSound");
	mDisablePointAtAndBeam			= gSavedSettings.getBOOL("DisablePointAtAndBeam");
	mPrivateLookAt				= gSavedSettings.getBOOL("PrivateLookAt");
	mSecondsInChatAndIMs		= gSavedSettings.getBOOL("SecondsInChatAndIMs");
	mFetchInventoryOnLogin		= gSavedSettings.getBOOL("FetchInventoryOnLogin");
	mPreviewAnimInWorld			= gSavedSettings.getBOOL("PreviewAnimInWorld");
	mSpeedRez					= gSavedSettings.getBOOL("SpeedRez");
	mSpeedRezInterval			= gSavedSettings.getU32("SpeedRezInterval");
	mLinksForChattingObjects	= gSavedSettings.getU32("LinksForChattingObjects");
	mRevokePermsOnStandUp		= gSavedSettings.getBOOL("RevokePermsOnStandUp");
	mEnableLLWind				= gSavedSettings.getBOOL("WindEnabled");
	mEnableClouds				= gSavedSettings.getBOOL("CloudsEnabled");
	mBroadcastViewerEffects		= gSavedSettings.getBOOL("BroadcastViewerEffects");
}

void LLPrefsInertImpl::refresh()
{
	refreshValues();

	childEnable("fetch_inventory_on_login_check");

	if (mSpeedRez)
	{
		childEnable("speed_rez_interval");
		childEnable("speed_rez_seconds");
	}
	else
	{
		childDisable("speed_rez_interval");
		childDisable("speed_rez_seconds");
	}

	std::string format = gSavedSettings.getString("ShortTimeFormat");
	if (format.find("%p") == -1)
	{
		mTimeFormat = 0;
	}
	else
	{
		mTimeFormat = 1;
	}

	format = gSavedSettings.getString("ShortDateFormat");
	if (format.find("%m/%d/%") != -1)
	{
		mDateFormat = 2;
	}
	else if (format.find("%d/%m/%") != -1)
	{
		mDateFormat = 1;
	}
	else
	{
		mDateFormat = 0;
	}

	// time format combobox
	LLComboBox* combo = getChild<LLComboBox>("time_format_combobox");
	if (combo)
	{
		combo->setCurrentByIndex(mTimeFormat);
	}

	// date format combobox
	combo = getChild<LLComboBox>("date_format_combobox");
	if (combo)
	{
		combo->setCurrentByIndex(mDateFormat);
	}
	mSpoofProtectionAtOpen = gSavedSettings.getU32("SpoofProtectionLevel");
}

void LLPrefsInertImpl::cancel()
{
	gSavedSettings.setBOOL("SaveScriptsAsMono",			mSaveScriptsAsMono);
	gSavedSettings.setBOOL("DoubleClickTeleport",		mDoubleClickTeleport);
	gSavedSettings.setBOOL("HideNotificationsInChat",	mHideNotificationsInChat);
	gSavedSettings.setBOOL("PlayTypingSound",			mPlayTypingSound);
	gSavedSettings.setBOOL("DisablePointAtAndBeam",			mDisablePointAtAndBeam);
	gSavedSettings.setBOOL("PrivateLookAt",				mPrivateLookAt);
	gSavedSettings.setBOOL("FetchInventoryOnLogin",		mFetchInventoryOnLogin);
	gSavedSettings.setBOOL("SecondsInChatAndIMs",		mSecondsInChatAndIMs);
	gSavedSettings.setBOOL("PreviewAnimInWorld",		mPreviewAnimInWorld);
	gSavedSettings.setBOOL("SpeedRez",					mSpeedRez);
	gSavedSettings.setU32("SpeedRezInterval",			mSpeedRezInterval);
	gSavedSettings.setU32("LinksForChattingObjects",	mLinksForChattingObjects);
	gSavedSettings.setBOOL("RevokePermsOnStandUp",		mRevokePermsOnStandUp);
	gSavedSettings.setBOOL("WindEnabled",				mEnableLLWind);
	gSavedSettings.setBOOL("BroadcastViewerEffects",	mBroadcastViewerEffects);
	gSavedSettings.setU32("SpoofProtectionLevel",		mSpoofProtectionAtOpen);

	gLLWindEnabled = mEnableLLWind;
	
	if(mInitialEnableClouds != gSavedSettings.getBOOL("CloudsEnabled"))
	{
		gSavedSettings.setBOOL("CloudsEnabled", mEnableClouds);
		LLPipeline::toggleRenderTypeControl((void*)LLPipeline::RENDER_TYPE_CLOUDS);
	}
}

void LLPrefsInertImpl::apply()
{
	std::string short_date, long_date, short_time, long_time, timestamp;	

	LLComboBox* combo = getChild<LLComboBox>("time_format_combobox");
	if (combo) {
		mTimeFormat = combo->getCurrentIndex();
	}

	combo = getChild<LLComboBox>("date_format_combobox");
	if (combo)
	{
		mDateFormat = combo->getCurrentIndex();
	}

	if (mTimeFormat == 0)
	{
		short_time = "%H:%M";
		long_time  = "%H:%M:%S";
		timestamp  = " %H:%M:%S";
	}
	else
	{
		short_time = "%I:%M %p";
		long_time  = "%I:%M:%S %p";
		timestamp  = " %I:%M %p";
	}

	if (mDateFormat == 0)
	{
		short_date = "%Y-%m-%d";
		long_date  = "%A %d %B %Y";
		timestamp  = "%a %d %b %Y" + timestamp;
	}
	else if (mDateFormat == 1)
	{
		short_date = "%d/%m/%Y";
		long_date  = "%A %d %B %Y";
		timestamp  = "%a %d %b %Y" + timestamp;
	}
	else
	{
		short_date = "%m/%d/%Y";
		long_date  = "%A, %B %d %Y";
		timestamp  = "%a %b %d %Y" + timestamp;
	}

	gSavedSettings.setString("ShortDateFormat",	short_date);
	gSavedSettings.setString("LongDateFormat",	long_date);
	gSavedSettings.setString("ShortTimeFormat",	short_time);
	gSavedSettings.setString("LongTimeFormat",	long_time);
	gSavedSettings.setString("TimestampFormat",	timestamp);
	if(gMessageSystem)
	{
		U32 new_spoof_protection = gSavedSettings.getU32("SpoofProtectionLevel");
		if(new_spoof_protection != mSpoofProtectionAtOpen)
		{
			mSpoofProtectionAtOpen = new_spoof_protection;

			gMessageSystem->stopSpoofProtection();

			if(LLViewerLogin::getInstance()->getGridChoice() < GRID_INFO_LOCAL)
				gMessageSystem->startSpoofProtection(new_spoof_protection);
			else
				gMessageSystem->startSpoofProtection(0);
		}
	}
	refreshValues();
}

//---------------------------------------------------------------------------

LLPrefsInert::LLPrefsInert()
:	impl(* new LLPrefsInertImpl())
{
}

LLPrefsInert::~LLPrefsInert()
{
	delete &impl;
}

void LLPrefsInert::apply()
{
	impl.apply();
}

void LLPrefsInert::cancel()
{
	impl.cancel();
}

LLPanel* LLPrefsInert::getPanel()
{
	return &impl;
}
