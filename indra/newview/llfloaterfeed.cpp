/** 
 * @file llfloaterfeed.cpp
 * @brief Feed send floater, allows setting caption and whether or to add location, etc.
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * Copyright (c) 2012, Aleric Ingelwood.
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

#include "llfloaterfeed.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "v3dmath.h"
#include "lldir.h"

#include "llagent.h"
#include "llui.h"
#include "lllineeditor.h"
#include "llviewertexteditor.h"
#include "llbutton.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "lluictrlfactory.h"
#include "lluploaddialog.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llstatusbar.h"
#include "llviewerregion.h"
#include "lleconomy.h"
#include "lltrans.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llimagepng.h"
#include "llimagej2c.h"
#include "llvfile.h"
#include "llvfs.h"

#include "llassetuploadresponders.h"

#include "hippogridmanager.h"

///----------------------------------------------------------------------------
/// Class LLFloaterFeed
///----------------------------------------------------------------------------

LLFloaterFeed::LLFloaterFeed(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& img_scale, LLVector3d const& pos_taken_global) :
    LLFloater(std::string("Feed Floater")),
	mPNGImage(png), mViewerImage(img), mImageScale(img_scale), mPosTakenGlobal(pos_taken_global), mHasFirstMsgFocus(false)
{
}

// Destroys the object
LLFloaterFeed::~LLFloaterFeed()
{
  mPNGImage = NULL;
}

BOOL LLFloaterFeed::postBuild()
{
  childSetAction("cancel_btn", onClickCancel, this);
  childSetAction("send_btn", onClickSend, this);

  childDisable("from_form");

  std::string name_string;
  gAgent.buildFullname(name_string);
  childSetValue("name_form", LLSD(name_string));

  LLTextEditor* MsgField = getChild<LLTextEditor>("msg_form");
  if (MsgField)
  {
	MsgField->setWordWrap(TRUE);

	// For the first time a user focusess to .the msg box, all text will be selected.
	MsgField->setFocusChangedCallback(boost::bind(&LLFloaterFeed::onMsgFormFocusRecieved, this, _1, MsgField));
  }
  
  childSetFocus("to_form", TRUE);

  return TRUE;
}

// static
LLFloaterFeed* LLFloaterFeed::showFromSnapshot(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& image_scale, LLVector3d const& pos_taken_global)
{
  // Take the images from the caller
  // It's now our job to clean them up
  LLFloaterFeed* instance = new LLFloaterFeed(png, img, image_scale, pos_taken_global);

  LLUICtrlFactory::getInstance()->buildFloater(instance, "floater_snapshot_feed.xml");

  S32 left, top;
  gFloaterView->getNewFloaterPosition(&left, &top);
  instance->setOrigin(left, top - instance->getRect().getHeight());
  
  instance->open();

  return instance;
}

void LLFloaterFeed::draw(void)
{
  LLGLSUIDefault gls_ui;
  LLFloater::draw();

  if (!isMinimized() && mViewerImage.notNull() && mPNGImage.notNull()) 
  {
	LLRect rect(getRect());

	// First set the max extents of our preview.
	rect.translate(-rect.mLeft, -rect.mBottom);
	rect.mLeft += 10;
	rect.mRight -= 10;
	rect.mTop -= 25;
	rect.mBottom = rect.mTop - 150;

	// Then fix the aspect ratio.
	F32 ratio = (F32)mPNGImage->getWidth() / (F32)mPNGImage->getHeight();
	if ((F32)rect.getWidth() / (F32)rect.getHeight() >= ratio)
	{
	  rect.mRight = (S32)((F32)rect.mLeft + ((F32)rect.getHeight() * ratio));
	}
	else
	{
	  rect.mBottom = (S32)((F32)rect.mTop - ((F32)rect.getWidth() / ratio));
	}
	{
	  gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	  gl_rect_2d(rect, LLColor4(0.f, 0.f, 0.f, 1.f));
	  rect.stretch(-1);
	}
	{
	  gGL.matrixMode(LLRender::MM_TEXTURE);
	  gGL.pushMatrix();
	  {
		gGL.scalef(mImageScale.mV[VX], mImageScale.mV[VY], 1.f);
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gl_draw_scaled_image(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), mViewerImage, LLColor4::white);
	  }
	  gGL.matrixMode(LLRender::MM_TEXTURE);
	  gGL.popMatrix();
	  gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
  }
}

// static
void LLFloaterFeed::onClickCancel(void* data)
{
  if (data)
  {
	LLFloaterFeed* self = (LLFloaterFeed*)data;

	self->close(false);
  }
}

// static
void LLFloaterFeed::onClickSend(void* data)
{
  if (data)
  {
	LLFloaterFeed* self = (LLFloaterFeed*)data;

	std::string from(self->childGetValue("from_form").asString());
	std::string to(self->childGetValue("to_form").asString());
	
	if (self->mPNGImage.notNull())
	{
	  self->sendFeed();
	}
	else
	{
	  LLNotificationsUtil::add("ErrorProcessingSnapshot");
	}
  }
}

void LLFloaterFeed::onMsgFormFocusRecieved(LLFocusableElement* receiver, LLTextEditor* msg_form)
{
  if (msg_form && msg_form == receiver && msg_form->hasFocus() && !(mHasFirstMsgFocus))
  {
	mHasFirstMsgFocus = true;
	msg_form->setText(LLStringUtil::null);
  }
}

void LLFloaterFeed::sendFeed(void)
{
  // upload the image
  // DO THE UPLOAD HERE
  
  // give user feedback of the event
  gViewerWindow->playSnapshotAnimAndSound();
  LLUploadDialog::modalUploadDialog(getString("upload_message"));

  // don't destroy the window until the upload is done
  // this way we keep the information in the form
  setVisible(FALSE);

  // also remove any dependency on another floater
  // so that we can be sure to outlive it while we
  // need to.
  LLFloater* dependee = getDependee();
  if (dependee)
  {
	dependee->removeDependentFloater(this);
  }
}
