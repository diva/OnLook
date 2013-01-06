/** 
 * @file ascentprefsvan.cpp
 * @Ascent Viewer preferences panel
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Henri Beauchamp.
 * Rewritten in its entirety 2010 Hg Beeks. 
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

//File include
#include "ascentprefsvan.h"

//project includes
#include "llaudioengine.h" //For gAudiop
#include "llstreamingaudio.h" //For LLStreamingAudioInterface
#include "llcolorswatch.h"
#include "llvoavatarself.h"
#include "llagent.h"
#include "llfloaterchat.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "v4color.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llwind.h"
#include "llviewernetwork.h"
#include "pipeline.h"


LLPrefsAscentVan::LLPrefsAscentVan()
{
    LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_ascent_vanity.xml");

	childSetVisible("announce_streaming_metadata", gAudiop && gAudiop->getStreamingAudioImpl() && gAudiop->getStreamingAudioImpl()->supportsMetaData());

    childSetCommitCallback("tag_spoofing_combobox", onCommitClientTag, this);

    childSetCommitCallback("show_my_tag_check", onCommitCheckBox, this);
    childSetCommitCallback("show_self_tag_check", onCommitCheckBox, this);
    childSetCommitCallback("show_self_tag_color_check", onCommitCheckBox, this);
    childSetCommitCallback("customize_own_tag_check", onCommitCheckBox, this);
    childSetCommitCallback("show_friend_tag_check", onCommitCheckBox, this);
    childSetCommitCallback("use_status_check", onCommitCheckBox, this);

    childSetCommitCallback("custom_tag_label_box", onCommitTextModified, this);

    childSetCommitCallback("X Modifier", onCommitUpdateAvatarOffsets);
    childSetCommitCallback("Y Modifier", onCommitUpdateAvatarOffsets);
    childSetCommitCallback("Z Modifier", onCommitUpdateAvatarOffsets);

    childSetAction("update_clientdefs", onManualClientUpdate, this);

    refreshValues();
    refresh();
}

LLPrefsAscentVan::~LLPrefsAscentVan()
{
}

//static
void LLPrefsAscentVan::onCommitClientTag(LLUICtrl* ctrl, void* userdata)
{
    std::string client_uuid;
    U32 client_index;

    LLPrefsAscentVan* self = (LLPrefsAscentVan*)userdata;
    LLComboBox* combo = (LLComboBox*)ctrl;

    if (combo)
    {
        client_index = combo->getCurrentIndex();
        //Don't rebake if it's not neccesary.
        if (client_index != self->mSelectedClient)
        {
            client_uuid = combo->getSelectedValue().asString();
            gSavedSettings.setString("AscentReportClientUUID",  client_uuid);
            gSavedSettings.setU32("AscentReportClientIndex",  client_index);

            if (gAgentAvatarp)
            {
                // Slam pending upload count to "unstick" things
                bool slam_for_debug = true;
                gAgentAvatarp->forceBakeAllTextures(slam_for_debug);
            }
        }
    }
}

//static
void LLPrefsAscentVan::onCommitUpdateAvatarOffsets(LLUICtrl* ctrl, void* userdata)
{
    if (!gAgent.getID().isNull())
    {
        gAgent.sendAgentSetAppearance();
    }
}

//static
void LLPrefsAscentVan::onCommitTextModified(LLUICtrl* ctrl, void* userdata)
{
    LLPrefsAscentVan* self = (LLPrefsAscentVan*)userdata;

    if (ctrl->getName() == "custom_tag_label_box")
    {
        gSavedSettings.setString("AscentCustomTagLabel", self->childGetValue("custom_tag_label_box"));
    }
}

//static
void LLPrefsAscentVan::onManualClientUpdate(void* data)
{
	LLChat chat("Definitions already up-to-date.");
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	if(SHClientTagMgr::instance().fetchDefinitions())
	{
		if(SHClientTagMgr::instance().parseDefinitions())
			chat.mText="Client definitons updated.";
		else
			chat.mText="Failed to parse updated definitions.";
	}
	LLFloaterChat::addChat(chat);
}

//static
void LLPrefsAscentVan::onCommitCheckBox(LLUICtrl* ctrl, void* user_data)
{
    LLPrefsAscentVan* self = (LLPrefsAscentVan*)user_data;

//	llinfos << "Control named " << ctrl->getControlName() << llendl;

    if (ctrl->getName() == "use_status_check")
    {
		bool showCustomColors = gSavedSettings.getBOOL("AscentUseStatusColors");
		self->childSetEnabled("friends_color_textbox", showCustomColors);
		bool frColors = gSavedSettings.getBOOL("ColorFriendChat");
		self->childSetEnabled("friend_color_swatch", showCustomColors || frColors);
		bool eoColors = gSavedSettings.getBOOL("ColorEstateOwnerChat");
		self->childSetEnabled("estate_owner_color_swatch", showCustomColors || eoColors);
		bool lindColors = gSavedSettings.getBOOL("ColorLindenChat");
		self->childSetEnabled("linden_color_swatch", showCustomColors || lindColors);
		bool muteColors = gSavedSettings.getBOOL("ColorMutedChat");
		self->childSetEnabled("muted_color_swatch", showCustomColors || muteColors);
    }
    else if (ctrl->getName() == "customize_own_tag_check")
    {
        BOOL showCustomOptions = gSavedSettings.getBOOL("AscentUseCustomTag");
        self->childSetEnabled("custom_tag_label_text", showCustomOptions);
        self->childSetEnabled("custom_tag_label_box", showCustomOptions);
        self->childSetEnabled("custom_tag_color_text", showCustomOptions);
        self->childSetEnabled("custom_tag_color_swatch", showCustomOptions);
    }
}

// Store current settings for cancel
void LLPrefsAscentVan::refreshValues()
{
    //Main -----------------------------------------------------------------------------------
    mUseAccountSettings		= gSavedSettings.getBOOL("AscentStoreSettingsPerAccount");
    mShowTPScreen			= !gSavedSettings.getBOOL("AscentDisableTeleportScreens");
    mPlayTPSound			= gSavedSettings.getBOOL("OptionPlayTpSound");
    mShowLogScreens			= !gSavedSettings.getBOOL("AscentDisableLogoutScreens");
	mDisableChatAnimation   = gSavedSettings.getBOOL("SGDisableChatAnimation");
	mAddNotReplace = gSavedSettings.getBOOL("LiruAddNotReplace");
	mTurnAround = gSavedSettings.getBOOL("TurnAroundWhenWalkingBackwards");
	mAnnounceSnapshots = gSavedSettings.getBOOL("AnnounceSnapshots");
	mAnnounceStreamMetadata = gSavedSettings.getBOOL("AnnounceStreamMetadata");

    //Tags\Colors ----------------------------------------------------------------------------
    mAscentUseTag           = gSavedSettings.getBOOL("AscentUseTag");
    mReportClientUUID       = gSavedSettings.getString("AscentReportClientUUID");
    mSelectedClient			= gSavedSettings.getU32("AscentReportClientIndex");
    mShowSelfClientTag		= gSavedSettings.getBOOL("AscentShowSelfTag");
    mShowSelfClientTagColor = gSavedSettings.getBOOL("AscentShowSelfTagColor");
    mShowFriendsTag         = gSavedSettings.getBOOL("AscentShowFriendsTag");
    mCustomTagOn			= gSavedSettings.getBOOL("AscentUseCustomTag");
    mCustomTagLabel			= gSavedSettings.getString("AscentCustomTagLabel");
    mCustomTagColor			= gSavedSettings.getColor4("AscentCustomTagColor");
    mShowOthersTag          = gSavedSettings.getBOOL("AscentShowOthersTag");
    mShowOthersTagColor     = gSavedSettings.getBOOL("AscentShowOthersTagColor");
    mShowIdleTime           = gSavedSettings.getBOOL("AscentShowIdleTime");
    mUseStatusColors        = gSavedSettings.getBOOL("AscentUseStatusColors");
    mUpdateTagsOnLoad       = gSavedSettings.getBOOL("AscentUpdateTagsOnLoad");
    mEffectColor			= gSavedSettings.getColor4("EffectColor");
    mFriendColor			= gSavedSettings.getColor4("AscentFriendColor");
    mEstateOwnerColor		= gSavedSettings.getColor4("AscentEstateOwnerColor");
    mLindenColor			= gSavedSettings.getColor4("AscentLindenColor");
    mMutedColor				= gSavedSettings.getColor4("AscentMutedColor");
    mCustomColor			= gSavedSettings.getColor4("MoyMiniMapCustomColor");
	mColorFriendChat        = gSavedSettings.getBOOL("ColorFriendChat");
	mColorEOChat            = gSavedSettings.getBOOL("ColorEstateOwnerChat");
	mColorLindenChat        = gSavedSettings.getBOOL("ColorLindenChat");
	mColorMutedChat         = gSavedSettings.getBOOL("ColorMutedChat");
//	mColorCustomChat        = gSavedSettings.getBOOL("ColorCustomChat");

    //Body Dynamics --------------------------------------------------------------------------
    mBreastPhysicsToggle    = gSavedSettings.getBOOL("EmeraldBreastPhysicsToggle");
    mBoobMass               = gSavedSettings.getF32("EmeraldBoobMass");
    mBoobHardness           = gSavedSettings.getF32("EmeraldBoobHardness");
    mBoobVelMax             = gSavedSettings.getF32("EmeraldBoobVelMax");
    mBoobFriction           = gSavedSettings.getF32("EmeraldBoobFriction");
    mBoobVelMin             = gSavedSettings.getF32("EmeraldBoobVelMin");

    mAvatarXModifier        = gSavedSettings.getF32("AscentAvatarXModifier");
    mAvatarYModifier        = gSavedSettings.getF32("AscentAvatarYModifier");
    mAvatarZModifier        = gSavedSettings.getF32("AscentAvatarZModifier");
}

// Update controls based on current settings
void LLPrefsAscentVan::refresh()
{
    //Main -----------------------------------------------------------------------------------

    //Tags\Colors ----------------------------------------------------------------------------
    LLComboBox* combo = getChild<LLComboBox>("tag_spoofing_combobox");
    combo->setCurrentByIndex(mSelectedClient);

    childSetEnabled("friends_color_textbox",     mUseStatusColors);
    childSetEnabled("friend_color_swatch",       mUseStatusColors || mColorFriendChat);
    childSetEnabled("estate_owner_color_swatch", mUseStatusColors || mColorEOChat);
    childSetEnabled("linden_color_swatch",       mUseStatusColors || mColorLindenChat);
    childSetEnabled("muted_color_swatch",        mUseStatusColors || mColorMutedChat);

    childSetEnabled("custom_tag_label_text",   mCustomTagOn);
    childSetEnabled("custom_tag_label_box",    mCustomTagOn);
    childSetValue("custom_tag_label_box", gSavedSettings.getString("AscentCustomTagLabel"));
    childSetEnabled("custom_tag_color_text",   mCustomTagOn);
    childSetEnabled("custom_tag_color_swatch", mCustomTagOn);

    //Body Dynamics --------------------------------------------------------------------------
    childSetEnabled("EmeraldBoobMass",     mBreastPhysicsToggle);
    childSetEnabled("EmeraldBoobHardness", mBreastPhysicsToggle);
    childSetEnabled("EmeraldBoobVelMax",   mBreastPhysicsToggle);
    childSetEnabled("EmeraldBoobFriction", mBreastPhysicsToggle);
    childSetEnabled("EmeraldBoobVelMin",   mBreastPhysicsToggle);
}

// Reset settings to local copy
void LLPrefsAscentVan::cancel()
{
    //Main -----------------------------------------------------------------------------------
    gSavedSettings.setBOOL("AscentStoreSettingsPerAccount", mUseAccountSettings);
    gSavedSettings.setBOOL("AscentDisableTeleportScreens", !mShowTPScreen);
    gSavedSettings.setBOOL("OptionPlayTpSound",             mPlayTPSound);
    gSavedSettings.setBOOL("AscentDisableLogoutScreens",   !mShowLogScreens);
	gSavedSettings.setBOOL("SGDisableChatAnimation",		mDisableChatAnimation);
	gSavedSettings.setBOOL("LiruAddNotReplace", mAddNotReplace);
	gSavedSettings.setBOOL("TurnAroundWhenWalkingBackwards", mTurnAround);
	gSavedSettings.setBOOL("AnnounceSnapshots", mAnnounceSnapshots);
	gSavedSettings.setBOOL("AnnounceStreamMetadata", mAnnounceStreamMetadata);

    //Tags\Colors ----------------------------------------------------------------------------
    gSavedSettings.setBOOL("AscentUseTag",               mAscentUseTag);
    gSavedSettings.setString("AscentReportClientUUID",   mReportClientUUID);
    gSavedSettings.setU32("AscentReportClientIndex",     mSelectedClient);
    gSavedSettings.setBOOL("AscentShowSelfTag",          mShowSelfClientTag);
    gSavedSettings.setBOOL("AscentShowSelfTagColor",     mShowSelfClientTagColor);
    gSavedSettings.setBOOL("AscentShowFriendsTag",       mShowFriendsTag);
    gSavedSettings.setBOOL("AscentUseCustomTag",         mCustomTagOn);
    gSavedSettings.setString("AscentCustomTagLabel",     mCustomTagLabel);
    gSavedSettings.setColor4("AscentCustomTagColor",     mCustomTagColor);
    gSavedSettings.setBOOL("AscentShowOthersTag",        mShowOthersTag);
    gSavedSettings.setBOOL("AscentShowOthersTagColor",   mShowOthersTagColor);
    gSavedSettings.setBOOL("AscentShowIdleTime",         mShowIdleTime);
    gSavedSettings.setBOOL("AscentUseStatusColors",      mUseStatusColors);
    gSavedSettings.setBOOL("AscentUpdateTagsOnLoad",     mUpdateTagsOnLoad);
    gSavedSettings.setColor4("EffectColor",              mEffectColor);
    gSavedSettings.setColor4("AscentFriendColor",        mFriendColor);
    gSavedSettings.setColor4("AscentEstateOwnerColor",   mEstateOwnerColor);
    gSavedSettings.setColor4("AscentLindenColor",        mLindenColor);
    gSavedSettings.setColor4("AscentMutedColor",         mMutedColor);
    gSavedSettings.setColor4("MoyMiniMapCustomColor",    mCustomColor);
    gSavedSettings.setBOOL("ColorFriendChat",            mColorFriendChat);
    gSavedSettings.setBOOL("ColorEstateOwnerChat",       mColorEOChat);
    gSavedSettings.setBOOL("ColorLindenChat",            mColorLindenChat);
    gSavedSettings.setBOOL("ColorMutedChat",             mColorMutedChat);
//	gSavedSettings.setBOOL("ColorCustomChat",            mColorCustomChat);

    //Body Dynamics --------------------------------------------------------------------------
    gSavedSettings.setBOOL("EmeraldBreastPhysicsToggle", mBreastPhysicsToggle);
    gSavedSettings.setF32("EmeraldBoobMass",             mBoobMass);
    gSavedSettings.setF32("EmeraldBoobHardness",         mBoobHardness);
    gSavedSettings.setF32("EmeraldBoobVelMax",           mBoobVelMax);
    gSavedSettings.setF32("EmeraldBoobFriction",         mBoobFriction);
    gSavedSettings.setF32("EmeraldBoobVelMin",           mBoobVelMin);

    gSavedSettings.setF32("AscentAvatarXModifier",       mAvatarXModifier);
    gSavedSettings.setF32("AscentAvatarYModifier",       mAvatarYModifier);
    gSavedSettings.setF32("AscentAvatarZModifier",       mAvatarZModifier);
}

// Update local copy so cancel has no effect
void LLPrefsAscentVan::apply()
{
    refreshValues();
    refresh();
}
