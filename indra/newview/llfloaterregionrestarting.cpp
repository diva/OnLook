/**
 * @file llfloaterregionrestarting.cpp
 * @brief Shows countdown timer during region restart
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llfloaterregionrestarting.h"

#include "lluictrlfactory.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llenvmanager.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

// <singu> For emergency teleports
#include "llinventorymodel.h"
void emergency_teleport()
{
	static const LLCachedControl<std::string> landmark(gSavedPerAccountSettings, "EmergencyTeleportLandmark");
	if (landmark().empty()) return;
	const LLUUID id(landmark);
	if (id.isNull()) return;
	if (LLViewerInventoryItem* item = gInventory.getItem(id))
		gAgent.teleportViaLandmark(item->getAssetUUID());
}
// </singu>

enum shake_state
{
	SHAKE_START,
	SHAKE_LEFT,
	SHAKE_UP,
	SHAKE_RIGHT,
	SHAKE_DOWN,
	SHAKE_DONE
};
static shake_state sShakeState;

LLFloaterRegionRestarting::LLFloaterRegionRestarting(const LLSD& key) :
	LLEventTimer(1)
,	mRestartSeconds(NULL)
,	mSeconds(key["SECONDS"].asInteger())
,	mShakeIterations()
,	mShakeMagnitude()
{
	//buildFromFile("floater_region_restarting.xml");
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_region_restarting.xml");

	LLStringUtil::format_map_t args;
	args["[NAME]"] = key["NAME"].asString();
	getChild<LLTextBox>("region_name")->setValue(getString("RegionName", args));
	mRestartSeconds = getChild<LLTextBox>("restart_seconds");
	center();

	refresh();

	mRegionChangedConnection = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterRegionRestarting::close, this, false));
	if (mSeconds <= 20) emergency_teleport(); // <singu/> For emergency teleports
}

LLFloaterRegionRestarting::~LLFloaterRegionRestarting()
{
	mRegionChangedConnection.disconnect();
}

BOOL LLFloaterRegionRestarting::postBuild()
{
	setBackgroundColor(gColors.getColor("NotifyCautionBoxColor"));
	sShakeState = SHAKE_START;
	return TRUE;
}

BOOL LLFloaterRegionRestarting::tick()
{
	refresh();

	return FALSE;
}

void LLFloaterRegionRestarting::refresh()
{
	LLStringUtil::format_map_t args;
	args["[SECONDS]"] = llformat("%d", mSeconds);
	mRestartSeconds->setValue(getString("RestartSeconds", args));

	if (mSeconds == 20) emergency_teleport(); // <singu/> For emergency teleports
	if (!mSeconds) return; // Zero means we're done.
	--mSeconds;
}

void LLFloaterRegionRestarting::draw()
{
	LLFloater::draw();

        static const LLCachedControl<bool> alchemyRegionShake(gSavedSettings, "AlchemyRegionRestartShake", true);
        if (!alchemyRegionShake)
                return;

	const F32 SHAKE_INTERVAL = 0.025;
	const F32 SHAKE_TOTAL_DURATION = 1.8; // the length of the default alert tone for this
	const F32 SHAKE_INITIAL_MAGNITUDE = 1.5;
	const F32 SHAKE_HORIZONTAL_BIAS = 0.25;
	F32 time_shaking;

	if (SHAKE_START == sShakeState)
	{
		mShakeTimer.setTimerExpirySec(SHAKE_INTERVAL);
		sShakeState = SHAKE_LEFT;
		mShakeIterations = 0;
		mShakeMagnitude = SHAKE_INITIAL_MAGNITUDE;
	}

	if (SHAKE_DONE != sShakeState && mShakeTimer.hasExpired())
	{
		gAgentCamera.unlockView();

		switch(sShakeState)
		{
			case SHAKE_LEFT:
				gAgentCamera.setPanLeftKey(mShakeMagnitude * SHAKE_HORIZONTAL_BIAS);
				sShakeState = SHAKE_UP;
				break;

			case SHAKE_UP:
				gAgentCamera.setPanUpKey(mShakeMagnitude);
				sShakeState = SHAKE_RIGHT;
				break;

			case SHAKE_RIGHT:
				gAgentCamera.setPanRightKey(mShakeMagnitude * SHAKE_HORIZONTAL_BIAS);
				sShakeState = SHAKE_DOWN;
				break;

			case SHAKE_DOWN:
				gAgentCamera.setPanDownKey(mShakeMagnitude);
				mShakeIterations++;
				time_shaking = SHAKE_INTERVAL * (mShakeIterations * 4 /* left, up, right, down */);
				if (SHAKE_TOTAL_DURATION <= time_shaking)
				{
					sShakeState = SHAKE_DONE;
					mShakeMagnitude = 0.0;
				}
				else
				{
					sShakeState = SHAKE_LEFT;
					F32 percent_done_shaking = (SHAKE_TOTAL_DURATION - time_shaking) / SHAKE_TOTAL_DURATION;
					mShakeMagnitude = SHAKE_INITIAL_MAGNITUDE * (percent_done_shaking * percent_done_shaking); // exponential decay
				}
				break;

			default:
				break;
		}
		mShakeTimer.setTimerExpirySec(SHAKE_INTERVAL);
	}
}

void LLFloaterRegionRestarting::onClose(bool app_quitting)
{
	if (sShakeState != SHAKE_DONE && sShakeState != SHAKE_START) // Finish shake if needed
	{
		gAgentCamera.resetView(TRUE, TRUE);
		sShakeState = SHAKE_DONE;
	}
	LLFloater::onClose(app_quitting);
}

void LLFloaterRegionRestarting::updateTime(const U32& time)
{
	mSeconds = time;
	if (mSeconds <= 20) emergency_teleport(); // <singu/> For emergency teleports
	sShakeState = SHAKE_START;
}
