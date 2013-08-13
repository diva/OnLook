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
		if (info.has("OpenSimExtras")) // OpenSim specific sim features
		{
			// For definition of OpenSimExtras please see
			// http://opensimulator.org/wiki/SimulatorFeatures_Extras
			mSupportsExport = info["OpenSimExtras"].has("ExportSupported") ? info["OpenSimExtras"]["ExportSupported"].asBoolean() : false;
			mMapServerURL = info["OpenSimExtras"].has("map-server-url") ? info["OpenSimExtras"]["map-server-url"].asString() : "";
			mSearchURL = info["OpenSimExtras"].has("search-server-url") ? info["OpenSimExtras"]["search-server-url"].asString() : "";
		}
		else // OpenSim specifics are unsupported reset all to default
		{
			mSupportsExport = false;
			mMapServerURL = "";
			mSearchURL = "";
		}
	}
}

boost::signals2::connection LFSimFeatureHandler::setSupportsExportCallback(const boost::signals2::signal<void()>::slot_type& slot)
{
	return mSupportsExport.connect(slot);
}

boost::signals2::connection LFSimFeatureHandler::setSearchURLCallback(const boost::signals2::signal<void()>::slot_type& slot)
{
	return mSearchURL.connect(slot);
}

