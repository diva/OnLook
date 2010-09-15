/** 
 * @file hbprefsinert.cpp
 * @author Henri Beauchamp
 * @brief Ascent Viewer preferences panel
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

//File include
#include "ascentprefsvan.h"

//project includes
#include "llcolorswatch.h"
#include "llvoavatar.h"
#include "llagent.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "v4color.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llsavedsettingsglue.h"
#include "llwind.h"
#include "llviewernetwork.h"
#include "pipeline.h"

class LLPrefsAscentVanImpl : public LLPanel
{
public:
	LLPrefsAscentVanImpl();
	/*virtual*/ ~LLPrefsAscentVanImpl() { };

	virtual void refresh();

	void apply();
	void cancel();

private:
	static void onCommitCheckBox(LLUICtrl* ctrl, void* user_data);
	static void onCommitColor(LLUICtrl* ctrl, void* user_data);
	void refreshValues();
	//General
	BOOL mUseAccountSettings;
	//Colors
	BOOL mShowSelfClientTag;
	BOOL mShowSelfClientTagColor;
	BOOL mCustomTagOn;
	std::string mCustomTagLabel;
	LLColor4 mCustomTagColor;
	LLColor4 mEffectColor;
	LLColor4 mFriendColor;
	U32 mSelectedClient;
};


LLPrefsAscentVanImpl::LLPrefsAscentVanImpl()
 : LLPanel(std::string("Ascent"))
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_ascent_vanity.xml");
	childSetCommitCallback("use_account_settings_check", onCommitCheckBox, this);
	childSetCommitCallback("customize_own_tag_check", onCommitCheckBox, this);
	childSetCommitCallback("show_friend_tag_check", onCommitCheckBox, this);

	childSetCommitCallback("custom_tag_color_swatch", onCommitColor, this);
	childSetCommitCallback("effect_color_swatch", onCommitColor, this);

	childSetCommitCallback("X Modifier", LLPrefsAscentVan::onCommitUpdateAvatarOffsets);
	childSetCommitCallback("Y Modifier", LLPrefsAscentVan::onCommitUpdateAvatarOffsets);
	childSetCommitCallback("Z Modifier", LLPrefsAscentVan::onCommitUpdateAvatarOffsets);
	
	refresh();
	
}

void LLPrefsAscentVan::onCommitUpdateAvatarOffsets(LLUICtrl* ctrl, void* userdata)
{
	gAgent.sendAgentSetAppearance();
	//llinfos << llformat("%d,%d,%d",gSavedSettings.getF32("EmeraldAvatarXModifier"),gSavedSettings.getF32("EmeraldAvatarYModifier"),gSavedSettings.getF32("EmeraldAvatarZModifier")) << llendl;
}

void LLPrefsAscentVanImpl::onCommitColor(LLUICtrl* ctrl, void* user_data)
{
	LLPrefsAscentVanImpl* self = (LLPrefsAscentVanImpl*)user_data;

	llinfos << "Control named " << ctrl->getControlName() << " aka " << ctrl->getName() << llendl;

	if (ctrl->getName() == "custom_tag_color_swatch")
	{
		
		llinfos << "Recreating color message for tag update." << llendl;
		LLSavedSettingsGlue::setCOAString("AscentCustomTagLabel",		self->childGetValue("custom_tag_label_box"));
		LLSavedSettingsGlue::setCOAColor4("AscentCustomTagColor",		self->childGetValue("custom_tag_color_swatch"));
		gAgent.sendAgentSetAppearance();
		gAgent.resetClientTag();
	}
}

//static
void LLPrefsAscentVanImpl::onCommitCheckBox(LLUICtrl* ctrl, void* user_data)
{
	
	LLPrefsAscentVanImpl* self = (LLPrefsAscentVanImpl*)user_data;

	llinfos << "Control named " << ctrl->getControlName() << llendl;
	if (ctrl->getControlName() == "AscentStoreSettingsPerAccount")
	{
		self->refresh();
	}

	if (ctrl->getName() == "show_friend_tag_check")
	{
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* avatarp = (LLVOAvatar*) *iter;
			if(avatarp)
			{
				LLVector3 root_pos_last = avatarp->mRoot.getWorldPosition();
				avatarp->mClientTag = "";
			}
		}
	}

	BOOL showCustomOptions;
	showCustomOptions = LLSavedSettingsGlue::getCOABOOL("AscentUseCustomTag");
	self->childSetValue("customize_own_tag_check", showCustomOptions);
	self->childSetEnabled("custom_tag_label_text", showCustomOptions);
	self->childSetEnabled("custom_tag_label_box", showCustomOptions);
	self->childSetEnabled("custom_tag_color_text", showCustomOptions);
	self->childSetEnabled("custom_tag_color_swatch", showCustomOptions);
	if (!gAgent.getID().isNull())
		gAgent.resetClientTag();
}

void LLPrefsAscentVanImpl::refreshValues()
{
	//General
	mUseAccountSettings			= gSavedSettings.getBOOL("AscentStoreSettingsPerAccount");
	
	//Colors
	mShowSelfClientTag			= gSavedSettings.getBOOL("AscentShowSelfTag");
	mShowSelfClientTagColor		= gSavedSettings.getBOOL("AscentShowSelfTagColor");	
	mCustomTagOn				= gSavedSettings.getBOOL("AscentUseCustomTag");
	
	mSelectedClient			= LLSavedSettingsGlue::getCOAU32("AscentReportClientIndex");
	mEffectColor			= LLSavedSettingsGlue::getCOAColor4("EffectColor");
	if (LLSavedSettingsGlue::getCOABOOL("AscentUseCustomTag"))
	{
		childEnable("custom_tag_label_text");
		childEnable("custom_tag_label_box");
		childEnable("custom_tag_color_text");
		childEnable("custom_tag_color_swatch");
	}
	else
	{
		childDisable("custom_tag_label_text");
		childDisable("custom_tag_label_box");
		childDisable("custom_tag_color_text");
		childDisable("custom_tag_color_swatch");
	}
	mCustomTagLabel			= LLSavedSettingsGlue::getCOAString("AscentCustomTagLabel");
	mCustomTagColor			= LLSavedSettingsGlue::getCOAColor4("AscentCustomTagColor");
	mFriendColor			= LLSavedSettingsGlue::getCOAColor4("AscentFriendColor");
}

