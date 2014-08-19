/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Jonathan Yap
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "lfsimfeaturehandler.h"
#include "llagent.h"
#include "llcheckboxctrl.h"
#include "llfloaterperms.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"
#include "hippogridmanager.h"

extern class AIHTTPTimeoutPolicy floaterPermsResponder_timeout;

//static
U32 LLFloaterPerms::getGroupPerms(std::string prefix)
{
	return gSavedSettings.getBOOL(prefix+"ShareWithGroup") ? PERM_COPY | PERM_MOVE | PERM_MODIFY : PERM_NONE;
}

//static
U32 LLFloaterPerms::getEveryonePerms(std::string prefix)
{
	U32 flags = PERM_NONE;
	if (prefix != "Bulk" && LFSimFeatureHandler::instance().simSupportsExport() && prefix.empty() && gSavedPerAccountSettings.getBOOL(prefix+"EveryoneExport")) // Singu TODO: Bulk?
		flags |= PERM_EXPORT;
	if (gSavedSettings.getBOOL(prefix+"EveryoneCopy"))
		flags |= PERM_COPY;
	return flags;
}

//static
U32 LLFloaterPerms::getNextOwnerPermsInverted(std::string prefix)
{
	// Sets bits for permissions that are off
	U32 flags = PERM_MOVE;
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerCopy"))
	{
		flags |= PERM_COPY;
	}
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerModify"))
	{
		flags |= PERM_MODIFY;
	}
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerTransfer"))
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

//static
U32 LLFloaterPerms::getNextOwnerPerms(std::string prefix)
{
	U32 flags = PERM_MOVE;
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

namespace
{
	void handle_checkboxes(LLView* view, const std::string& ctrl_name, const LLSD& value, const std::string& type)
	{
		if (ctrl_name == type+"everyone_export")
		{
			view->getChildView(type+"next_owner_copy")->setEnabled(!value);
			view->getChildView(type+"next_owner_modify")->setEnabled(!value);
			view->getChildView(type+"next_owner_transfer")->setEnabled(!value);
		}
		else
		{
			if (ctrl_name == type+"next_owner_copy")
			{
				if (!value) // Implements fair use
					gSavedSettings.setBOOL(type+"NextOwnerTransfer", true);
				view->getChildView(type+"next_owner_transfer")->setEnabled(value);
			}
			if (!value) // If any of these are unchecked, export can no longer be checked.
				view->getChildView(type+"everyone_export")->setEnabled(false);
			else
				view->getChildView(type+"everyone_export")->setEnabled(LFSimFeatureHandler::instance().simSupportsExport() && (LLFloaterPerms::getNextOwnerPerms(type) & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED);
		}
	}
}

static bool mCapSent = false;

LLFloaterPermsDefault::LLFloaterPermsDefault(const LLSD& seed)
	: LLFloater()
{
	mCommitCallbackRegistrar.add("PermsDefault.OK", boost::bind(&LLFloaterPermsDefault::onClickOK, this));
	mCommitCallbackRegistrar.add("PermsDefault.Cancel", boost::bind(&LLFloaterPermsDefault::onClickCancel, this));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_perm_prefs.xml");
}

// String equivalents of enum Categories - initialization order must match enum order!
const std::string LLFloaterPermsDefault::sCategoryNames[CAT_LAST] =
{
	"Objects",
	"Uploads",
	"Scripts",
	"Notecards",
	"Gestures",
	"Wearables"
};

void LLFloaterPermsDefault::initCheckboxes(bool export_support, const std::string& type)
{
	const U32 next_owner_perms = LLFloaterPerms::getNextOwnerPerms(type);
	getChildView(type + "everyone_export")->setEnabled(export_support && (next_owner_perms & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED);

	if (!(next_owner_perms & PERM_COPY))
	{
		getChildView(type + "next_owner_transfer")->setEnabled(false);
	}
	else if (export_support)
	{
		bool export_off = !gSavedPerAccountSettings.getBOOL(type+"EveryoneExport");
		getChildView(type + "next_owner_copy")->setEnabled(export_off);
		getChildView(type + "next_owner_modify")->setEnabled(export_off);
		getChildView(type + "next_owner_transfer")->setEnabled(export_off);
	}
	else // Set type+EveryoneExport false, just in case.
		gSavedPerAccountSettings.setBOOL(type+"EveryoneExport", false);
}

BOOL LLFloaterPermsDefault::postBuild()
{
	//handle_checkboxes
	bool export_support = LFSimFeatureHandler::instance().simSupportsExport();
	bool is_sl = gHippoGridManager->getCurrentGrid()->isSecondLife();
	for (S32 i = 0; i < CAT_LAST; ++i)
	{
		const std::string& type(sCategoryNames[i]);
		initCheckboxes(export_support, type);
		commit_callback_t handle_checks(boost::bind(handle_checkboxes, this, boost::bind(&LLView::getName, _1), _2, type));
		getChild<LLUICtrl>(type + "next_owner_copy")->setCommitCallback(handle_checks);
		getChild<LLUICtrl>(type + "next_owner_modify")->setCommitCallback(handle_checks);
		getChild<LLUICtrl>(type + "next_owner_transfer")->setCommitCallback(handle_checks);
		if (is_sl)
			getChildView(type + "everyone_export")->setVisible(false);
		else
			getChild<LLUICtrl>(type + "everyone_export")->setCommitCallback(handle_checks);
	}
	if (is_sl)
	{
		LLView* view(getChildView("ExportationLabel"));
		S32 shift(view->getRect().getWidth()); // Determine size of export area
		LLRect rect(getRect());
		rect.mRight -= shift;
		setRect(rect); // Cut off the export side
		view->setVisible(false); // Hide label
		// Move bottom buttons over so they look nice.
		shift /= -2;
		view = getChildView("ok");
		rect = view->getRect();
		rect.translate(shift, 0);
		view->setRect(rect);
		view = getChildView("cancel");
		rect = view->getRect();
		rect.translate(shift, 0);
		view->setRect(rect);
	}

	refresh();
	
	return TRUE;
}

void LLFloaterPermsDefault::onClickOK()
{
	ok();
	close();
}

void LLFloaterPermsDefault::onClickCancel()
{
	cancel();
	close();
}

class LLFloaterPermsResponder : public LLHTTPClient::ResponderWithResult
{
public:
	LLFloaterPermsResponder() : LLHTTPClient::ResponderWithResult() {}
private:
	static std::string sPreviousReason;

	void httpFailure(void)
	{
		// <singu> Prevent 404s from annoying the user all the tme
		if (mStatus == HTTP_NOT_FOUND)
			LL_INFOS("FloaterPermsResponder") << "Failed to send default permissions to simulator. 404, reason: " << mReason << LL_ENDL;
		else
		// </singu>
		// Do not display the same error more than once in a row
		if (mReason != sPreviousReason)
		{
			sPreviousReason = mReason;
			LLSD args;
			args["REASON"] = mReason;
			LLNotificationsUtil::add("DefaultObjectPermissions", args);
		}
	}
	void httpSuccess(void)
	{
		// Since we have had a successful POST call be sure to display the next error message
		// even if it is the same as a previous one.
		sPreviousReason = "";
		mCapSent = true;
		LL_INFOS("FloaterPermsResponder") << "Sent default permissions to simulator" << LL_ENDL;
	}
	/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const { return floaterPermsResponder_timeout; }
	/*virtual*/ char const* getName() const { return "LLFloaterPermsResponder"; }
};

std::string LLFloaterPermsResponder::sPreviousReason;

void LLFloaterPermsDefault::sendInitialPerms()
{
	if (!mCapSent)
	{
		updateCap();
	}
}

void LLFloaterPermsDefault::updateCap()
{
	std::string object_url = gAgent.getRegion()->getCapability("AgentPreferences");

	if (!object_url.empty())
	{
		LLSD report = LLSD::emptyMap();
		report["default_object_perm_masks"]["Group"] =
			(LLSD::Integer)LLFloaterPerms::getGroupPerms(sCategoryNames[CAT_OBJECTS]);
		report["default_object_perm_masks"]["Everyone"] =
			(LLSD::Integer)LLFloaterPerms::getEveryonePerms(sCategoryNames[CAT_OBJECTS]);
		report["default_object_perm_masks"]["NextOwner"] =
			(LLSD::Integer)LLFloaterPerms::getNextOwnerPerms(sCategoryNames[CAT_OBJECTS]);

		LLHTTPClient::post(object_url, report, new LLFloaterPermsResponder());
	}
}

void LLFloaterPermsDefault::ok()
{
	// Changes were already applied to saved settings.
	// Refreshing internal values makes it official.
	refresh();

	// We know some setting has changed but not which one.  Just in case it was a setting for
	// object permissions tell the server what the values are.
	updateCap();
}

void LLFloaterPermsDefault::cancel()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		gSavedSettings.setBOOL(sCategoryNames[iter]+"ShareWithGroup",    mShareWithGroup[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"EveryoneCopy",      mEveryoneCopy[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerCopy",     mNextOwnerCopy[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerModify",   mNextOwnerModify[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerTransfer", mNextOwnerTransfer[iter]);
		gSavedPerAccountSettings.setBOOL(sCategoryNames[iter]+"EveryoneExport", mEveryoneExport[iter]);
	}
}

void LLFloaterPermsDefault::refresh()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		mShareWithGroup[iter]    = gSavedSettings.getBOOL(sCategoryNames[iter]+"ShareWithGroup");
		mEveryoneCopy[iter]      = gSavedSettings.getBOOL(sCategoryNames[iter]+"EveryoneCopy");
		mNextOwnerCopy[iter]     = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerCopy");
		mNextOwnerModify[iter]   = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerModify");
		mNextOwnerTransfer[iter] = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerTransfer");
		mEveryoneExport[iter]    = gSavedPerAccountSettings.getBOOL(sCategoryNames[iter]+"EveryoneExport");
	}
}

void LLFloaterPermsDefault::onClose(bool app_quitting)
{
	// Cancel any unsaved changes before closing.
	// Note: when closed due to the OK button this amounts to a no-op.
	cancel();
	LLFloater::onClose(app_quitting);
}
