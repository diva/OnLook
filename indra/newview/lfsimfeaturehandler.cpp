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
		gAgent.addRegionChangedCallback(boost::bind(&LFSimFeatureHandler::handleRegionChange, this));
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

template<typename T>
void has_feature_or_default(SignaledType<T>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? static_cast<T>(features[feature]) : type.getDefault();
}
template<>
void has_feature_or_default(SignaledType<U32>& type, const LLSD& features, const std::string& feature)
{
	type = (features.has(feature)) ? features[feature].asInteger() : type.getDefault();
}

void LFSimFeatureHandler::setSupportedFeatures()
{
	if (LLViewerRegion* region = gAgent.getRegion())
	{
		LLSD info;
		region->getSimulatorFeatures(info);
		//bool hg(); // Singu Note: There should probably be a flag for this some day.
		if (info.has("OpenSimExtras")) // OpenSim specific sim features
		{
			// For definition of OpenSimExtras please see
			// http://opensimulator.org/wiki/SimulatorFeatures_Extras
			const LLSD& extras(info["OpenSimExtras"]);
			has_feature_or_default(mSupportsExport, extras, "ExportSupported");
			//if (hg)
			{
				has_feature_or_default(mDestinationGuideURL, extras, "destination-guide-url");
				mMapServerURL = extras.has("map-server-url") ? extras["map-server-url"].asString() : "";
				has_feature_or_default(mSearchURL, extras, "search-server-url");
				if (extras.has("GridName"))
				{
					const std::string& grid_name(extras["GridName"]);
					mGridName = gHippoGridManager->getConnectedGrid()->getGridName() != grid_name ? grid_name : "";
				}
			}
			has_feature_or_default(mSayRange, extras, "say-range");
			has_feature_or_default(mShoutRange, extras, "shout-range");
			has_feature_or_default(mWhisperRange, extras, "whisper-range");
		}
		else // OpenSim specifics are unsupported reset all to default
		{
			mSupportsExport.reset();
			//if (hg)
			{
				mDestinationGuideURL.reset();
				mMapServerURL = "";
				mSearchURL.reset();
				mGridName.reset();
			}
			mSayRange.reset();
			mShoutRange.reset();
			mWhisperRange.reset();
		}
	}
}