void LLPrefsAscentVanImpl::refresh()
{
	refreshValues();
	//General --------------------------------------------------------------------------------
	childSetValue("use_account_settings_check", mUseAccountSettings);

	//Colors ---------------------------------------------------------------------------------
	LLComboBox* combo = getChild<LLComboBox>("tag_spoofing_combobox");
	combo->setCurrentByIndex(mSelectedClient);

	childSetValue("show_self_tag_check", mShowSelfClientTag);
	childSetValue("show_self_tag_color_check", mShowSelfClientTagColor);
	childSetValue("customize_own_tag_check", mCustomTagOn);
	childSetValue("custom_tag_label_box", mCustomTagLabel);
	
	getChild<LLColorSwatchCtrl>("effect_color_swatch")->set(mEffectColor);
	getChild<LLColorSwatchCtrl>("custom_tag_color_swatch")->set(mCustomTagColor);
	getChild<LLColorSwatchCtrl>("friend_color_swatch")->set(mFriendColor);
	LLSavedSettingsGlue::setCOAColor4("EffectColor", LLColor4::white);
	LLSavedSettingsGlue::setCOAColor4("EffectColor", mEffectColor);
	LLSavedSettingsGlue::setCOAColor4("AscentFriendColor", LLColor4::yellow);
	LLSavedSettingsGlue::setCOAColor4("AscentFriendColor", mFriendColor);
	gAgent.resetClientTag();
}

void LLPrefsAscentVanImpl::cancel()
{
	//General --------------------------------------------------------------------------------
	childSetValue("use_account_settings_check", mUseAccountSettings);

	//Colors ---------------------------------------------------------------------------------
	LLComboBox* combo = getChild<LLComboBox>("tag_spoofing_combobox");
	combo->setCurrentByIndex(mSelectedClient);

	childSetValue("show_self_tag_check", mShowSelfClientTag);
	childSetValue("show_self_tag_color_check", mShowSelfClientTagColor);
	childSetValue("customize_own_tag_check", mCustomTagOn);
	childSetValue("custom_tag_label_box", mCustomTagLabel);
	
	getChild<LLColorSwatchCtrl>("effect_color_swatch")->set(mEffectColor);
	getChild<LLColorSwatchCtrl>("custom_tag_color_swatch")->set(mCustomTagColor);
	getChild<LLColorSwatchCtrl>("friend_color_swatch")->set(mFriendColor);
	LLSavedSettingsGlue::setCOAColor4("EffectColor", LLColor4::white);
	LLSavedSettingsGlue::setCOAColor4("EffectColor", mEffectColor);
	LLSavedSettingsGlue::setCOAColor4("AscentFriendColor", LLColor4::yellow);
	LLSavedSettingsGlue::setCOAColor4("AscentFriendColor", mFriendColor);
}

void LLPrefsAscentVanImpl::apply()
{
	std::string client_uuid;
	U32 client_index;
	//General -----------------------------------------------------------------------------


	//Colors ------------------------------------------------------------------------------
	LLComboBox* combo = getChild<LLComboBox>("tag_spoofing_combobox");
	if (combo)
	{
		client_index = combo->getCurrentIndex();
		//Don't rebake if it's not neccesary.
		if (client_index != mSelectedClient)
		{
			client_uuid = combo->getSelectedValue().asString();
			LLSavedSettingsGlue::setCOAString("AscentReportClientUUID",  client_uuid);
			LLSavedSettingsGlue::setCOAU32("AscentReportClientIndex",  client_index);
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			if (!avatar) return;

			// Slam pending upload count to "unstick" things
			bool slam_for_debug = true;
			avatar->forceBakeAllTextures(slam_for_debug);
		}
	}
	gSavedSettings.setBOOL("AscentShowSelfTag",			childGetValue("show_self_tag_check"));
	gSavedSettings.setBOOL("AscentShowSelfTagColor",	childGetValue("show_self_tag_color_check"));

	LLSavedSettingsGlue::setCOAColor4("EffectColor",			childGetValue("effect_color_swatch"));
	LLSavedSettingsGlue::setCOAColor4("AscentFriendColor",		childGetValue("friend_color_swatch"));
	LLSavedSettingsGlue::setCOABOOL("AscentUseCustomTag",		childGetValue("customize_own_tag_check"));
	LLSavedSettingsGlue::setCOAString("AscentCustomTagLabel",	childGetValue("custom_tag_label_box"));
	LLSavedSettingsGlue::setCOAColor4("AscentCustomTagColor",	childGetValue("custom_tag_color_swatch"));
	
	refreshValues();
}

//---------------------------------------------------------------------------

LLPrefsAscentVan::LLPrefsAscentVan()
:	impl(* new LLPrefsAscentVanImpl())
{
}

LLPrefsAscentVan::~LLPrefsAscentVan()
{
	delete &impl;
}

void LLPrefsAscentVan::apply()
{
	impl.apply();
}

void LLPrefsAscentVan::cancel()
{
	impl.cancel();
}

LLPanel* LLPrefsAscentVan::getPanel()
{
	return &impl;
}
