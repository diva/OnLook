/** 
 * @file llfloaterperms.h
 * @brief Asset creation permission preferences.
 * @author Coco 
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERPERMPREFS_H
#define LL_LLFLOATERPERMPREFS_H

#include "llfloater.h"

namespace LLFloaterPerms
{
	// Convenience methods to get current permission preference bitfields from saved settings:
	U32 getEveryonePerms(std::string prefix=""); // prefix + "EveryoneCopy"
	U32 getGroupPerms(std::string prefix=""); // prefix + "ShareWithGroup"
	U32 getNextOwnerPerms(std::string prefix=""); // bitfield for prefix + "NextOwner" + "Copy", "Modify", and "Transfer"
	U32 getNextOwnerPermsInverted(std::string prefix="");
}

class LLFloaterPermsDefault : public LLFloater, public LLFloaterSingleton<LLFloaterPermsDefault>
{
	friend class LLUISingleton<LLFloaterPermsDefault, VisibilityPolicy<LLFloater> >;
	
public:
	/*virtual*/ void onClose(bool app_quitting = false);
	/*virtual*/ BOOL postBuild();
	void initCheckboxes(bool export_support, const std::string& type);
	void ok();
	void cancel();
	void onClickOK();
	void onClickCancel();
	static void sendInitialPerms();
	static void updateCap();

	// Update instantiation of sCategoryNames in the .cpp file to match if you change this!
	enum Categories
	{
		CAT_OBJECTS,
		CAT_UPLOADS,
		CAT_SCRIPTS,
		CAT_NOTECARDS,
		CAT_GESTURES,
		CAT_WEARABLES,
		CAT_LAST
	};

private:
	LLFloaterPermsDefault(const LLSD& seed);
	void refresh();

	static const std::string sCategoryNames[CAT_LAST];

	// cached values only for implementing cancel.
	bool mShareWithGroup[CAT_LAST];
	bool mEveryoneCopy[CAT_LAST];
	bool mEveryoneExport[CAT_LAST];
	bool mNextOwnerCopy[CAT_LAST];
	bool mNextOwnerModify[CAT_LAST];
	bool mNextOwnerTransfer[CAT_LAST];
};

#endif
