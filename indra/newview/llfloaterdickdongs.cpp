/** 
 * @file LLFloaterDickDongs.cpp
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco
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
#include "llfloaterdickdongs.h"
#include "llcommon.h"
#include "llmd5.h"
#include "llagent.h"
#include "lluuid.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"

//this is really the only thing that needs to be here atm
LLFloaterDickDongs::LLFloaterDickDongs(const LLSD& seed)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_dickdongs.xml");
}

//Not needed yet
/*
BOOL LLFloaterDickDongs::postBuild()
{
	childSetCommitCallback("touch_only",      onClickUICheck, this);
	childSetCommitCallback("scripted",        onClickUICheck, this);
	childSetCommitCallback("physical",        onClickUICheck, this);
	childSetCommitCallback("sounds",          onClickUICheck, this);
	childSetCommitCallback("particles",       onClickUICheck, this);
	childSetCommitCallback("highlights",      onClickUICheck, this);
	childSetCommitCallback("beacons",         onClickUICheck, this);
	return TRUE;
}
*/


void LLFloaterDickDongs::open()
{
	LLUUID user = gAgent.getID();
	char hex_salty_uuid[MD5HEX_STR_SIZE];
	LLMD5 salted_uuid_hash((const U8*)user.asString().c_str(), 1);
	salted_uuid_hash.hex_digest(hex_salty_uuid);
	int i = (int)strtol((std::string(hex_salty_uuid).substr(0, 7) + "\n").c_str(),(char **)NULL,16);
	llinfos << "Bridge Channel: " << (S32)i << llendl;
	LLFloater::open();

}
void LLFloaterDickDongs::close(bool app_quitting)
{
	LLFloater::close(app_quitting);
}

//Also not needed yet
/*
// Callback attached to each check box control to both affect their main purpose
// and to implement the couple screwy interdependency rules that some have.
//static 
void LLFloaterDickDongs::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	std::string name = check->getName();
	LLFloaterDickDongs* view = (LLFloaterDickDongs*)data;
}
*/
