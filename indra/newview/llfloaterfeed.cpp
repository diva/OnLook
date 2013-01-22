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

#include "llfloatersnapshot.h"
#include "llimagepng.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"
#include "llwebprofile.h"
#include "lluploaddialog.h"
#include "lltexteditor.h"

#include <boost/bind.hpp>

///----------------------------------------------------------------------------
/// Class LLFloaterFeed
///----------------------------------------------------------------------------

LLFloaterFeed::LLFloaterFeed(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& img_scale, int index) : 
	LLFloater(std::string("Feed Floater")),
	mPNGImage(png), mViewerImage(img), mImageScale(img_scale), mSnapshotIndex(index), mHasFirstMsgFocus(false)
{
}

// Destroys the object
LLFloaterFeed::~LLFloaterFeed()
{
	mPNGImage = NULL;
}

BOOL LLFloaterFeed::postBuild()
{
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterFeed::onClickCancel, this));
	getChild<LLUICtrl>("post_btn")->setCommitCallback(boost::bind(&LLFloaterFeed::onClickPost, this));
	LLTextEditor* caption = getChild<LLTextEditor>("caption");
	if (caption)
	{
		// For the first time a user focusess to the msg box, all text will be selected.
		caption->setFocusChangedCallback(boost::bind(&LLFloaterFeed::onMsgFormFocusRecieved, this, _1, caption));
	}
	childSetFocus("cancel_btn", TRUE);

	return true;
}

void LLFloaterFeed::onMsgFormFocusRecieved(LLFocusableElement* receiver, LLTextEditor* caption)
{
	if (caption && caption == receiver && caption->hasFocus() && !mHasFirstMsgFocus)
	{
		mHasFirstMsgFocus = true;
		caption->setText(LLStringUtil::null);
	}
}

// static
LLFloaterFeed* LLFloaterFeed::showFromSnapshot(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& image_scale, int index)
{
  // Take the images from the caller
  // It's now our job to clean them up
  LLFloaterFeed* instance = new LLFloaterFeed(png, img, image_scale, index);

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

void LLFloaterFeed::onClickCancel()
{
	// Cancel is the same as just closing the floater.
	close(false);
}

void LLFloaterFeed::onClose(bool app_quitting)
{
	LLFloaterSnapshot::saveFeedDone(false, mSnapshotIndex);
	destroy();
}

void LLFloaterFeed::onClickPost()
{
	if (!mHasFirstMsgFocus)
	{
		// The user never switched focus to the messagee window. 
		// Using the default string.
		childSetValue("caption", getString("default_message"));
	}
	if (mPNGImage.notNull())
	{
		static LLCachedControl<bool> add_location("SnapshotFeedAddLocation");
		const std::string caption = childGetValue("caption").asString();
		LLWebProfile::setImageUploadResultCallback(boost::bind(&LLFloaterSnapshot::saveFeedDone, _1, mSnapshotIndex));
		LLFloaterSnapshot::saveStart(mSnapshotIndex);
		LLWebProfile::uploadImage(mPNGImage, caption, add_location);

		// give user feedback of the event
		gViewerWindow->playSnapshotAnimAndSound();
		LLUploadDialog::modalUploadDialog(getString("upload_message"));

		// don't destroy the window until the upload is done
		// this way we keep the information in the form
		setVisible(FALSE);
	}
	else
	{
		LLNotificationsUtil::add("ErrorProcessingSnapshot");
	}
}
