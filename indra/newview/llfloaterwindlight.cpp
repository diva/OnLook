/** 
 * @file llfloaterwindlight.cpp
 * @brief LLFloaterWindLight class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloaterwindlight.h"

#include "pipeline.h"
#include "llsky.h"

#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llfloaterdaycycle.h"
#include "lltabcontainer.h"
#include "llboost.h"

#include "llagent.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

#include "v4math.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsavedsettingsglue.h"

#include "llwlparamset.h"
#include "llwlparammanager.h"

#undef max

LLFloaterWindLight* LLFloaterWindLight::sWindLight = NULL;

std::set<std::string> LLFloaterWindLight::sDefaultPresets;

static const F32 WL_SUN_AMBIENT_SLIDER_SCALE = 3.0f;
static const F32 WL_BLUE_HORIZON_DENSITY_SCALE = 2.0f;
static const F32 WL_CLOUD_SLIDER_SCALE = 1.0f;

LLFloaterWindLight::LLFloaterWindLight() : LLFloater(std::string("windlight floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_windlight_options.xml");
	
	// add the combo boxes
	mSkyPresetCombo = getChild<LLComboBox>("WLPresetsCombo");

	if(mSkyPresetCombo != NULL) {
		populateSkyPresetsList();
		mSkyPresetCombo->setCommitCallback(onChangePresetName);
	}

	// add the list of presets
	std::string def_days = getString("WLDefaultSkyNames");

	// no editing or deleting of the blank string
	sDefaultPresets.insert("");
	boost_tokenizer tokens(def_days, boost::char_separator<char>(":"));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string tok(*token_iter);
		sDefaultPresets.insert(tok);
	}

	// load it up
	initCallbacks();
}

LLFloaterWindLight::~LLFloaterWindLight()
{
}

void LLFloaterWindLight::initCallbacks(void) {

	// help buttons
	initHelpBtn("WLBlueHorizonHelp", "HelpBlueHorizon");
	initHelpBtn("WLHazeHorizonHelp", "HelpHazeHorizon");
	initHelpBtn("WLBlueDensityHelp", "HelpBlueDensity");
	initHelpBtn("WLHazeDensityHelp", "HelpHazeDensity");

	initHelpBtn("WLDensityMultHelp", "HelpDensityMult");
	initHelpBtn("WLDistanceMultHelp", "HelpDistanceMult");
	initHelpBtn("WLMaxAltitudeHelp", "HelpMaxAltitude");

	initHelpBtn("WLSunlightColorHelp", "HelpSunlightColor");
	initHelpBtn("WLAmbientHelp", "HelpSunAmbient");
	initHelpBtn("WLSunGlowHelp", "HelpSunGlow");
	initHelpBtn("WLTimeOfDayHelp", "HelpTimeOfDay");
	initHelpBtn("WLEastAngleHelp", "HelpEastAngle");

	initHelpBtn("WLSceneGammaHelp", "HelpSceneGamma");
	initHelpBtn("WLStarBrightnessHelp", "HelpStarBrightness");

	initHelpBtn("WLCloudColorHelp", "HelpCloudColor");
	initHelpBtn("WLCloudDetailHelp", "HelpCloudDetail");
	initHelpBtn("WLCloudDensityHelp", "HelpCloudDensity");
	initHelpBtn("WLCloudCoverageHelp", "HelpCloudCoverage");

	initHelpBtn("WLCloudScaleHelp", "HelpCloudScale");
	initHelpBtn("WLCloudScrollXHelp", "HelpCloudScrollX");
	initHelpBtn("WLCloudScrollYHelp", "HelpCloudScrollY");

	initHelpBtn("WLClassicCloudsHelp", "HelpClassicClouds");

	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	// blue horizon
	childSetCommitCallback("WLBlueHorizonR", onColorControlRMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonG", onColorControlGMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonB", onColorControlBMoved, &param_mgr->mBlueHorizon);
	childSetCommitCallback("WLBlueHorizonI", onColorControlIMoved, &param_mgr->mBlueHorizon);

	// haze density, horizon, mult, and altitude
	childSetCommitCallback("WLHazeDensity", onFloatControlMoved, &param_mgr->mHazeDensity);
	childSetCommitCallback("WLHazeHorizon", onFloatControlMoved, &param_mgr->mHazeHorizon);
	childSetCommitCallback("WLDensityMult", onFloatControlMoved, &param_mgr->mDensityMult);
	childSetCommitCallback("WLMaxAltitude", onFloatControlMoved, &param_mgr->mMaxAlt);

	// blue density
	childSetCommitCallback("WLBlueDensityR", onColorControlRMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityG", onColorControlGMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityB", onColorControlBMoved, &param_mgr->mBlueDensity);
	childSetCommitCallback("WLBlueDensityI", onColorControlIMoved, &param_mgr->mBlueDensity);

	// Lighting
	
	// sunlight
	childSetCommitCallback("WLSunlightR", onColorControlRMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightG", onColorControlGMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightB", onColorControlBMoved, &param_mgr->mSunlight);
	childSetCommitCallback("WLSunlightI", onColorControlIMoved, &param_mgr->mSunlight);

	// glow
	childSetCommitCallback("WLGlowR", onGlowRMoved, &param_mgr->mGlow);
	childSetCommitCallback("WLGlowB", onGlowBMoved, &param_mgr->mGlow);

	// ambient
	childSetCommitCallback("WLAmbientR", onColorControlRMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientG", onColorControlGMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientB", onColorControlBMoved, &param_mgr->mAmbient);
	childSetCommitCallback("WLAmbientI", onColorControlIMoved, &param_mgr->mAmbient);

	// time of day
	childSetCommitCallback("WLSunAngle", onSunMoved, &param_mgr->mLightnorm);
	childSetCommitCallback("WLEastAngle", onSunMoved, &param_mgr->mLightnorm);

	// Clouds

	// Cloud Color
	childSetCommitCallback("WLCloudColorR", onColorControlRMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorG", onColorControlGMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorB", onColorControlBMoved, &param_mgr->mCloudColor);
	childSetCommitCallback("WLCloudColorI", onColorControlIMoved, &param_mgr->mCloudColor);

	// Cloud
	childSetCommitCallback("WLCloudX", onColorControlRMoved, &param_mgr->mCloudMain);
	childSetCommitCallback("WLCloudY", onColorControlGMoved, &param_mgr->mCloudMain);
	childSetCommitCallback("WLCloudDensity", onColorControlBMoved, &param_mgr->mCloudMain);

	// Cloud Detail
	childSetCommitCallback("WLCloudDetailX", onColorControlRMoved, &param_mgr->mCloudDetail);
	childSetCommitCallback("WLCloudDetailY", onColorControlGMoved, &param_mgr->mCloudDetail);
	childSetCommitCallback("WLCloudDetailDensity", onColorControlBMoved, &param_mgr->mCloudDetail);

	// Cloud extras
	childSetCommitCallback("WLCloudCoverage", onFloatControlMoved, &param_mgr->mCloudCoverage);
	childSetCommitCallback("WLCloudScale", onFloatControlMoved, &param_mgr->mCloudScale);
	childSetCommitCallback("WLCloudLockX", onCloudScrollXToggled, NULL);
	childSetCommitCallback("WLCloudLockY", onCloudScrollYToggled, NULL);
	childSetCommitCallback("WLCloudScrollX", onCloudScrollXMoved, NULL);
	childSetCommitCallback("WLCloudScrollY", onCloudScrollYMoved, NULL);
	childSetCommitCallback("WLDistanceMult", onFloatControlMoved, &param_mgr->mDistanceMult);
	childSetCommitCallback("DrawClassicClouds", onCommitControlSetting(gSavedSettings), (void*)"SkyUseClassicClouds");

	// WL Top
	childSetAction("WLDayCycleMenuButton", onOpenDayCycle, NULL);
	
	// Load/save
	//childSetAction("WLLoadPreset", onLoadPreset, mSkyPresetCombo);
	childSetAction("WLNewPreset", onNewPreset, mSkyPresetCombo);
	childSetAction("WLDeletePreset", onDeletePreset, mSkyPresetCombo);
	childSetCommitCallback("WLSavePreset", onSavePreset, this);


	// Dome
	childSetCommitCallback("WLGamma", onFloatControlMoved, &param_mgr->mWLGamma);
	childSetCommitCallback("WLStarAlpha", onStarAlphaMoved, NULL);

	// next/prev buttons
	childSetAction("next", onClickNext, this);
	childSetAction("prev", onClickPrev, this);
}

void LLFloaterWindLight::onClickHelp(void* data)
{
	LLFloaterWindLight* self = LLFloaterWindLight::instance();

	const std::string xml_alert = *(std::string*)data;
	LLNotifications::instance().add(self->contextualNotification(xml_alert));
}

void LLFloaterWindLight::initHelpBtn(const std::string& name, const std::string& xml_alert)
{
	childSetAction(name, onClickHelp, new std::string(xml_alert));
}

bool LLFloaterWindLight::newPromptCallback(const LLSD& notification, const LLSD& response)
{
	std::string text = response["message"].asString();
	S32 option = LLNotification::getSelectedOption(notification, response);

	if(text == "")
	{
		return false;
	}

	if(option == 0)
	{
		LLFloaterDayCycle* sDayCycle = NULL;
		LLComboBox* keyCombo = NULL;
		if(LLFloaterDayCycle::isOpen()) 
		{
			sDayCycle = LLFloaterDayCycle::instance();
			keyCombo = sDayCycle->getChild<LLComboBox>( 
				"WLKeyPresets");
		}

		// add the current parameters to the list
		// see if it's there first

		const LLWLParamKey key(text, LLEnvKey::SCOPE_LOCAL);

		// if not there, add a new one
		if(!LLWLParamManager::getInstance()->hasParamSet(key))
		{
			LLWLParamManager::getInstance()->addParamSet(key,
				LLWLParamManager::getInstance()->mCurParams);
			sWindLight->mSkyPresetCombo->add(text);
			sWindLight->mSkyPresetCombo->sortByName();

			// add a blank to the bottom
			sWindLight->mSkyPresetCombo->selectFirstItem();
			if(sWindLight->mSkyPresetCombo->getSimple() == "")
			{
				sWindLight->mSkyPresetCombo->remove(0);
			}
			sWindLight->mSkyPresetCombo->add(LLStringUtil::null);

			sWindLight->mSkyPresetCombo->selectByValue(text);

			if(LLFloaterDayCycle::isOpen()) 
			{
				keyCombo->add(text);
				keyCombo->sortByName();
			}
			const LLWLParamKey key(text, LLEnvKey::SCOPE_LOCAL);
			LLWLParamManager::getInstance()->savePreset(key);

			LLEnvManagerNew::instance().setUseSkyPreset(text);

		// otherwise, send a message to the user
		} 
		else 
		{
			LLNotifications::instance().add("ExistsSkyPresetAlert");
		}
	}
	return false;
}

void LLFloaterWindLight::syncMenu()
{
	bool err;

	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	LLWLParamSet& cur_params = param_mgr->mCurParams;
	//std::map<std::string, LLVector4> & currentParams = param_mgr->mCurParams.mParamValues;

// [RLVa:KB] - Checked: 2009-07-10 (RLVa-1.0.0g)
	// Fixes LL "bug" (preset name isn't kept synchronized)
	if (mSkyPresetCombo->getSelectedItemLabel() != LLEnvManagerNew::instance().getSkyPresetName())
	{
		mSkyPresetCombo->selectByValue(LLEnvManagerNew::instance().getSkyPresetName());
	}
// [/RLVa:KB]

	// blue horizon
	param_mgr->mBlueHorizon = cur_params.getVector(param_mgr->mBlueHorizon.mName, err);
	setColorSwatch("WLBlueHorizon", param_mgr->mBlueHorizon, WL_BLUE_HORIZON_DENSITY_SCALE);

	// haze density, horizon, mult, and altitude
	param_mgr->mHazeDensity = cur_params.getFloat(param_mgr->mHazeDensity.mName, err);
	childSetValue("WLHazeDensity", (F32) param_mgr->mHazeDensity);
	param_mgr->mHazeHorizon = cur_params.getFloat(param_mgr->mHazeHorizon.mName, err);
	childSetValue("WLHazeHorizon", (F32) param_mgr->mHazeHorizon);
	param_mgr->mDensityMult = cur_params.getFloat(param_mgr->mDensityMult.mName, err);
	childSetValue("WLDensityMult", ((F32) param_mgr->mDensityMult) * param_mgr->mDensityMult.mult);
	param_mgr->mMaxAlt = cur_params.getFloat(param_mgr->mMaxAlt.mName, err);
	childSetValue("WLMaxAltitude", (F32) param_mgr->mMaxAlt);

	// blue density
	param_mgr->mBlueDensity = cur_params.getVector(param_mgr->mBlueDensity.mName, err);
	setColorSwatch("WLBlueDensity", param_mgr->mBlueDensity, WL_BLUE_HORIZON_DENSITY_SCALE);

	// Lighting

	// sunlight
	param_mgr->mSunlight = cur_params.getVector(param_mgr->mSunlight.mName, err);
	setColorSwatch("WLSunlight", param_mgr->mSunlight, WL_SUN_AMBIENT_SLIDER_SCALE);

	// glow
	param_mgr->mGlow = cur_params.getVector(param_mgr->mGlow.mName, err);
	childSetValue("WLGlowR", 2 - param_mgr->mGlow.r / 20.0f);
	childSetValue("WLGlowB", -param_mgr->mGlow.b / 5.0f);
		
	// ambient
	param_mgr->mAmbient = cur_params.getVector(param_mgr->mAmbient.mName, err);
	setColorSwatch("WLAmbient", param_mgr->mAmbient, WL_SUN_AMBIENT_SLIDER_SCALE);

	childSetValue("WLSunAngle", param_mgr->mCurParams.getFloat("sun_angle",err) / F_TWO_PI);
	childSetValue("WLEastAngle", param_mgr->mCurParams.getFloat("east_angle",err) / F_TWO_PI);

	// Clouds

	// Cloud Color
	param_mgr->mCloudColor = cur_params.getVector(param_mgr->mCloudColor.mName, err);
	setColorSwatch("WLCloudColor", param_mgr->mCloudColor, WL_CLOUD_SLIDER_SCALE);

	// Cloud
	param_mgr->mCloudMain = cur_params.getVector(param_mgr->mCloudMain.mName, err);
	childSetValue("WLCloudX", param_mgr->mCloudMain.r);
	childSetValue("WLCloudY", param_mgr->mCloudMain.g);
	childSetValue("WLCloudDensity", param_mgr->mCloudMain.b);

	// Cloud Detail
	param_mgr->mCloudDetail = cur_params.getVector(param_mgr->mCloudDetail.mName, err);
	childSetValue("WLCloudDetailX", param_mgr->mCloudDetail.r);
	childSetValue("WLCloudDetailY", param_mgr->mCloudDetail.g);
	childSetValue("WLCloudDetailDensity", param_mgr->mCloudDetail.b);

	// Cloud extras
	param_mgr->mCloudCoverage = cur_params.getFloat(param_mgr->mCloudCoverage.mName, err);
	param_mgr->mCloudScale = cur_params.getFloat(param_mgr->mCloudScale.mName, err);
	childSetValue("WLCloudCoverage", (F32) param_mgr->mCloudCoverage);
	childSetValue("WLCloudScale", (F32) param_mgr->mCloudScale);

	// cloud scrolling
	bool lockX = !param_mgr->mCurParams.getEnableCloudScrollX();
	bool lockY = !param_mgr->mCurParams.getEnableCloudScrollY();
	childSetValue("WLCloudLockX", lockX);
	childSetValue("WLCloudLockY", lockY);
	childSetValue("DrawClassicClouds", gSavedSettings.getBOOL("SkyUseClassicClouds"));
	
	// disable if locked, enable if not
	if (lockX)
	{
		childDisable("WLCloudScrollX");
	}
	else
	{
		childEnable("WLCloudScrollX");
	}
	if (lockY)
	{
		childDisable("WLCloudScrollY");
	}
	else
	{
		childEnable("WLCloudScrollY");
	}

	// *HACK cloud scrolling is off my an additive of 10
	childSetValue("WLCloudScrollX", param_mgr->mCurParams.getCloudScrollX() - 10.0f);
	childSetValue("WLCloudScrollY", param_mgr->mCurParams.getCloudScrollY() - 10.0f);

	param_mgr->mDistanceMult = cur_params.getFloat(param_mgr->mDistanceMult.mName, err);
	childSetValue("WLDistanceMult", (F32) param_mgr->mDistanceMult);

	// Tweak extras

	param_mgr->mWLGamma = cur_params.getFloat(param_mgr->mWLGamma.mName, err);
	childSetValue("WLGamma", (F32) param_mgr->mWLGamma);

	childSetValue("WLStarAlpha", param_mgr->mCurParams.getStarBrightness());
}

void LLFloaterWindLight::setColorSwatch(const std::string& name, const WLColorControl& from_ctrl, F32 k)
{
	std::string child_name(name);
	LLVector4 color_vec = from_ctrl;
	color_vec/=k;
	
	child_name.push_back('R');
	childSetValue(name.data(), color_vec[0]);
	child_name.replace(child_name.length()-1,1,1,'G');
	childSetValue(child_name, color_vec[1]);
	child_name.replace(child_name.length()-1,1,1,'B');
	childSetValue(child_name, color_vec[2]);
	child_name.replace(child_name.length()-1,1,1,'I');
	childSetValue(child_name, llmax(color_vec.mV[0], color_vec.mV[1], color_vec.mV[2]));
}

// static
LLFloaterWindLight* LLFloaterWindLight::instance()
{
	if (!sWindLight)
	{
		sWindLight = new LLFloaterWindLight();
		sWindLight->open();
		sWindLight->setFocus(TRUE);
	}
	return sWindLight;
}
void LLFloaterWindLight::show()
{
	if (!sWindLight)
	{
		LLFloaterWindLight* windLight = instance();
		windLight->syncMenu();

		// comment in if you want the menu to rebuild each time
		//LLUICtrlFactory::getInstance()->buildFloater(windLight, "floater_windlight_options.xml");
		//windLight->initCallbacks();
	}
	else
	{
		if (sWindLight->getVisible())
		{
			sWindLight->close();
		}
		else
		{
			sWindLight->open();
		}
	}
}

bool LLFloaterWindLight::isOpen()
{
	if (sWindLight != NULL)
	{
		return sWindLight->getVisible();
	}
	return false;
}

// virtual
void LLFloaterWindLight::onClose(bool app_quitting)
{
	if (sWindLight)
	{
		sWindLight->setVisible(FALSE);
	}
}

// color control callbacks
void LLFloaterWindLight::onColorControlRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->r = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->r *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->r *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->r >= color_ctrl->g && color_ctrl->r >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->r;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			sWindLight->childSetValue(name, color_ctrl->r / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if	(color_ctrl->isBlueHorizonOrDensity)
		{
			sWindLight->childSetValue(name, color_ctrl->r / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			sWindLight->childSetValue(name, color_ctrl->r);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlGMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->g = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->g *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->g *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->g >= color_ctrl->r && color_ctrl->g >= color_ctrl->b && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->g;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			sWindLight->childSetValue(name, color_ctrl->g / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			sWindLight->childSetValue(name, color_ctrl->g / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			sWindLight->childSetValue(name, color_ctrl->g);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlBMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	color_ctrl->b = sldr_ctrl->getValueF32();
	if (color_ctrl->isSunOrAmbientColor)
	{
		color_ctrl->b *= WL_SUN_AMBIENT_SLIDER_SCALE;
	}
	if (color_ctrl->isBlueHorizonOrDensity)
	{
		color_ctrl->b *= WL_BLUE_HORIZON_DENSITY_SCALE;
	}

	// move i if it's the max
	if (color_ctrl->b >= color_ctrl->r && color_ctrl->b >= color_ctrl->g && color_ctrl->hasSliderName)
	{
		color_ctrl->i = color_ctrl->b;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		if (color_ctrl->isSunOrAmbientColor)
		{
			sWindLight->childSetValue(name, color_ctrl->b / WL_SUN_AMBIENT_SLIDER_SCALE);
		}
		else if (color_ctrl->isBlueHorizonOrDensity)
		{
			sWindLight->childSetValue(name, color_ctrl->b / WL_BLUE_HORIZON_DENSITY_SCALE);
		}
		else
		{
			sWindLight->childSetValue(name, color_ctrl->b);
		}
	}

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);

	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onColorControlIMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl * color_ctrl = static_cast<WLColorControl *>(userData);

	color_ctrl->i = sldr_ctrl->getValueF32();
	
	// only for sliders where we pass a name
	if(color_ctrl->hasSliderName)
	{
		
		// set it to the top
		F32 maxVal = std::max(std::max(color_ctrl->r, color_ctrl->g), color_ctrl->b);
		
		F32 scale = 1.f;
		if(color_ctrl->isSunOrAmbientColor)
			scale = WL_SUN_AMBIENT_SLIDER_SCALE;
		else if(color_ctrl->isBlueHorizonOrDensity)
			scale = WL_BLUE_HORIZON_DENSITY_SCALE;
		
		F32 iVal = color_ctrl->i * scale;

		// handle if at 0
		if(iVal == 0)
		{
			color_ctrl->r = 0;
			color_ctrl->g = 0;
			color_ctrl->b = 0;
		
		// if all at the start
		// set them all to the intensity
		}
		else if (maxVal == 0)
		{
			color_ctrl->r = iVal;
			color_ctrl->g = iVal;
			color_ctrl->b = iVal;

		}
		else
		{
			// add delta amounts to each
			F32 delta = (iVal - maxVal) / maxVal;
			color_ctrl->r *= (1.0f + delta);
			color_ctrl->g *= (1.0f + delta);
			color_ctrl->b *= (1.0f + delta);
		}

		// set the sliders to the new vals
		std::string child_name(color_ctrl->mSliderName);
		child_name.push_back('R');
		sWindLight->childSetValue(child_name, color_ctrl->r/scale);
		child_name.replace(child_name.length()-1,1,1,'G');
		sWindLight->childSetValue(child_name, color_ctrl->g/scale);
		child_name.replace(child_name.length()-1,1,1,'B');
		sWindLight->childSetValue(child_name, color_ctrl->b/scale);
	}

	// now update the current parameters and send them to shaders
	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

/// GLOW SPECIFIC CODE
void LLFloaterWindLight::onGlowRMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	// scaled by 20
	color_ctrl->r = (2 - sldr_ctrl->getValueF32()) * 20;

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

/// \NOTE that we want NEGATIVE (-) B
void LLFloaterWindLight::onGlowBMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLColorControl* color_ctrl = static_cast<WLColorControl *>(userdata);

	/// \NOTE that we want NEGATIVE (-) B and NOT by 20 as 20 is too big
	color_ctrl->b = -sldr_ctrl->getValueF32() * 5;

	color_ctrl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onFloatControlMoved(LLUICtrl* ctrl, void* userdata)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	WLFloatControl * floatControl = static_cast<WLFloatControl *>(userdata);

	floatControl->x = sldr_ctrl->getValueF32() / floatControl->mult;

	floatControl->update(LLWLParamManager::getInstance()->mCurParams);
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onBoolToggle(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cbCtrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool value = cbCtrl->get();
	(*(static_cast<BOOL *>(userData))) = value;
}


// Lighting callbacks

// time of day
void LLFloaterWindLight::onSunMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sunSldr = sWindLight->getChild<LLSliderCtrl>("WLSunAngle");
	LLSliderCtrl* eastSldr = sWindLight->getChild<LLSliderCtrl>("WLEastAngle");

	WLColorControl * color_ctrl = static_cast<WLColorControl *>(userData);
	
	// get the two angles
	LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

	param_mgr->mCurParams.setSunAngle(F_TWO_PI * sunSldr->getValueF32());
	param_mgr->mCurParams.setEastAngle(F_TWO_PI * eastSldr->getValueF32());

	// set the sun vector
	color_ctrl->r = -sin(param_mgr->mCurParams.getEastAngle()) * 
		cos(param_mgr->mCurParams.getSunAngle());
	color_ctrl->g = sin(param_mgr->mCurParams.getSunAngle());
	color_ctrl->b = cos(param_mgr->mCurParams.getEastAngle()) * 
		cos(param_mgr->mCurParams.getSunAngle());
	color_ctrl->i = 1.f;

	color_ctrl->update(param_mgr->mCurParams);
	param_mgr->propagateParameters();
}

void LLFloaterWindLight::onFloatTweakMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);
	F32 * tweak = static_cast<F32 *>(userData);

	(*tweak) = sldrCtrl->getValueF32();
	LLWLParamManager::getInstance()->propagateParameters();
}

void LLFloaterWindLight::onStarAlphaMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldrCtrl = static_cast<LLSliderCtrl*>(ctrl);

	LLWLParamManager::getInstance()->mCurParams.setStarBrightness(sldrCtrl->getValueF32());
}

void LLFloaterWindLight::onNewPreset(void* userData)
{
	LLNotifications::instance().add("NewSkyPreset", LLSD(), LLSD(), newPromptCallback);
}

void LLFloaterWindLight::onSavePreset(LLUICtrl* ctrl, void* userData)
{
	// don't save the empty name
	if(sWindLight->mSkyPresetCombo->getSelectedItemLabel() == "")
	{
		return;
	}

	if (ctrl->getValue().asString() == "save_inventory_item")
	{

	}
	else
	{
		// check to see if it's a default and shouldn't be overwritten
		std::set<std::string>::iterator sIt = sDefaultPresets.find(
			sWindLight->mSkyPresetCombo->getSelectedItemLabel());
		if(sIt != sDefaultPresets.end() && !gSavedSettings.getBOOL("SkyEditPresets")) 
		{
			LLNotifications::instance().add("WLNoEditDefault");
			return;
		}

		LLWLParamManager::getInstance()->mCurParams.mName =
			sWindLight->mSkyPresetCombo->getSelectedItemLabel();

		LLNotifications::instance().add("WLSavePresetAlert", LLSD(), LLSD(), saveAlertCallback);
	}
}

bool LLFloaterWindLight::saveNotecardCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	// if they choose save, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLWLParamManager * param_mgr = LLWLParamManager::getInstance();
		param_mgr->setParamSet(param_mgr->mCurParams.mName, param_mgr->mCurParams);
		param_mgr->savePresetToNotecard(param_mgr->mCurParams.mName);
	}
	return false;
}

bool LLFloaterWindLight::saveAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	// if they choose save, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLWLParamManager * param_mgr = LLWLParamManager::getInstance();

		param_mgr->setParamSet(param_mgr->mCurParams.mName, param_mgr->mCurParams);
		
		// comment this back in to save to file
		const LLWLParamKey key(param_mgr->mCurParams.mName, LLEnvKey::SCOPE_LOCAL);
		param_mgr->savePreset(key);
	}
	return false;
}

void LLFloaterWindLight::onDeletePreset(void* userData)
{
	if(sWindLight->mSkyPresetCombo->getSelectedValue().asString() == "")
	{
		return;
	}

	LLSD args;
	args["SKY"] = sWindLight->mSkyPresetCombo->getSelectedValue().asString();
	LLNotifications::instance().add("WLDeletePresetAlert", args, LLSD(), 
									boost::bind(&LLFloaterWindLight::deleteAlertCallback, sWindLight, _1, _2));
}

bool LLFloaterWindLight::deleteAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	// if they choose delete, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		LLFloaterDayCycle* day_cycle = NULL;
		LLComboBox* key_combo = NULL;
		LLMultiSliderCtrl* mult_sldr = NULL;

		if(LLFloaterDayCycle::isOpen()) 
		{
			day_cycle = LLFloaterDayCycle::instance();
			key_combo = day_cycle->getChild<LLComboBox>( 
				"WLKeyPresets");
			mult_sldr = day_cycle->getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
		}

		std::string name(mSkyPresetCombo->getSelectedValue().asString());

		// check to see if it's a default and shouldn't be deleted
		std::set<std::string>::iterator sIt = sDefaultPresets.find(name);
		if(sIt != sDefaultPresets.end()) 
		{
			LLNotifications::instance().add("WLNoEditDefault");
			return false;
		}

		LLWLParamManager::getInstance()->removeParamSet(name, true);
		
		// remove and choose another
		S32 new_index = mSkyPresetCombo->getCurrentIndex();

		mSkyPresetCombo->remove(name);
		if(key_combo != NULL) 
		{
			key_combo->remove(name);

			// remove from slider, as well
			day_cycle->deletePreset(name);
		}

		// pick the previously selected index after delete
		if(new_index > 0) 
		{
			new_index--;
		}
		
		if(mSkyPresetCombo->getItemCount() > 0) 
		{
			mSkyPresetCombo->setCurrentByIndex(new_index);

			// If we don't update the name here, we crash on next/prev -- MC
			LLWLParamManager::getInstance()->mCurParams.mName = mSkyPresetCombo->getSelectedValue().asString();
			if (LLWLParamManager::getInstance()->mCurParams.mName.empty())
			{
				LLWLParamManager::getInstance()->mCurParams.mName = "Default";
			}
			LLEnvManagerNew::instance().setUseSkyPreset(LLWLParamManager::getInstance()->mCurParams.mName);
		}
	}
	return false;
}


void LLFloaterWindLight::onChangePresetName(LLUICtrl* ctrl, void * userData)
{
	LLComboBox * combo_box = static_cast<LLComboBox*>(ctrl);
	
	if(combo_box->getSimple() == "")
	{
		return;
	}

	const LLWLParamKey key(combo_box->getSelectedValue().asString(), LLEnvKey::SCOPE_LOCAL);
	if (LLWLParamManager::getInstance()->hasParamSet(key))
	{
		LLEnvManagerNew::instance().setUseSkyPreset(key.name);
	}
	else
	{
		//if that failed, use region's
		// LLEnvManagerNew::instance().useRegionSky();
		LLEnvManagerNew::instance().setUseSkyPreset("Default");
	}
	
	//LL_INFOS("WindLight") << "Current inventory ID: " << LLWLParamManager::getInstance()->mCurParams.mInventoryID << LL_ENDL;
	sWindLight->syncMenu();
}

void LLFloaterWindLight::onOpenDayCycle(void* userData)
{
	LLFloaterDayCycle::show();
}

// Clouds
void LLFloaterWindLight::onCloudScrollXMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);
	// *HACK  all cloud scrolling is off by an additive of 10.
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollX(sldr_ctrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollYMoved(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	// *HACK  all cloud scrolling is off by an additive of 10.
	LLWLParamManager::getInstance()->mCurParams.setCloudScrollY(sldr_ctrl->getValueF32() + 10.0f);
}

void LLFloaterWindLight::onCloudScrollXToggled(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cb_ctrl = static_cast<LLCheckBoxCtrl*>(ctrl);

	bool lock = cb_ctrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollX(!lock);

	LLSliderCtrl* sldr = sWindLight->getChild<LLSliderCtrl>("WLCloudScrollX");

	if (cb_ctrl->get())
	{
		sldr->setEnabled(false);
	} 
	else 
	{
		sldr->setEnabled(true);
	}

}

void LLFloaterWindLight::onCloudScrollYToggled(LLUICtrl* ctrl, void* userData)
{
	LLWLParamManager::getInstance()->mAnimator.deactivate();

	LLCheckBoxCtrl* cb_ctrl = static_cast<LLCheckBoxCtrl*>(ctrl);
	bool lock = cb_ctrl->get();
	LLWLParamManager::getInstance()->mCurParams.setEnableCloudScrollY(!lock);

	LLSliderCtrl* sldr = sWindLight->getChild<LLSliderCtrl>("WLCloudScrollY");

	if (cb_ctrl->get())
	{
		sldr->setEnabled(false);
	} 
	else 
	{
		sldr->setEnabled(true);
	}
}


void LLFloaterWindLight::onClickNext(void* user_data)
{
	S32 index = sWindLight->mSkyPresetCombo->getCurrentIndex();
	index++;
	if (index == sWindLight->mSkyPresetCombo->getItemCount())
		index = 0;
	sWindLight->mSkyPresetCombo->setCurrentByIndex(index);

	LLFloaterWindLight::onChangePresetName(sWindLight->mSkyPresetCombo, sWindLight);
}

void LLFloaterWindLight::onClickPrev(void* user_data)
{
	S32 index = sWindLight->mSkyPresetCombo->getCurrentIndex();
	if (index == 0)
		index = sWindLight->mSkyPresetCombo->getItemCount();
	index--;
	sWindLight->mSkyPresetCombo->setCurrentByIndex(index);

	LLFloaterWindLight::onChangePresetName(sWindLight->mSkyPresetCombo, sWindLight);
}

//static
void LLFloaterWindLight::selectTab(std::string tab_name)
{
	if (!tab_name.empty())
	{
		LLTabContainer* tabs = LLFloaterWindLight::instance()->getChild<LLTabContainer>("WindLight Tabs");
		tabs->selectTabByName(tab_name);
	}
}

void LLFloaterWindLight::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t local_presets;
	LLWLParamManager::getInstance()->getLocalPresetNames(local_presets);

	for (LLWLParamManager::preset_name_list_t::const_iterator it = local_presets.begin(); it != local_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	LLWLParamManager::preset_name_list_t user_presets;
	LLWLParamManager::getInstance()->getUserPresetNames(user_presets);

	for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	mSkyPresetCombo->selectByValue(LLEnvManagerNew::instance().getSkyPresetName());
}
