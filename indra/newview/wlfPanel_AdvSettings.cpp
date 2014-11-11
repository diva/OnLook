/* Copyright (c) 2010
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "wlfPanel_AdvSettings.h"

#include "llbutton.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "lliconctrl.h"
#include "lloverlaybar.h"
#include "lltextbox.h"
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "llstartup.h"

#include "llfloaterwindlight.h"
#include "llfloaterwater.h"

#include "llagentcamera.h"
#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llwaterparammanager.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"

wlfPanel_AdvSettings::wlfPanel_AdvSettings() : mExpanded(false)
{
	setVisible(false);
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	mCommitCallbackRegistrar.add("Wlf.ChangeCameraPreset", boost::bind(&wlfPanel_AdvSettings::onChangeCameraPreset, this, _1, _2));
	if(rlv_handler_t::isEnabled())
		gRlvHandler.setBehaviourToggleCallback(boost::bind(&wlfPanel_AdvSettings::onRlvBehaviorChange, this, _1, _2));
}

//static
void wlfPanel_AdvSettings::updateClass()
{
	if (!instanceExists()) return;
	instance().build();
}

void wlfPanel_AdvSettings::build()
{
	mConnections.clear();
	deleteAllChildren();
	mExpanded = gSavedSettings.getBOOL("wlfAdvSettingsPopup");
	LLUICtrlFactory::instance().buildPanel(this, mExpanded ? "wlfPanel_AdvSettings_expanded.xml" : "wlfPanel_AdvSettings.xml", &getFactoryMap());
}

// [RLVa:KB] - Checked: 2013-06-20
void wlfPanel_AdvSettings::updateRlvVisibility()
{
	if (!mExpanded || !rlv_handler_t::isEnabled())
		return;

	bool enable = !gRlvHandler.hasBehaviour(RLV_BHVR_SETENV);
	childSetEnabled("use_estate_wl", enable);
	childSetEnabled("EnvAdvancedWaterButton", enable);
	mWaterPresetCombo->setEnabled(enable);
	childSetEnabled("WWprev", enable);
	childSetEnabled("WWnext", enable);
	childSetEnabled("EnvAdvancedSkyButton", enable);
	mSkyPresetCombo->setEnabled(enable);
	childSetEnabled("WLprev", enable);
	childSetEnabled("WLnext", enable);
	mTimeSlider->setEnabled(enable);
}

void wlfPanel_AdvSettings::onRlvBehaviorChange(ERlvBehaviour eBhvr, ERlvParamType eType)
{
	if (eBhvr == RLV_BHVR_SETENV) updateRlvVisibility();
}
// [/RLVa:KB]

void wlfPanel_AdvSettings::refreshLists()
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();

	// Populate the combo boxes with appropriate lists of available presets.
	//actually, dont do this, its laggy and not needed to just refresh the selections
	// populateWaterPresetsList();
	// populateSkyPresetsList();
	// populateDayCyclePresetsList();

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mWaterPresetCombo->selectByValue("Default");
		mSkyPresetCombo->selectByValue("Default");
		//mDayCyclePresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mWaterPresetCombo->selectByValue(env_mgr.getWaterPresetName());
		mSkyPresetCombo->selectByValue(env_mgr.getSkyPresetName());
		//mDayCyclePresetCombo->selectByValue(env_mgr.getDayCycleName());
	}
	
	updateTimeSlider();
}

BOOL wlfPanel_AdvSettings::postBuild()
{
	setVisible(true);
	if (LLUICtrl* ctrl = findChild<LLUICtrl>("expand"))
		ctrl->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onClickExpandBtn, this));

	if (mExpanded)
	{
		getChild<LLCheckBoxCtrl>("use_estate_wl")->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onUseRegionSettings, this, _2));

		mWaterPresetCombo = getChild<LLComboBox>("WLWaterPresetsCombo");
		mWaterPresetCombo->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onChangeWWPresetName, this, _2));

		mSkyPresetCombo = getChild<LLComboBox>("WLSkyPresetsCombo");
		mSkyPresetCombo->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onChangeWLPresetName, this, _2));

		// mDayCyclePresetCombo = getChild<LLComboBox>("DCPresetsCombo");
		// mDayCyclePresetCombo->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onChangeDCPresetName, this, _2));

		mConnections.push_front(new boost::signals2::scoped_connection(LLEnvManagerNew::instance().setPreferencesChangeCallback(boost::bind(&wlfPanel_AdvSettings::refreshLists, this))));
		mConnections.push_front(new boost::signals2::scoped_connection(LLWaterParamManager::getInstance()->setPresetListChangeCallback(boost::bind(&wlfPanel_AdvSettings::populateWaterPresetsList, this))));
		mConnections.push_front(new boost::signals2::scoped_connection(LLWLParamManager::getInstance()->setPresetListChangeCallback(boost::bind(&wlfPanel_AdvSettings::populateSkyPresetsList, this))));
		// LLDayCycleManager::instance().setModifyCallback(boost::bind(&wlfPanel_AdvSettings::populateDayCyclePresetsList, this));

		populateWaterPresetsList();
		populateSkyPresetsList();
		//populateDayCyclePresetsList();

		// next/prev buttons
		getChild<LLUICtrl>("WWnext")->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onClickWWNext, this));
		getChild<LLUICtrl>("WWprev")->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onClickWWPrev, this));
		getChild<LLUICtrl>("WLnext")->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onClickWLNext, this));
		getChild<LLUICtrl>("WLprev")->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onClickWLPrev, this));

		getChild<LLUICtrl>("EnvAdvancedSkyButton")->setCommitCallback(boost::bind(LLFloaterWindLight::show));
		getChild<LLUICtrl>("EnvAdvancedWaterButton")->setCommitCallback(boost::bind(LLFloaterWater::show));

		mTimeSlider = getChild<LLSliderCtrl>("EnvTimeSlider");
		mTimeSlider->setCommitCallback(boost::bind(&wlfPanel_AdvSettings::onChangeDayTime, this, _2));
		updateTimeSlider();
		updateRlvVisibility();

		const U32 preset(gSavedSettings.getU32("CameraPreset"));
		if (preset == CAMERA_PRESET_REAR_VIEW)
			getChildView("Rear")->setValue(true);
		else if (preset == CAMERA_PRESET_FRONT_VIEW)
			getChildView("Front")->setValue(true);
		else if (preset == CAMERA_PRESET_GROUP_VIEW)
			getChildView("Group")->setValue(true);
	}
	return TRUE;
}

void wlfPanel_AdvSettings::draw()
{
	refresh();
	LLPanel::draw();
}

wlfPanel_AdvSettings::~wlfPanel_AdvSettings ()
{
}

void wlfPanel_AdvSettings::onClickExpandBtn()
{
	LLControlVariable* ctrl = gSavedSettings.getControl("wlfAdvSettingsPopup");
	ctrl->set(!ctrl->get());
}

void wlfPanel_AdvSettings::onChangeCameraPreset(LLUICtrl* ctrl, const LLSD& param)
{
	if (!ctrl->getValue()) // One of these must be set at all times
	{
		ctrl->setValue(true);
		return;
	}

	// 0 is rear, 1 is front, 2 is group
	const ECameraPreset preset((param.asInteger() == 0) ? CAMERA_PRESET_REAR_VIEW : (param.asInteger() == 1) ? CAMERA_PRESET_FRONT_VIEW : CAMERA_PRESET_GROUP_VIEW);
	// Turn off the other preset indicators
	if (preset != CAMERA_PRESET_REAR_VIEW) getChildView("Rear")->setValue(false);
	if (preset != CAMERA_PRESET_FRONT_VIEW) getChildView("Front")->setValue(false);
	if (preset != CAMERA_PRESET_GROUP_VIEW) getChildView("Group")->setValue(false);
	// Actually switch the camera
	gAgentCamera.switchCameraPreset(preset);
}

void wlfPanel_AdvSettings::onUseRegionSettings(const LLSD& value)
{
	LLEnvManagerNew::instance().setUseRegionSettings(value.asBoolean(), gSavedSettings.getBOOL("PhoenixInterpolateSky"));
}

void wlfPanel_AdvSettings::onChangeWWPresetName(const LLSD& value)
{
	const std::string& wwset = value.asString();
	if (wwset.empty()) return;
	if (LLWaterParamManager::getInstance()->hasParamSet(wwset))
	{
		LLEnvManagerNew::instance().setUseWaterPreset(wwset, gSavedSettings.getBOOL("PhoenixInterpolateWater"));
	}
	else
	{
		//if that failed, use region's
		// LLEnvManagerNew::instance().useRegionWater();
		LLEnvManagerNew::instance().setUseWaterPreset("Default", gSavedSettings.getBOOL("PhoenixInterpolateWater"));
	}
}

void wlfPanel_AdvSettings::onChangeWLPresetName(const LLSD& value)
{
	const std::string& wlset = value.asString();
	if (wlset.empty()) return;
	const LLWLParamKey key(wlset, LLEnvKey::SCOPE_LOCAL);
	if (LLWLParamManager::getInstance()->hasParamSet(key))
	{
		LLEnvManagerNew::instance().setUseSkyPreset(wlset, gSavedSettings.getBOOL("PhoenixInterpolateSky"));
	}
	else
	{
		//if that failed, use region's
		// LLEnvManagerNew::instance().useRegionSky();
		LLEnvManagerNew::instance().setUseSkyPreset("Default", gSavedSettings.getBOOL("PhoenixInterpolateSky"));
	}
}

void wlfPanel_AdvSettings::onClickWWNext()
{
	S32 index = mWaterPresetCombo->getCurrentIndex();
	++index;
	if (index == mWaterPresetCombo->getItemCount())
		index = 0;
	mWaterPresetCombo->setCurrentByIndex(index);

	onChangeWWPresetName(mWaterPresetCombo->getSelectedValue());
}

void wlfPanel_AdvSettings::onClickWWPrev()
{
	S32 index = mWaterPresetCombo->getCurrentIndex();
	if (index == 0)
		index = mWaterPresetCombo->getItemCount();
	--index;
	mWaterPresetCombo->setCurrentByIndex(index);

	onChangeWWPresetName(mWaterPresetCombo->getSelectedValue());
}

void wlfPanel_AdvSettings::onClickWLNext()
{
	S32 index = mSkyPresetCombo->getCurrentIndex();
	++index;
	if (index == mSkyPresetCombo->getItemCount())
		index = 0;
	mSkyPresetCombo->setCurrentByIndex(index);

	onChangeWLPresetName(mSkyPresetCombo->getSelectedValue());
}

void wlfPanel_AdvSettings::onClickWLPrev()
{
	S32 index = mSkyPresetCombo->getCurrentIndex();
	if (index == 0)
		index = mSkyPresetCombo->getItemCount();
	--index;
	mSkyPresetCombo->setCurrentByIndex(index);

	onChangeWLPresetName(mSkyPresetCombo->getSelectedValue());
}

void wlfPanel_AdvSettings::onChangeDayTime(const LLSD& value)
{
	// deactivate animator
	LLWLParamManager& inst(LLWLParamManager::instance());
	inst.mAnimator.deactivate();

	F32 val = value.asFloat() + 0.25f;
	if(val > 1.0)
	{
		val--;
	}

	inst.mAnimator.setDayTime((F64)val);
	inst.mAnimator.update(inst.mCurParams);
}

void wlfPanel_AdvSettings::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();

	std::list<std::string> presets;
	LLWaterParamManager::getInstance()->getPresetNames(presets);

	for (std::list<std::string>::const_iterator it = presets.begin(); it != presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mWaterPresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mWaterPresetCombo->selectByValue(LLEnvManagerNew::instance().getWaterPresetName());
	}
}

void wlfPanel_AdvSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t local_presets;
	LLWLParamManager::getInstance()->getLocalPresetNames(local_presets);

	for (LLWLParamManager::preset_name_list_t::const_iterator it = local_presets.begin(); it != local_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	//populate the combos with "Default" if using region settings
	if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	{
		mSkyPresetCombo->selectByValue("Default");
	}
	else
	{
		// Select the current presets in combo boxes.
		mSkyPresetCombo->selectByValue(LLEnvManagerNew::instance().getSkyPresetName());
	}
}

// void wlfPanel_AdvSettings::populateDayCyclePresetsList()
// {
	// mDayCyclePresetCombo->removeall();

	// LLDayCycleManager::preset_name_list_t user_days, sys_days;
	// LLDayCycleManager::instance().getPresetNames(user_days, sys_days);

	// // Add user days.
	// for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
	// {
		// mDayCyclePresetCombo->add(*it);
	// }

	// if (user_days.size() > 0)
	// {
		// mDayCyclePresetCombo->addSeparator();
	// }

	// // Add system days.
	// for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
	// {
		// mDayCyclePresetCombo->add(*it);
	// }

	//populate the combos with "Default" if using region settings
	// if (gSavedSettings.getBOOL("UseEnvironmentFromRegion"))
	// {
		// mDayCyclePresetCombo->selectByValue("Default");
	// }
	// else
	// {
		// Select the current presets in combo boxes.
		// mDayCyclePresetCombo->selectByValue(LLEnvManagerNew::instance().getDayCycleName());
	// }
// }

void wlfPanel_AdvSettings::updateTimeSlider()
{
	
	F32 val = LLWLParamManager::getInstance()->mAnimator.getDayTime() - 0.25f;
	if(val < 0.0)
	{
		val++;
	}
	mTimeSlider->setValue(val);
}
