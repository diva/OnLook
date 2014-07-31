/** 
 * @file llfloaterdestinations.h
 * @author Leyla Farazha
 * @brief floater for the destinations guide
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */

#include "llviewerprecompiledheaders.h"

#include "lfsimfeaturehandler.h"
#include "llfloaterdestinations.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"
#include "llweb.h"


LLFloaterDestinations::LLFloaterDestinations(const LLSD& key)
	:	LLFloater(key)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_destinations.xml", NULL, false);
}

LLFloaterDestinations::~LLFloaterDestinations()
{
}

BOOL LLFloaterDestinations::postBuild()
{
	enableResizeCtrls(true, true, false);
	LLMediaCtrl* destinations = getChild<LLMediaCtrl>("destination_guide_contents");
	if (destinations)
	{
		destinations->setErrorPageURL(gSavedSettings.getString("GenericErrorPageURL"));
		std::string url = gSavedSettings.getString("DestinationGuideURL");
		changeURL(destinations, url);
		LFSimFeatureHandler::instance().setDestinationGuideURLCallback(boost::bind(&LLFloaterDestinations::changeURL, this, destinations, _1));
	}
	return TRUE;
}

void LLFloaterDestinations::changeURL(LLMediaCtrl* destinations, const std::string& url)
{
	if (url.empty())
	{
		close();
		return;
	}

	destinations->navigateTo(LLWeb::expandURLSubstitutions(url, LLSD()), "text/html");
}


