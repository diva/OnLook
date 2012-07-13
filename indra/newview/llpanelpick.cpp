/** 
 * @file llpanelpick.cpp
 * @brief LLPanelPick class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

// Display of a "Top Pick" used both for the global top picks in the 
// Find directory, and also for each individual user's picks in their
// profile.

#include "llviewerprecompiledheaders.h"

#include "llpanelpick.h"

#include "lldir.h"
#include "llparcel.h"
#include "message.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"
#include "lllineeditor.h"
#include "lltabcontainervertical.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "llviewergenericmessage.h"
#include "lluictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llworldmap.h"
#include "llfloaterworldmap.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llnotificationsutil.h"

// [RLVa:KB]
#include "rlvhandler.h"
// [/RLVa:KB]


//For pick import and export - RK
#include "statemachine/aifilepicker.h"
#include "llviewernetwork.h"
#include "llsdserialize.h"
#include "hippogridmanager.h"
//static
std::list<LLPanelPick*> LLPanelPick::sAllPanels;

LLPanelPick::LLPanelPick(BOOL top_pick)
:	LLPanel(std::string("Top Picks Panel")),
	mTopPick(top_pick),
	mPickID(),
	mCreatorID(),
	mParcelID(),
	mDataRequested(FALSE),
	mDataReceived(FALSE),
    mPosGlobal(),
    mSnapshotCtrl(NULL),
    mNameEditor(NULL),
    mDescEditor(NULL),
    mLocationEditor(NULL),
    mTeleportBtn(NULL),
    mMapBtn(NULL),
    //mLandmarkBtn(NULL),
    mSortOrderText(NULL),
    mSortOrderEditor(NULL),
    mEnabledCheck(NULL),
    mSetBtn(NULL),
	mImporting(0)
{
    sAllPanels.push_back(this);

	std::string pick_def_file;
	if (top_pick)
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_top_pick.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_pick.xml");
	}	
}


LLPanelPick::~LLPanelPick()
{
	if(mDataRequested && !mDataReceived)
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);
	}
    sAllPanels.remove(this);
}


void LLPanelPick::reset()
{
	if(mDataRequested && !mDataReceived)
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);
	}
	mPickID.setNull();
	mCreatorID.setNull();
	mParcelID.setNull();

	// Don't request data, this isn't valid
	mDataRequested = TRUE;
	mDataReceived = FALSE;

	mPosGlobal.clearVec();

	clearCtrls();
}


BOOL LLPanelPick::postBuild()
{
    mSnapshotCtrl = getChild<LLTextureCtrl>("snapshot_ctrl");
	mSnapshotCtrl->setCommitCallback(onCommitAny);
	mSnapshotCtrl->setCallbackUserData(this);

    mNameEditor = getChild<LLLineEditor>("given_name_editor");
	mNameEditor->setCommitOnFocusLost(TRUE);
	mNameEditor->setCommitCallback(onCommitAny);
	mNameEditor->setCallbackUserData(this);

    mDescEditor = getChild<LLTextEditor>("desc_editor");
	mDescEditor->setCommitOnFocusLost(TRUE);
	mDescEditor->setCommitCallback(onCommitAny);
	mDescEditor->setCallbackUserData(this);
	mDescEditor->setTabsToNextField(TRUE);

    mLocationEditor = getChild<LLLineEditor>("location_editor");

    mSetBtn = getChild<LLButton>( "set_location_btn");
    mSetBtn->setClickedCallback(boost::bind(&LLPanelPick::onClickSet,this));

    mTeleportBtn = getChild<LLButton>( "pick_teleport_btn");
    mTeleportBtn->setClickedCallback(boost::bind(&LLPanelPick::onClickTeleport,this));

    mMapBtn = getChild<LLButton>( "pick_map_btn");
    mMapBtn->setClickedCallback(boost::bind(&LLPanelPick::onClickMap,this));

	mSortOrderText = getChild<LLTextBox>("sort_order_text");

	mSortOrderEditor = getChild<LLLineEditor>("sort_order_editor");
	mSortOrderEditor->setPrevalidate(LLLineEditor::prevalidateInt);
	mSortOrderEditor->setCommitOnFocusLost(TRUE);
	mSortOrderEditor->setCommitCallback(onCommitAny);
	mSortOrderEditor->setCallbackUserData(this);

	mEnabledCheck = getChild<LLCheckBoxCtrl>( "enabled_check");
	mEnabledCheck->setCommitCallback(onCommitAny);
	mEnabledCheck->setCallbackUserData(this);

    return TRUE;
}

void LLPanelPick::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PICK_INFO != type)
	{
		return;
	}

	LLPickData* pick_info = static_cast<LLPickData*>(data);
	//llassert_always(pick_info->creator_id != gAgent.getID());
	//llassert_always(mCreatorID != gAgent.getID());
	if(!pick_info 
		|| pick_info->creator_id != mCreatorID 
		|| pick_info->pick_id != mPickID)
	{
		return;
	}

	LLAvatarPropertiesProcessor::getInstance()->removeObserver(mCreatorID, this);

	// "Location text" is actually the owner name, the original
    // name that owner gave the parcel, and the location.
	std::string location_text = pick_info->user_name + ", ";

	if (!pick_info->original_name.empty())
	{
		location_text.append(pick_info->original_name);
		location_text.append(", ");
	}

	location_text.append(pick_info->sim_name);
	location_text.append(" ");

	//Fix for location text importing - RK
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelPick* self = *iter;
		if(!self->mImporting)	self->mLocationText = location_text;
		else location_text = self->mLocationText;
		self->mImporting = false;
	}

    S32 region_x = llround((F32)pick_info->pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)pick_info->pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)pick_info->pos_global.mdV[VZ]);
   
    location_text.append(llformat("(%d, %d, %d)", region_x, region_y, region_z));

	mDataReceived = TRUE;

    // Found the panel, now fill in the information
	mPickID = pick_info->pick_id;
	mCreatorID = pick_info->creator_id;
	mParcelID = pick_info->parcel_id;
	mSimName = pick_info->sim_name;
	mPosGlobal = pick_info->pos_global;

	// Update UI controls
    mNameEditor->setText(pick_info->name);
    mDescEditor->setText(pick_info->desc);
    mSnapshotCtrl->setImageAssetID(pick_info->snapshot_id);
    mLocationEditor->setText(location_text);
    mEnabledCheck->set(pick_info->enabled);

	mSortOrderEditor->setText(llformat("%d", pick_info->sort_order));
    
}

// Fill in some reasonable defaults for a new pick.
void LLPanelPick::initNewPick()
{
	mPickID.generate();

	mCreatorID = gAgent.getID();

	mPosGlobal = gAgent.getPositionGlobal();

	// Try to fill in the current parcel
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		mNameEditor->setText(parcel->getName());
		mDescEditor->setText(parcel->getDesc());
		mSnapshotCtrl->setImageAssetID(parcel->getSnapshotID());
	}

	// Commit to the database, since we've got "new" values.
	sendPickInfoUpdate();
}

//Imports a new pick from an xml - RK
void LLPanelPick::importNewPick(void (*callback)(void*, bool), void* data)
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(FFLOAD_XML, "", "export");
	filepicker->run(boost::bind(&LLPanelPick::importNewPick_continued, this, callback, data, filepicker));
}

void LLPanelPick::importNewPick_continued(void (*callback)(void*, bool), void* data, AIFilePicker* filepicker)
{
	bool result = false;
	if (filepicker->hasFilename())
	{
		std::string file = filepicker->getFilename();

		llifstream importer(file);
		LLSD data;
		LLSDSerialize::fromXMLDocument(data, importer);

		LLSD file_data = data["Data"];
		data = LLSD();

		mPickID.generate();

		mCreatorID = gAgent.getID();

		mPosGlobal = LLVector3d(file_data["globalPos"]);

		mNameEditor->setText(file_data["name"].asString());
		mDescEditor->setText(file_data["desc"].asString());
		mSnapshotCtrl->setImageAssetID(file_data["snapshotID"].asUUID());
		mParcelID = file_data["parcelID"].asUUID();
		mLocationText = file_data["locationText"].asString();
		mImporting = true;

		sendPickInfoUpdate();
		result = true;
	}
	(*callback)(data, result);
}

//Exports a pick to an XML - RK
void LLPanelPick::exportPick()
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open("", FFSAVE_XML, "", "export");
	filepicker->run(boost::bind(&LLPanelPick::exportPick_continued, this, filepicker));
}

void LLPanelPick::exportPick_continued(AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename())
		return;

	std::string destination = filepicker->getFilename();

	LLSD datas;

	datas["name"] = mNameEditor->getText();
	datas["desc"] = mDescEditor->getText();
	datas["parcelID"] = mParcelID;
	datas["snapshotID"] = mSnapshotCtrl->getImageAssetID();
	datas["globalPos"] = mPosGlobal.getValue();
	datas["locationText"] = mLocationText;

	LLSD file;
	LLSD header;
	header["Version"] = 2;
	file["Header"] = header;
	std::string grid_uri = gHippoGridManager->getConnectedGrid()->getLoginUri();
	//LLStringUtil::toLower(uris[0]);
	file["Grid"] = grid_uri;
	file["Data"] = datas;

	// Create a file stream and write to it
	llofstream export_file(destination);
	LLSDSerialize::toPrettyXML(file, export_file);
	// Open the file save dialog
	export_file.close();
}


void LLPanelPick::setPickID(const LLUUID& pick_id, const LLUUID& creator_id)
{
	mPickID = pick_id;
	mCreatorID = creator_id;
}


// Schedules the panel to request data
// from the server next time it is drawn.
void LLPanelPick::markForServerRequest()
{
	mDataRequested = FALSE;
	mDataReceived = FALSE;
}


std::string LLPanelPick::getPickName()
{
	return mNameEditor->getText();
}


void LLPanelPick::sendPickInfoRequest()
{
	//llassert_always(mCreatorID != gAgent.getID());
	LLAvatarPropertiesProcessor::getInstance()->addObserver(mCreatorID, this);
	LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(mCreatorID, mPickID);

	mDataRequested = TRUE;
}


void LLPanelPick::sendPickInfoUpdate()
{
	LLPickData pick_data;

	// If we don't have a pick id yet, we'll need to generate one,
	// otherwise we'll keep overwriting pick_id 00000 in the database.
	if (mPickID.isNull())
	{
		mPickID.generate();
	}

	pick_data.agent_id = gAgent.getID();
	pick_data.session_id = gAgent.getSessionID();
	pick_data.pick_id = mPickID;
	pick_data.creator_id = gAgent.getID();

	//legacy var  need to be deleted
	pick_data.top_pick = mTopPick; 
	pick_data.parcel_id = mParcelID;
	pick_data.name = mNameEditor->getText();
	pick_data.desc = mDescEditor->getText();
	pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
	pick_data.pos_global = mPosGlobal;
	if(mTopPick)
		pick_data.sort_order = atoi(mSortOrderEditor->getText().c_str());
	else
		pick_data.sort_order = 0;

	pick_data.enabled = mEnabledCheck->get();

	LLAvatarPropertiesProcessor::getInstance()->sendPickInfoUpdate(&pick_data);
}



void LLPanelPick::draw()
{
	refresh();

	LLPanel::draw();
}


void LLPanelPick::refresh()
{
	if (!mDataRequested)
	{
		sendPickInfoRequest();
	}

    // Check for god mode
    BOOL godlike = gAgent.isGodlike();
	BOOL is_self = (gAgent.getID() == mCreatorID);

    // Set button visibility/enablement appropriately
	if (mTopPick)
	{
		mSnapshotCtrl->setEnabled(godlike);
		mNameEditor->setEnabled(godlike);
		mDescEditor->setEnabled(godlike);

		mSortOrderText->setVisible(godlike);

		mSortOrderEditor->setVisible(godlike);
		mSortOrderEditor->setEnabled(godlike);

		mEnabledCheck->setVisible(godlike);
		mEnabledCheck->setEnabled(godlike);

		mSetBtn->setVisible(godlike);
		//mSetBtn->setEnabled(godlike);
// [RLVa:KB] - Checked: 2009-07-04 (RLVa-1.0.0a)
		mSetBtn->setEnabled(godlike && (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) );
// [/RLVa:KB]
	}



	else
	{
		mSnapshotCtrl->setEnabled(is_self);
		mNameEditor->setEnabled(is_self);
		mDescEditor->setEnabled(is_self);

		mSortOrderText->setVisible(FALSE);

		mSortOrderEditor->setVisible(FALSE);
		mSortOrderEditor->setEnabled(FALSE);

		mEnabledCheck->setVisible(FALSE);
		mEnabledCheck->setEnabled(FALSE);

		mSetBtn->setVisible(is_self);
		//mSetBtn->setEnabled(is_self);
// [RLVa:KB] - Checked: 2009-07-04 (RLVa-1.0.0a)
		mSetBtn->setEnabled(is_self && (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) );
// [/RLVa]
	}
}





// static
void LLPanelPick::onClickTeleport(void* data)
{
    LLPanelPick* self = (LLPanelPick*)data;

    if (!self->mPosGlobal.isExactlyZero())
    {
        gAgent.teleportViaLocation(self->mPosGlobal);
        gFloaterWorldMap->trackLocation(self->mPosGlobal);
    }
}


// static
void LLPanelPick::onClickMap(void* data)
{
	LLPanelPick* self = (LLPanelPick*)data;
	gFloaterWorldMap->trackLocation(self->mPosGlobal);
	LLFloaterWorldMap::show(NULL, TRUE);
}

// static
/*
void LLPanelPick::onClickLandmark(void* data)
{
    LLPanelPick* self = (LLPanelPick*)data;
	create_landmark(self->mNameEditor->getText(), "", self->mPosGlobal);
}
*/

