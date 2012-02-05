/* Copyright (C) 2012 Siana Gearz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */

#include "llviewerprecompiledheaders.h"

#include "meta7windlight.h"
#include "llenvmanager.h"
#include "llwaterparammanager.h"
#include "llwlparammanager.h"
#include "message_prehash.h"

#include "m7wlinterface.h"

M7WindlightInterface::M7WindlightInterface()
:mHasOverride(false)
{
}

void M7WindlightInterface::receiveMessage(LLMessageSystem* msg)
{
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_ParamList);
	for (S32 i = 0; i < count; ++i)
	{
		// our param is binary data)
		S32 size = msg->getSizeFast(_PREHASH_ParamList, i, _PREHASH_Parameter);
		if (size >= 0)
		{
			mHasOverride = true;
			char buf[250];
			msg->getBinaryDataFast(
			_PREHASH_ParamList, _PREHASH_Parameter,
			buf, size, i, 249);

			LLWaterParamManager::getInstance()->getParamSet("Default", mWater);

			Meta7WindlightPacket* wl = (Meta7WindlightPacket*)buf;
			mWater.set("waterFogColor", wl->waterColor.red / 256.f, wl->waterColor.green / 256.f, wl->waterColor.blue / 256.f);
			mWater.set("waterFogDensity", pow(2.0f, wl->waterFogDensityExponent));
			mWater.set("underWaterFogMod", wl->underwaterFogModifier);
			mWater.set("normScale", wl->reflectionWaveletScale.X,wl->reflectionWaveletScale.Y,wl->reflectionWaveletScale.Z);
			mWater.set("fresnelScale", wl->fresnelScale);
			mWater.set("fresnelOffset", wl->fresnelOffset);
			mWater.set("scaleAbove", wl->refractScaleAbove);
			mWater.set("scaleBelow", wl->refractScaleBelow);
			mWater.set("blurMultiplier", wl->blurMultiplier);
			mWater.set("wave1Dir", wl->littleWaveDirection.X, wl->littleWaveDirection.Y);
			mWater.set("wave2Dir", wl->bigWaveDirection.X, wl->bigWaveDirection.Y);

			std::string out = llformat(
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			(U8)(wl->normalMapTexture[0]),
			(U8)(wl->normalMapTexture[1]),
			(U8)(wl->normalMapTexture[2]),
			(U8)(wl->normalMapTexture[3]),
			(U8)(wl->normalMapTexture[4]),
			(U8)(wl->normalMapTexture[5]),
			(U8)(wl->normalMapTexture[6]),
			(U8)(wl->normalMapTexture[7]),
			(U8)(wl->normalMapTexture[8]),
			(U8)(wl->normalMapTexture[9]),
			(U8)(wl->normalMapTexture[10]),
			(U8)(wl->normalMapTexture[11]),
			(U8)(wl->normalMapTexture[12]),
			(U8)(wl->normalMapTexture[13]),
			(U8)(wl->normalMapTexture[14]),
			(U8)(wl->normalMapTexture[15]));

			mNormalMapTexture.set(out);
			LLWaterParamManager::getInstance()->mCurParams = mWater;
			LLWaterParamManager::getInstance()->setNormalMapID(mNormalMapTexture);
			LLWaterParamManager::getInstance()->propagateParameters();

			LLWLParamManager::getInstance()->mAnimator.deactivate();

			LLWLParamManager::getInstance()->getParamSet(LLWLParamKey("Default", LLWLParamKey::SCOPE_LOCAL), mWindlight);

			mWindlight.setSunAngle(F_TWO_PI * wl->sunMoonPosiiton);
			mWindlight.setEastAngle(F_TWO_PI * wl->eastAngle);
			mWindlight.set("sunlight_color", wl->sunMoonColor.red * 3.0f, wl->sunMoonColor.green * 3.0f, wl->sunMoonColor.blue * 3.0f, wl->sunMoonColor.alpha * 3.0f);
			mWindlight.set("ambient", wl->ambient.red * 3.0f, wl->ambient.green * 3.0f, wl->ambient.blue * 3.0f, wl->ambient.alpha * 3.0f);
			mWindlight.set("blue_horizon", wl->horizon.red * 2.0f, wl->horizon.green *2.0f, wl->horizon.blue * 2.0f, wl->horizon.alpha * 2.0f);
			mWindlight.set("blue_density", wl->blueDensity.red * 2.0f, wl->blueDensity.green * 2.0f, wl->blueDensity.blue * 2.0f, wl->blueDensity.alpha * 2.0f);
			mWindlight.set("haze_horizon", wl->hazeHorizon, wl->hazeHorizon, wl->hazeHorizon, 1.f);
			mWindlight.set("haze_density", wl->hazeDensity, wl->hazeDensity, wl->hazeDensity, 1.f);
			mWindlight.set("cloud_shadow", wl->cloudCoverage, wl->cloudCoverage, wl->cloudCoverage, wl->cloudCoverage);
			mWindlight.set("density_multiplier", wl->densityMultiplier / 1000.0f);
			mWindlight.set("distance_multiplier", wl->distanceMultiplier, wl->distanceMultiplier, wl->distanceMultiplier, wl->distanceMultiplier);
			mWindlight.set("max_y",(F32)wl->maxAltitude);
			mWindlight.set("cloud_color", wl->cloudColor.red, wl->cloudColor.green, wl->cloudColor.blue, wl->cloudColor.alpha);
			mWindlight.set("cloud_pos_density1", wl->cloudXYDensity.X, wl->cloudXYDensity.Y, wl->cloudXYDensity.Z);
			mWindlight.set("cloud_pos_density2", wl->cloudDetailXYDensity.X, wl->cloudDetailXYDensity.Y, wl->cloudDetailXYDensity.Z);
			mWindlight.set("cloud_scale", wl->cloudScale, 0.f, 0.f, 1.f);
			mWindlight.set("gamma", wl->sceneGamma, wl->sceneGamma, wl->sceneGamma, 0.0f);
			mWindlight.set("glow",(2 - wl->sunGlowSize) * 20 , 0.f, -wl->sunGlowFocus * 5);
			mWindlight.setCloudScrollX(wl->cloudScrollX + 10.0f);
			mWindlight.setCloudScrollY(wl->cloudScrollY + 10.0f);
			mWindlight.setEnableCloudScrollX(!wl->cloudScrollXLock);
			mWindlight.setEnableCloudScrollY(!wl->cloudScrollYLock);
			mWindlight.setStarBrightness(wl->starBrightness);

			LLWLParamManager::getInstance()->mCurParams = mWindlight;
			LLWLParamManager::getInstance()->propagateParameters();
		}
	}
}

void M7WindlightInterface::receiveReset()
{
	mHasOverride = false;
	LLEnvManagerNew::getInstance()->usePrefs();
}
