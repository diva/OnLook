/**
 * @file llmakeoutfitdialog.h
 * @brief The Make Outfit Dialog, triggered by "Make Outfit" and similar UICtrls.
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

#ifndef LLMAKEOUTFITDIALOG_H
#define LLMAKEOUTFITDIALOG_H


#include "llinventorymodel.h"
#include "llmodaldialog.h"

class LLMakeOutfitDialog : public LLModalDialog
{
private:
	std::vector<std::pair<std::string,S32> > mCheckBoxList;

public:
	LLMakeOutfitDialog(bool modal = true);
	/*virtual*/ void draw();
	BOOL postBuild();
	void refresh();

	void setWearableToInclude(S32 wearable, bool enabled, bool selected); //TODO: Call this when Wearables are added or removed to update the Dialog in !modal mode.
	void onSave();
	void onCheckAll(bool check);

	//Accessors
	void getIncludedItems(LLInventoryModel::item_array_t& item_list);
	bool getUseOutfits() { return childGetValue("checkbox_use_outfits").asBoolean(); }
	bool getUseLinks() { return childGetValue("checkbox_use_links").asBoolean(); }
	//bool getRenameClothing() { return childGetValue("rename").asBoolean(); }

protected:
	void makeOutfit(const std::string folder_name);
};

#endif //LLMAKEOUTFITDIALOG_H

