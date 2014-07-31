/* Copyright (C) 2013 Liru Færs
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

#include "lfsimfeaturehandler.h"

#include "llagent.h"
#include "llenvmanager.h"
#include "llviewerregion.h"
#include "hippogridmanager.h"

LFSimFeatureHandler::LFSimFeatureHandler()
: mSupportsExport(false)
, mDestinationGuideURL(gSavedSettings.getString("DestinationGuideURL"))
, mSearchURL(gSavedSettings.getString("SearchURL"))
, mSayRange(20)
, mShoutRange(100)
, mWhisperRange(10)
{
	if (!gHippoGridManager->getCurrentGrid()->isSecondLife()) // Remove this line if we ever handle SecondLife sim features
		LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LFSimFeatureHandler::handleRegionChange, this));
}

ExportPolicy LFSimFeatureHandler::exportPolicy() const
{
	return gHippoGridManager->getCurrentGrid()->isSecondLife() ? ep_creator_only : (mSupportsExport ? ep_export_bit : ep_full_perm);
}

void LFSimFeatureHandler::handleRegionChange()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		if (region->getFeaturesReceived())
		{
			setSupportedFeatures();
		}
		else
		{
			region->setFeaturesReceivedCallback(boost::bind(&LFSimFeatureHandler::setSupportedFeatures, this));
		}
	}
}

void LFSimFeatureHandler::setSupportedFeatures()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		LLSD info;
		region->getSimulatorFeatures(info);
		bool hg(!gHippoGridManager->getCurrentGrid()->isAvination()); // Singu Note: There should be a flag for this some day.
		static bool init(false); // This could be a grid where OpenSimExtras aren't implemented, in that case, don't alter the hg specific members.
		if (info.has("OpenSimExtras")) // OpenSim specific sim features
		{
			// For definition of OpenSimExtras please see
			// http://opensimulator.org/wiki/SimulatorFeatures_Extras
			const LLSD& extras(info["OpenSimExtras"]);
			mSupportsExport = extras.has("ExportSupported") ? extras["ExportSupported"].asBoolean() : false;
			if (hg)
			{
				mDestinationGuideURL = extras.has("destination-guide-url") ? extras["destination-guide-url"].asString() : "";
				mMapServerURL = extras.has("map-server-url") ? extras["map-server-url"].asString() : "";
				mSearchURL = extras.has("search-server-url") ? extras["search-server-url"].asString() : "";
				init = true;
			}
			mSayRange = extras.has("say-range") ? extras["say-range"].asInteger() : 20;
			mShoutRange = extras.has("shout-range") ? extras["shout-range"].asInteger() : 100;
			mWhisperRange = extras.has("whisper-range") ? extras["whisper-range"].asInteger() : 10;
		}
		else // OpenSim specifics are unsupported reset all to default
		{
			mSupportsExport = false;
			if (hg && init)
			{
				mMapServerURL = "";
				mSearchURL = "";
			}
			mSayRange = 20;
			mShoutRange = 100;
			mWhisperRange = 10;
		}
	}
}

boost::signals2::connection LFSimFeatureHandler::setSupportsExportCallback(const SignaledType<bool>::slot_t& slot)
{
	return mSupportsExport.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setDestinationGuideURLCallback(const SignaledType<std::string>::slot_t& slot)
{
	return mDestinationGuideURL.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSearchURLCallback(const SignaledType<std::string>::slot_t& slot)
{
	return mSearchURL.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSayRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mSayRange.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setShoutRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mShoutRange.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setWhisperRangeCallback(const SignaledType<U32>::slot_t& slot)
{
	return mWhisperRange.connect(slot);
}