// static
void LLPanelPick::onClickSet(void* data)
{
// [RLVa:KB] - Checked: 2009-07-04 (RLVa-1.0.0a)
	if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		return;
	}
// [/RLVa:KB]
    LLPanelPick* self = (LLPanelPick*)data;

	// Save location for later.
	self->mPosGlobal = gAgent.getPositionGlobal();

	std::string location_text;
	location_text.assign("(will update after save)");
	location_text.append(", ");

    S32 region_x = llround((F32)self->mPosGlobal.mdV[VX]) % REGION_WIDTH_UNITS;
    S32 region_y = llround((F32)self->mPosGlobal.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)self->mPosGlobal.mdV[VZ]);

	location_text.append(self->mSimName);
    location_text.append(llformat(" (%d, %d, %d)", region_x, region_y, region_z));

	// if sim name in pick is different from current sim name
	// make sure it's clear that all that's being changed
	// is the location and nothing else
	if ( gAgent.getRegion ()->getName () != self->mSimName )
	{
		LLNotificationsUtil::add("SetPickLocation");
	};

	self->mLocationEditor->setText(location_text);

	onCommitAny(NULL, data);
}


// static
void LLPanelPick::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelPick* self = (LLPanelPick*)data;

	if(self->mCreatorID != gAgent.getID())
		return;

	// have we received up to date data for this pick?
	if (self->mDataReceived)
	{
		self->sendPickInfoUpdate();

		// Big hack - assume that top picks are always in a browser,
		// and non-top-picks are always in a tab container.
		/*if (self->mTopPick)
		{
			LLPanelDirPicks* panel = (LLPanelDirPicks*)self->getParent();
			panel->renamePick(self->mPickID, self->mNameEditor->getText());
		}
		else
		{*/
		LLTabContainer* tab = (LLTabContainer*)self->getParent();
		if (tab)
		{
			if(tab) tab->setCurrentTabName(self->mNameEditor->getText());
		}
		//}
	}
}
