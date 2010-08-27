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
	U32 mSelectedClient;
};


LLPrefsAscentVanImpl::LLPrefsAscentVanImpl()
 : LLPanel(std::string("Ascent"))
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_ascent_vanity.xml");
	childSetCommitCallback("use_account_settings_check", onCommitCheckBox, this);
	childSetCommitCallback("customize_own_tag_check", onCommitCheckBox, this);

	
	refresh();
	
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
	else
	{
		if (gSavedSettings.getBOOL("AscentUseCustomTag"))
		{
			self->childEnable("custom_tag_label_text");
			self->childEnable("custom_tag_label_box");
			self->childEnable("custom_tag_color_text");
			self->childEnable("custom_tag_color_swatch");
		}
		else
		{
			self->childDisable("custom_tag_label_text");
			self->childDisable("custom_tag_label_box");
			self->childDisable("custom_tag_color_text");
			self->childDisable("custom_tag_color_swatch");
		}
	}
}

void LLPrefsAscentVanImpl::refreshValues()
{
	//General
	mUseAccountSettings			= gSavedSettings.getBOOL("AscentStoreSettingsPerAccount");
	
	//Colors
	mShowSelfClientTag			= gSavedSettings.getBOOL("AscentShowSelfTag");
	mShowSelfClientTagColor		= gSavedSettings.getBOOL("AscentShowSelfTagColor");	
	
	
	if (!gSavedSettings.getBOOL("AscentStoreSettingsPerAccount"))
	{
		mSelectedClient			= gSavedSettings.getU32("AscentSpoofClientIndex");
		mEffectColor			= gSavedSettings.getColor4("EffectColor");
		if (gSavedSettings.getBOOL("AscentUseCustomTag"))
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
		mCustomTagLabel			= gSavedSettings.getString("AscentCustomTagLabel");
		mCustomTagColor			= gSavedSettings.getColor4("AscentCustomTagColor");
	}
	else
	{
		mSelectedClient			= gSavedPerAccountSettings.getU32("AscentSpoofClientIndex");
		mEffectColor			= gSavedPerAccountSettings.getColor4("EffectColor");
		if (gSavedPerAccountSettings.getBOOL("AscentUseCustomTag"))
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
		mCustomTagLabel			= gSavedPerAccountSettings.getString("AscentCustomTagLabel");
		mCustomTagColor			= gSavedPerAccountSettings.getColor4("AscentCustomTagColor");
	}
	
	
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
	
	if (!gSavedSettings.getBOOL("AscentStoreSettingsPerAccount"))
	{
		llinfos << "Retrieving color from client" << llendl;
		getChild<LLColorSwatchCtrl>("effect_color_swatch")->set(gSavedSettings.getColor4("EffectColor"));
		gSavedSettings.setColor4("EffectColor", LLColor4::white);
		gSavedSettings.setColor4("EffectColor", mEffectColor);
	}
	else
	{
		llinfos << "Retrieving color from account" << llendl;
		getChild<LLColorSwatchCtrl>("effect_color_swatch")->set(gSavedPerAccountSettings.getColor4("EffectColor"));
		gSavedPerAccountSettings.setColor4("EffectColor", LLColor4::white);
		gSavedPerAccountSettings.setColor4("EffectColor", mEffectColor);
	}
}

void LLPrefsAscentVanImpl::cancel()
{
	
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
			if (!gSavedSettings.getBOOL("AscentStoreSettingsPerAccount"))
			{
				gSavedSettings.setString("AscentSpoofClientUUID",  client_uuid);
				gSavedSettings.setU32("AscentSpoofClientIndex",  client_index);
			}
			else
			{
				gSavedPerAccountSettings.setString("AscentSpoofClientUUID",  client_uuid);
				gSavedPerAccountSettings.setU32("AscentSpoofClientIndex",  client_index);
			}
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			if (!avatar) return;

			// Slam pending upload count to "unstick" things
			bool slam_for_debug = true;
			avatar->forceBakeAllTextures(slam_for_debug);
		}
	}
	gSavedSettings.setBOOL("AscentShowSelfTag",			childGetValue("show_self_tag_check"));
	gSavedSettings.setBOOL("AscentShowSelfTagColor",	childGetValue("show_self_tag_color_check"));

	if (!gSavedSettings.getBOOL("AscentStoreSettingsPerAccount"))
	{
		llinfos << "Storing color in client" << llendl;
		gSavedSettings.setColor4("EffectColor",				childGetValue("effect_color_swatch"));
		gSavedSettings.setBOOL("AscentUseCustomTag",		childGetValue("customize_own_tag_check"));
		gSavedSettings.setString("AscentCustomTagLabel",		childGetValue("custom_tag_label_box"));
		gSavedSettings.setColor4("AscentCustomTagColor",	childGetValue("custom_tag_color_swatch"));
	}
	else
	{
		llinfos << "Storing color in account" << llendl;
		gSavedPerAccountSettings.setColor4("EffectColor",			childGetValue("effect_color_swatch"));
		gSavedPerAccountSettings.setBOOL("AscentUseCustomTag",		childGetValue("customize_own_tag_check"));
		gSavedPerAccountSettings.setString("AscentCustomTagLabel",	childGetValue("custom_tag_label_box"));
		gSavedPerAccountSettings.setColor4("AscentCustomTagColor",	childGetValue("custom_tag_color_swatch"));
	}

	

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
