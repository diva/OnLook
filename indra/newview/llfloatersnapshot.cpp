/** 
 * @file llfloatersnapshot.cpp
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "llrender.h"
#include "v3dmath.h"
#include "llmath.h"
#include "lldir.h"
#include "lllocalcliprect.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llcallbacklist.h"
#include "llcriticaldamp.h"
#include "llfloaterperms.h"
#include "llui.h"
#include "llviewertexteditor.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lleconomy.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerstats.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llfloaterpostcard.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llworld.h"
#include "llagentui.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llvfs.h"

#include "hippogridmanager.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
S32 LLFloaterSnapshot::sUIWinHeightLong = 629 ;
S32 LLFloaterSnapshot::sUIWinHeightShort = LLFloaterSnapshot::sUIWinHeightLong - 270 ;
S32 LLFloaterSnapshot::sUIWinWidth = 219 ;
S32 const THUMBHEIGHT = 159;

LLSnapshotFloaterView* gSnapshotFloaterView = NULL;

LLFloaterSnapshot* LLFloaterSnapshot::sInstance = NULL;

const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;

F32 SHINE_TIME = 0.5f;
F32 SHINE_WIDTH = 0.6f;
F32 SHINE_OPACITY = 0.3f;
F32 FALL_TIME = 0.6f;
S32 BORDER_WIDTH = 6;

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const S32 MAX_TEXTURE_SIZE = 512 ; //max upload texture size 512 * 512

///----------------------------------------------------------------------------
/// Class LLSnapshotLivePreview 
///----------------------------------------------------------------------------
class LLSnapshotLivePreview : public LLView
{
public:
	enum ESnapshotType
	{
		SNAPSHOT_FEED,
		SNAPSHOT_POSTCARD,
		SNAPSHOT_TEXTURE,
		SNAPSHOT_LOCAL
	};

	U32 typeToMask(ESnapshotType type) const { return 1 << type; }
	void addUsedBy(ESnapshotType type) { mUsedBy |= typeToMask(type); }
	bool isUsedBy(ESnapshotType type) const { return (mUsedBy & typeToMask(type)) != 0; }
	bool isUsed(void) const { return mUsedBy; }
	void addManualOverride(ESnapshotType type) { mManualSizeOverride |= typeToMask(type); }
	bool isOverriddenBy(ESnapshotType type) const { return (mManualSizeOverride & typeToMask(type)) != 0; }

	LLSnapshotLivePreview(const LLRect& rect);
	~LLSnapshotLivePreview();

	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	
	void setSize(S32 w, S32 h);
	void getSize(S32& w, S32& h) const;
	void setAspect(F32 a);
	F32 getAspect() const;
	void getUsedSize(S32& w, S32& h) const;
	S32 getDataSize() const { return mFormattedDataSize; }
	void setMaxImageSize(S32 size) ;
	S32  getMaxImageSize() {return mMaxImageSize ;}
	
	ESnapshotType getSnapshotType() const { return mSnapshotType; }
	LLFloaterSnapshot::ESnapshotFormat getSnapshotFormat() const { return mSnapshotFormat; }
	BOOL getSnapshotUpToDateExceptForSize() const;
	BOOL getSnapshotUpToDate() const;
	BOOL isSnapshotActive() { return mSnapshotActive; }
	LLViewerTexture* getThumbnailImage() const { return mThumbnailImage ; }
	S32  getThumbnailWidth() const { return mThumbnailWidth ; }
	S32  getThumbnailHeight() const { return mThumbnailHeight ; }
	BOOL getThumbnailLock() const { return mThumbnailUpdateLock ; }
	BOOL getThumbnailUpToDate() const { return mThumbnailUpToDate ;}
	bool getShowFreezeFrameSnapshot() const { return mShowFreezeFrameSnapshot; }
	LLViewerTexture* getCurrentImage();
	BOOL isImageScaled();
	char const* aspectComboName() const;
	
	void setSnapshotType(ESnapshotType type) { mSnapshotType = type; }
	void setSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat type) { mSnapshotFormat = type; }
	void setSnapshotQuality(S32 quality);
	void setSnapshotBufferType(LLFloaterSnapshot* floater, LLViewerWindow::ESnapshotType type);
	void showFreezeFrameSnapshot(bool show);
	void updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail = FALSE, F32 delay = 0.f);
	void saveFeed();
	LLFloaterPostcard* savePostcard();
	void saveTexture();
	void saveLocal();
	void saveLocalDone(bool success);
	void close(LLFloaterSnapshot* view);
	void doCloseAfterSave();

	BOOL setThumbnailImageSize() ;
	void generateThumbnailImage(BOOL force_update = FALSE) ;
	void resetThumbnailImage() { mThumbnailImage = NULL ; }
	void drawPreviewRect(S32 offset_x, S32 offset_y) ;

	// Returns TRUE when snapshot generated, FALSE otherwise.
	static BOOL onIdle( void* snapshot_preview );

private:
	LLColor4					mColor;
	LLPointer<LLViewerTexture>	mViewerImage[2]; //used to represent the scene when the frame is frozen.
	LLRect						mImageRect[2];
	S32							mWidth[2];
	S32							mHeight[2];
	BOOL						mImageScaled[2];
	S32                         mMaxImageSize ;
	F32							mAspectRatio;
	
	//thumbnail image
	LLPointer<LLViewerTexture>	mThumbnailImage ;
	S32                         mThumbnailWidth ;
	S32                         mThumbnailHeight ;
	LLRect                      mPreviewRect ;
	BOOL                        mThumbnailUpdateLock ;
	BOOL                        mThumbnailUpToDate ;

	LLPointer<LLImageRaw>		mPreviewImage;
	LLPointer<LLImageRaw>		mPreviewImageEncoded;
	LLPointer<LLImageFormatted>	mFormattedImage;
	LLFrameTimer				mSnapshotDelayTimer;
	U32							mUsedBy;
	U32							mManualSizeOverride;
	S32							mShineCountdown;
	LLFrameTimer				mShineAnimTimer;
	F32							mFlashAlpha;
	BOOL						mNeedsFlash;
	LLVector3d					mPosTakenGlobal;
	S32							mSnapshotQuality;
	S32							mFormattedDataSize;
	ESnapshotType				mSnapshotType;
	LLFloaterSnapshot::ESnapshotFormat	mSnapshotFormat;
	S32							mUpToDateWidth;
	S32							mUpToDateHeight;
	F32							mUpToDateAspectRatio;
	BOOL						mUpToDateRenderUI;
	BOOL						mUpToDateRenderHUD;
	LLViewerWindow::ESnapshotType mUpToDateBufferType;
	S32							mUpToDateMaxImageSize;
	BOOL						mShowFreezeFrameSnapshot;
	LLFrameTimer				mFallAnimTimer;
	LLVector3					mCameraPos;
	LLQuaternion				mCameraRot;
	BOOL						mSnapshotActive;
	LLViewerWindow::ESnapshotType mSnapshotBufferType;

	bool						mCallbackHappened;
	bool						mSaveLocalSuccessful;
	LLFloaterSnapshot*			mCloseCalled;

public:
	static std::set<LLSnapshotLivePreview*> sList;
};

void LLSnapshotLivePreview::setSnapshotBufferType(LLFloaterSnapshot* floater, LLViewerWindow::ESnapshotType type)
{
	mSnapshotBufferType = type;
	switch(type)
	{
	  case LLViewerWindow::SNAPSHOT_TYPE_COLOR:
		floater->childSetValue("layer_types", "colors");
		break;
	  case LLViewerWindow::SNAPSHOT_TYPE_DEPTH:
		floater->childSetValue("layer_types", "depth");
		break;
	}
}

BOOL LLSnapshotLivePreview::getSnapshotUpToDateExceptForSize() const
{
	return mUpToDateAspectRatio == mAspectRatio &&
		   mUpToDateRenderUI == gSavedSettings.getBOOL("RenderUIInSnapshot") &&
		   mUpToDateRenderHUD == gSavedSettings.getBOOL("RenderHUDInSnapshot") &&
		   mUpToDateBufferType == mSnapshotBufferType &&
		   mUpToDateMaxImageSize != 0 && mUpToDateMaxImageSize <= mMaxImageSize;
}

BOOL LLSnapshotLivePreview::getSnapshotUpToDate() const
{
	return mUpToDateWidth == mWidth[0] &&
		   mUpToDateHeight == mHeight[0] &&
		   getSnapshotUpToDateExceptForSize();
}

std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;
LLSnapshotLivePreview::LLSnapshotLivePreview (const LLRect& rect) : 
	LLView(std::string("snapshot_live_preview"), rect, FALSE), 
	mColor(1.f, 0.f, 0.f, 0.5f), 
	mPreviewImage(NULL),
	mThumbnailImage(NULL) ,
	mThumbnailWidth(0),
	mThumbnailHeight(0),
	mPreviewImageEncoded(NULL),
	mFormattedImage(NULL),
	mUsedBy(0),
	mManualSizeOverride(0),
	mShineCountdown(0),
	mFlashAlpha(0.f),
	mNeedsFlash(TRUE),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mFormattedDataSize(0),
	mSnapshotType(SNAPSHOT_FEED),
	mSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat"))),
	mUpToDateWidth(0),
	mUpToDateHeight(0),
	mUpToDateAspectRatio(1.0),
	mUpToDateRenderUI(FALSE),
	mUpToDateRenderHUD(FALSE),
	mUpToDateBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR),
	mUpToDateMaxImageSize(0),
	mShowFreezeFrameSnapshot(FALSE),
	mCameraPos(LLViewerCamera::getInstance()->getOrigin()),
	mCameraRot(LLViewerCamera::getInstance()->getQuaternion()),
	mSnapshotActive(FALSE),
	mSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR)
{
	DoutEntering(dc::notice, "LLSnapshotLivePreview() [" << (void*)this << "]");
	setSnapshotQuality(gSavedSettings.getS32("SnapshotQuality"));
	mSnapshotDelayTimer.start();
// 	gIdleCallbacks.addFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.insert(this);
	setFollowsAll();
	mWidth[0] = gViewerWindow->getWindowDisplayWidth();
	mWidth[1] = gViewerWindow->getWindowDisplayWidth();
	mHeight[0] = gViewerWindow->getWindowDisplayHeight();
	mHeight[1] = gViewerWindow->getWindowDisplayHeight();
	mImageScaled[0] = FALSE;
	mImageScaled[1] = FALSE;

	mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	mThumbnailUpdateLock = FALSE ;
	mThumbnailUpToDate   = FALSE ;
	updateSnapshot(TRUE,TRUE); //To initialize mImageRect to correct values
}

LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
	DoutEntering(dc::notice, "~LLSnapshotLivePreview() [" << (void*)this << "]");
	// delete images
	mPreviewImage = NULL;
	mPreviewImageEncoded = NULL;
	mFormattedImage = NULL;

// 	gIdleCallbacks.deleteFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.erase(this);
}

void LLSnapshotLivePreview::setMaxImageSize(S32 size) 
{
	if(size < MAX_SNAPSHOT_IMAGE_SIZE)
	{
		mMaxImageSize = size;
	}
	else
	{
		mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	}
}

LLViewerTexture* LLSnapshotLivePreview::getCurrentImage()
{
	return mViewerImage[0];
}

BOOL LLSnapshotLivePreview::isImageScaled()
{
	return mImageScaled[0];
}

void LLSnapshotLivePreview::showFreezeFrameSnapshot(bool show)
{
	DoutEntering(dc::notice, "LLSnapshotLivePreview::showFreezeFrameSnapshot(" << show << ")");
	// If we stop to show the fullscreen "freeze frame" snapshot, play the "fall away" animation.
	if (mShowFreezeFrameSnapshot && !show)
	{
		mViewerImage[1] = mViewerImage[0];
		mImageScaled[1] = mImageScaled[0];
		mImageRect[1] = mImageRect[0];
		mWidth[1] = mWidth[0];
		mHeight[1] = mHeight[0];
		mFallAnimTimer.start();		
	}
	mShowFreezeFrameSnapshot = show;
}

void LLSnapshotLivePreview::updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail, F32 delay) 
{
	DoutEntering(dc::notice, "LLSnapshotLivePreview::updateSnapshot(" << new_snapshot << ", " << new_thumbnail << ", " << delay << ")");

	LLRect& rect = mImageRect[0];
	rect.set(0, getRect().getHeight(), getRect().getWidth(), 0);

	F32 window_aspect_ratio = ((F32)getRect().getWidth()) / ((F32)getRect().getHeight());

	if (mAspectRatio > window_aspect_ratio)
	{
		// trim off top and bottom
		S32 height_diff = llround(getRect().getHeight() - (F32)getRect().getWidth() / mAspectRatio);
		S32 half_height_diff = llround((getRect().getHeight() - (F32)getRect().getWidth() / mAspectRatio) * 0.5);
		rect.mBottom += half_height_diff;
		rect.mTop -= height_diff - half_height_diff;
	}
	else if (mAspectRatio < window_aspect_ratio)
	{
		// trim off left and right
		S32 width_diff = llround(getRect().getWidth() - (F32)getRect().getHeight() * mAspectRatio);
		S32 half_width_diff = llround((getRect().getWidth() - (F32)getRect().getHeight() * mAspectRatio) * 0.5f);
		rect.mLeft += half_width_diff;
		rect.mRight -= width_diff - half_width_diff;
	}

	mShineAnimTimer.stop();
	if (new_snapshot)
	{
		mSnapshotDelayTimer.start(delay);
	}
	if(new_thumbnail)
	{
		mThumbnailUpToDate = FALSE ;
	}
	setThumbnailImageSize();
}

void LLSnapshotLivePreview::setSnapshotQuality(S32 quality)
{
	llclamp(quality, 0, 100);
	if (quality != mSnapshotQuality)
	{
		mSnapshotQuality = quality;
		gSavedSettings.setS32("SnapshotQuality", quality);
	}
}

void LLSnapshotLivePreview::drawPreviewRect(S32 offset_x, S32 offset_y)
{
	// Draw two alpha rectangles to cover areas outside of the snapshot image.
	LLColor4 alpha_color(0.5f, 0.5f, 0.5f, 0.8f);
	if (mThumbnailWidth > mPreviewRect.getWidth())
	{
		gl_rect_2d(                      offset_x,                        offset_y,
				   mPreviewRect.mLeft  + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
		gl_rect_2d(mPreviewRect.mRight + offset_x,                        offset_y,
				   mThumbnailWidth     + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
	}
	if (mThumbnailHeight > mPreviewRect.getHeight())
	{
		gl_rect_2d(                      offset_x,                        offset_y,
				   mThumbnailWidth     + offset_x, mPreviewRect.mBottom + offset_y,
				   alpha_color, TRUE);
		gl_rect_2d(                      offset_x, mPreviewRect.mTop    + offset_y,
				   mThumbnailWidth     + offset_x, mThumbnailHeight     + offset_y,
				   alpha_color, TRUE);
	}
	// Draw border around captured part.
	F32 line_width; 
	glGetFloatv(GL_LINE_WIDTH, &line_width);
	glLineWidth(2.0f * line_width);
	gl_rect_2d( mPreviewRect.mLeft  + offset_x, mPreviewRect.mTop    + offset_y,
				mPreviewRect.mRight + offset_x, mPreviewRect.mBottom + offset_y,
				LLColor4::black, FALSE);
	glLineWidth(line_width);
}

//called when the frame is frozen.
void LLSnapshotLivePreview::draw()
{
	if (mViewerImage[0].notNull() &&
	    mPreviewImageEncoded.notNull() &&
	    mShowFreezeFrameSnapshot)
	{
		LLColor4 bg_color(0.f, 0.f, 0.3f, 0.4f);
		gl_rect_2d(getRect(), bg_color);
		LLRect &rect = mImageRect[0];
		LLRect shadow_rect = mImageRect[0];
		shadow_rect.stretch(BORDER_WIDTH);
		gl_drop_shadow(shadow_rect.mLeft, shadow_rect.mTop, shadow_rect.mRight, shadow_rect.mBottom, LLColor4(0.f, 0.f, 0.f, mNeedsFlash ? 0.f :0.5f), 10);

		LLColor4 image_color(1.f, 1.f, 1.f, 1.f);
		gGL.color4fv(image_color.mV);
		gGL.getTexUnit(0)->bind(mViewerImage[0]);
		// calculate UV scale
		F32 uv_width = mImageScaled[0] ? 1.f : llmin((F32)mWidth[0] / (F32)mViewerImage[0]->getWidth(), 1.f);
		F32 uv_height = mImageScaled[0] ? 1.f : llmin((F32)mHeight[0] / (F32)mViewerImage[0]->getHeight(), 1.f);
		gGL.pushMatrix();
		{
			gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom, 0.f);
			gGL.begin(LLRender::QUADS);
			{
				gGL.texCoord2f(uv_width, uv_height);
				gGL.vertex2i(rect.getWidth(), rect.getHeight() );

				gGL.texCoord2f(0.f, uv_height);
				gGL.vertex2i(0, rect.getHeight() );

				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2i(0, 0);

				gGL.texCoord2f(uv_width, 0.f);
				gGL.vertex2i(rect.getWidth(), 0);
			}
			gGL.end();
		}
		gGL.popMatrix();

		gGL.color4f(1.f, 1.f, 1.f, mFlashAlpha);
		gl_rect_2d(getRect());
		if (mNeedsFlash)
		{
			if (mFlashAlpha < 1.f)
			{
				mFlashAlpha = lerp(mFlashAlpha, 1.f, LLCriticalDamp::getInterpolant(0.02f));
			}
			else
			{
				mNeedsFlash = FALSE;
			}
		}
		else
		{
			mFlashAlpha = lerp(mFlashAlpha, 0.f, LLCriticalDamp::getInterpolant(0.15f));
		}

		if (mShineCountdown > 0)
		{
			mShineCountdown--;
			if (mShineCountdown == 0)
			{
				mShineAnimTimer.start();
			}
		}
		else if (mShineAnimTimer.getStarted())
		{
			//LLDebugVarMessageBox::show("Shine time", &SHINE_TIME, 10.f, 0.1f);
			//LLDebugVarMessageBox::show("Shine width", &SHINE_WIDTH, 2.f, 0.05f);
			//LLDebugVarMessageBox::show("Shine opacity", &SHINE_OPACITY, 1.f, 0.05f);

			F32 shine_interp = llmin(1.f, mShineAnimTimer.getElapsedTimeF32() / SHINE_TIME);
			
			// draw "shine" effect
			LLLocalClipRect clip(getLocalRect());
			{
				// draw diagonal stripe with gradient that passes over screen
				S32 x1 = gViewerWindow->getWindowWidthScaled() * llround((clamp_rescale(shine_interp, 0.f, 1.f, -1.f - SHINE_WIDTH, 1.f)));
				S32 x2 = x1 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 x3 = x2 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 y1 = 0;
				S32 y2 = gViewerWindow->getWindowHeightScaled();

				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gGL.begin(LLRender::QUADS);
				{
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x1, y1);
					gGL.vertex2i(x1 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x2, y1);

					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2, y1);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x3 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x3, y1);
				}
				gGL.end();
			}

			// if we're at the end of the animation, stop
			if (shine_interp >= 1.f)
			{
				mShineAnimTimer.stop();
			}
		}
	}

	// draw framing rectangle
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		LLRect outline_rect = mImageRect[0];
		gGL.begin(LLRender::QUADS);
		{
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
		}
		gGL.end();
	}

	// draw old image dropping away
	if (mFallAnimTimer.getStarted())
	{
		if (mViewerImage[1].notNull() && mFallAnimTimer.getElapsedTimeF32() < FALL_TIME)
		{
			F32 fall_interp = mFallAnimTimer.getElapsedTimeF32() / FALL_TIME;
			F32 alpha = clamp_rescale(fall_interp, 0.f, 1.f, 0.8f, 0.4f);
			LLColor4 image_color(1.f, 1.f, 1.f, alpha);
			gGL.color4fv(image_color.mV);
			gGL.getTexUnit(0)->bind(mViewerImage[1]);
			// calculate UV scale
			// *FIX get this to work with old image
			BOOL rescale = !mImageScaled[1] && mViewerImage[0].notNull();
			F32 uv_width = rescale ? llmin((F32)mWidth[1] / (F32)mViewerImage[0]->getWidth(), 1.f) : 1.f;
			F32 uv_height = rescale ? llmin((F32)mHeight[1] / (F32)mViewerImage[0]->getHeight(), 1.f) : 1.f;
			gGL.pushMatrix();
			{
				LLRect& rect = mImageRect[1];
				gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom - llround(getRect().getHeight() * 2.f * (fall_interp * fall_interp)), 0.f);
				gGL.rotatef(-45.f * fall_interp, 0.f, 0.f, 1.f);
				gGL.begin(LLRender::QUADS);
				{
					gGL.texCoord2f(uv_width, uv_height);
					gGL.vertex2i(rect.getWidth(), rect.getHeight() );

					gGL.texCoord2f(0.f, uv_height);
					gGL.vertex2i(0, rect.getHeight() );

					gGL.texCoord2f(0.f, 0.f);
					gGL.vertex2i(0, 0);

					gGL.texCoord2f(uv_width, 0.f);
					gGL.vertex2i(rect.getWidth(), 0);
				}
				gGL.end();
			}
			gGL.popMatrix();
		}
	}
}

/*virtual*/ 
void LLSnapshotLivePreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect old_rect = getRect();
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		updateSnapshot(FALSE, TRUE);
	}
}

BOOL LLSnapshotLivePreview::setThumbnailImageSize()
{
	if(mWidth[0] < 10 || mHeight[0] < 10)
	{
		return FALSE ;
	}
	S32 window_width = gViewerWindow->getWindowWidthRaw() ;
	S32 window_height = gViewerWindow->getWindowHeightRaw() ;

	F32 window_aspect_ratio = ((F32)window_width) / ((F32)window_height);

	// UI size for thumbnail
	S32 max_width =  THUMBHEIGHT * 4 / 3; // == LLFloaterSnapshot::getUIWinWidth() - 7;
	S32 max_height = THUMBHEIGHT;

	if (window_aspect_ratio > (F32)max_width / max_height)
	{
		// image too wide, shrink to width
		mThumbnailWidth = max_width;
		mThumbnailHeight = llround((F32)max_width / window_aspect_ratio);
	}
	else
	{
		// image too tall, shrink to height
		mThumbnailHeight = max_height;
		mThumbnailWidth = llround((F32)max_height * window_aspect_ratio);
	}
	
	if(mThumbnailWidth > window_width || mThumbnailHeight > window_height)
	{
		return FALSE ;//if the window is too small, ignore thumbnail updating.
	}

	S32 left = 0 , top = mThumbnailHeight, right = mThumbnailWidth, bottom = 0 ;
	F32 ratio = mAspectRatio * window_height / window_width;
	if(ratio > 1.f)
	{
		top = llround(top / ratio);
	}
	else
	{
		right = llround(right * ratio);
	}			
	left = (mThumbnailWidth - right + 1) / 2;
	bottom = (mThumbnailHeight - top + 1) / 2;
	top += bottom ;
	right += left ;
	mPreviewRect.set(left - 1, top + 1, right + 1, bottom - 1) ;

	return TRUE ;
}

void LLSnapshotLivePreview::generateThumbnailImage(BOOL force_update)
{	
	if(mThumbnailUpdateLock) //in the process of updating
	{
		return ;
	}
	if(mThumbnailUpToDate && !force_update)//already updated
	{
		return ;
	}
	if(mWidth[0] < 10 || mHeight[0] < 10)
	{
		return ;
	}

	////lock updating
	mThumbnailUpdateLock = TRUE ;

	if(!setThumbnailImageSize())
	{
		mThumbnailUpdateLock = FALSE ;
		mThumbnailUpToDate = TRUE ;
		return ;
	}

	Dout(dc::notice, "LLSnapshotLivePreview::generateThumbnailImage: Actually making a new thumbnail!");

	if(mThumbnailImage)
	{
		resetThumbnailImage() ;
	}		

	LLPointer<LLImageRaw> raw = NULL ;
	S32 w , h ;
	// Set w, h to the nearest power of two that is larger or equal to mThumbnailWidth, mThumbnailHeight (but never more than 1024).
	w = get_lower_power_two(mThumbnailWidth - 1, 512) * 2 ;		// -1 so that w becomes mThumbnailWidth if it is a power of 2, instead of 2 * mThumbnailWidth.
	h = get_lower_power_two(mThumbnailHeight - 1, 512) * 2 ;
	{
		raw = new LLImageRaw ;
		if(!gViewerWindow->thumbnailSnapshot(raw,
								w, h,
								gSavedSettings.getBOOL("RenderUIInSnapshot"),
								FALSE,
								mSnapshotBufferType) )								
		{
			raw = NULL ;
		}
	}

	if(raw)
	{
		mThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE); 		
		mThumbnailUpToDate = TRUE ;
		Dout(dc::notice, "thumbnailSnapshot(" << w << ", " << h << ", ...) returns " << raw->getWidth() << ", " << raw->getHeight());
	}

	//unlock updating
	mThumbnailUpdateLock = FALSE ;		
}


// Called often. Checks whether it's time to grab a new snapshot and if so, does it.
// Returns TRUE if new snapshot generated, FALSE otherwise.
//static 
BOOL LLSnapshotLivePreview::onIdle( void* snapshot_preview )
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)snapshot_preview;	

	LLVector3 new_camera_pos = LLViewerCamera::getInstance()->getOrigin();
	LLQuaternion new_camera_rot = LLViewerCamera::getInstance()->getQuaternion();
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if (freeze_time && 
		(new_camera_pos != previewp->mCameraPos || dot(new_camera_rot, previewp->mCameraRot) < 0.995f))
	{
		previewp->mCameraPos = new_camera_pos;
		previewp->mCameraRot = new_camera_rot;
		previewp->showFreezeFrameSnapshot(false);
		// request a new snapshot whenever the camera moves, with a time delay
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		previewp->updateSnapshot(
			autosnap, // whether a new snapshot is needed or merely invalidate the existing one
			FALSE, // or if 1st arg is false, whether to produce a new thumbnail image.
			autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f); // shutter delay if 1st arg is true.
	}

	// see if it's time yet to snap the shot and bomb out otherwise.
	previewp->mSnapshotActive = 
		(previewp->mSnapshotDelayTimer.getStarted() && previewp->mSnapshotDelayTimer.hasExpired())
		&& !LLToolCamera::getInstance()->hasMouseCapture(); // don't take snapshots while ALT-zoom active
	if ( ! previewp->mSnapshotActive)
	{
		return FALSE;
	}

	// time to produce a snapshot

	if (!previewp->mPreviewImage)
	{
		previewp->mPreviewImage = new LLImageRaw;
	}

	if (!previewp->mPreviewImageEncoded)
	{
		previewp->mPreviewImageEncoded = new LLImageRaw;
	}

	previewp->setVisible(FALSE);
	previewp->setEnabled(FALSE);
	
	previewp->getWindow()->incBusyCount();
	previewp->mImageScaled[0] = FALSE;

	// grab the raw image and encode it into desired format
	Dout(dc::notice, "LLSnapshotLivePreview::onIdle: Actually making a new snapshot!");
	previewp->mFormattedDataSize = 0;		// The size of (to be generated) formatted image is unknown.
	previewp->mUsedBy = 0;					// This snapshot wasn't used yet.
	previewp->mManualSizeOverride = 0;

	// Remember with what parameters this snapshot was taken.
	previewp->mUpToDateWidth = previewp->mWidth[0];
	previewp->mUpToDateHeight = previewp->mHeight[0];
	previewp->mUpToDateRenderUI = gSavedSettings.getBOOL("RenderUIInSnapshot");
	previewp->mUpToDateRenderHUD = gSavedSettings.getBOOL("RenderHUDInSnapshot");
	previewp->mUpToDateBufferType = previewp->mSnapshotBufferType;
	previewp->mUpToDateAspectRatio = previewp->mAspectRatio;

	if((previewp->mUpToDateMaxImageSize = gViewerWindow->rawSnapshot(
							previewp->mPreviewImage,
							previewp->mUpToDateWidth,
							previewp->mUpToDateHeight,
							previewp->mUpToDateAspectRatio,
							previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE,		// UNUSED
							previewp->mUpToDateRenderUI,
							FALSE,
							previewp->mUpToDateBufferType,
							previewp->getMaxImageSize())))
	{
		previewp->mPreviewImageEncoded->resize(
			previewp->mPreviewImage->getWidth(), 
			previewp->mPreviewImage->getHeight(), 
			previewp->mPreviewImage->getComponents());

		if(previewp->getSnapshotType() == SNAPSHOT_TEXTURE)
		{
			LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
			LLPointer<LLImageRaw> scaled = new LLImageRaw(
				previewp->mPreviewImage->getData(),
				previewp->mPreviewImage->getWidth(),
				previewp->mPreviewImage->getHeight(),
				previewp->mPreviewImage->getComponents());
		
			scaled->biasedScaleToPowerOfTwo(512);
			previewp->mImageScaled[0] = TRUE;
			if (formatted->encode(scaled, 0.f))
			{
				previewp->mFormattedDataSize = formatted->getDataSize();
				Dout(dc::notice, "LLSnapshotLivePreview::onIdle: Making a new mPreviewImageEncoded (J2C)!");
				formatted->decode(previewp->mPreviewImageEncoded, 0);
			}
		}
		else
		{
			Dout(dc::notice, "LLSnapshotLivePreview::onIdle: Making a new mFormattedImage!");

			// delete any existing image
			previewp->mFormattedImage = NULL;
			// now create the new one of the appropriate format.
			// note: feeds and postcards hardcoded to always use jpeg.
			LLFloaterSnapshot::ESnapshotFormat format =
				(previewp->getSnapshotType() == SNAPSHOT_POSTCARD || previewp->getSnapshotType() == SNAPSHOT_FEED)
				? LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG : previewp->getSnapshotFormat();
			switch(format)
			{
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
				previewp->mFormattedImage = new LLImagePNG(); 
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
				previewp->mFormattedImage = new LLImageJPEG(previewp->mSnapshotQuality); 
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
				previewp->mFormattedImage = new LLImageBMP(); 
				break;
			}
			if (previewp->mFormattedImage->encode(previewp->mPreviewImage, 0))
			{
				previewp->mFormattedDataSize = previewp->mFormattedImage->getDataSize();
				// special case BMP to copy instead of decode otherwise decode will crash.
				if(format == LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP)
				{
					Dout(dc::notice, "LLSnapshotLivePreview::onIdle: Making a new mPreviewImageEncoded (BMP)!");
					previewp->mPreviewImageEncoded->copy(previewp->mPreviewImage);
				}
				else
				{
					Dout(dc::notice, "LLSnapshotLivePreview::onIdle: Making a new mPreviewImageEncoded (" << format << ")!");
					previewp->mFormattedImage->decode(previewp->mPreviewImageEncoded, 0);
				}
			}
		}

		LLPointer<LLImageRaw> scaled = new LLImageRaw(
			previewp->mPreviewImageEncoded->getData(),
			previewp->mPreviewImageEncoded->getWidth(),
			previewp->mPreviewImageEncoded->getHeight(),
			previewp->mPreviewImageEncoded->getComponents());
		
		if(!scaled->isBufferInvalid())
		{
			// leave original image dimensions, just scale up texture buffer
			if (previewp->mPreviewImageEncoded->getWidth() > 1024 || previewp->mPreviewImageEncoded->getHeight() > 1024)
			{
				// go ahead and shrink image to appropriate power of 2 for display
				scaled->biasedScaleToPowerOfTwo(1024);
				previewp->mImageScaled[0] = TRUE;
			}
			else
			{
				// expand image but keep original image data intact
				scaled->expandToPowerOfTwo(1024, FALSE);
			}

			previewp->mViewerImage[0] = LLViewerTextureManager::getLocalTexture(scaled.get(), FALSE);
			LLPointer<LLViewerTexture> curr_preview_image = previewp->mViewerImage[0];
			gGL.getTexUnit(0)->bind(curr_preview_image);
			if (previewp->getSnapshotType() != SNAPSHOT_TEXTURE)
			{
				curr_preview_image->setFilteringOption(LLTexUnit::TFO_POINT);
			}
			else
			{
				curr_preview_image->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
			}
			curr_preview_image->setAddressMode(LLTexUnit::TAM_CLAMP);

			previewp->mShowFreezeFrameSnapshot = TRUE;
			//Resize to thumbnail.
			{
				previewp->mThumbnailUpToDate = TRUE ;
				previewp->mThumbnailUpdateLock = TRUE ;
				S32 w = get_lower_power_two(scaled->getWidth(), 512) * 2 ;
				S32 h = get_lower_power_two(scaled->getHeight(), 512) * 2 ;
				scaled->scale(w,h);
				previewp->mThumbnailImage =  LLViewerTextureManager::getLocalTexture(scaled.get(), FALSE);
				previewp->mThumbnailUpdateLock = FALSE ;
				previewp->setThumbnailImageSize();
			}

			previewp->mPosTakenGlobal = gAgentCamera.getCameraPositionGlobal();
			previewp->mShineCountdown = 4; // wait a few frames to avoid animation glitch due to readback this frame
		}
	}
	previewp->getWindow()->decBusyCount();
	// only show fullscreen preview when in freeze frame mode
	previewp->setVisible(gSavedSettings.getBOOL("FreezeTime"));
	previewp->mSnapshotDelayTimer.stop();
	previewp->mSnapshotActive = FALSE;
	previewp->mThumbnailUpToDate = FALSE;
	previewp->generateThumbnailImage() ;

	return TRUE;
}

void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
	mWidth[0] = w;
	mHeight[0] = h;
}

void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
	w = mWidth[0];
	h = mHeight[0];
}

void LLSnapshotLivePreview::setAspect(F32 a)
{
	mAspectRatio = a;
}

F32 LLSnapshotLivePreview::getAspect() const
{
	return mAspectRatio;
}

void LLSnapshotLivePreview::getUsedSize(S32& w, S32& h) const
{
	w = mUpToDateWidth;
	h = mUpToDateHeight;
}

void LLSnapshotLivePreview::saveFeed()
{
	addUsedBy(SNAPSHOT_FEED);
}

LLFloaterPostcard* LLSnapshotLivePreview::savePostcard()
{
	if(mViewerImage[0].isNull())
	{
		//this should never happen!!
		llwarns << "The snapshot image has not been generated!" << llendl ;
		return NULL ;
	}

	// calculate and pass in image scale in case image data only use portion
	// of viewerimage buffer
	LLVector2 image_scale(1.f, 1.f);
	if (!isImageScaled())
	{
		image_scale.setVec(llmin(1.f, (F32)mWidth[0] / (F32)getCurrentImage()->getWidth()), llmin(1.f, (F32)mHeight[0] / (F32)getCurrentImage()->getHeight()));
	}

	LLImageJPEG* jpg = dynamic_cast<LLImageJPEG*>(mFormattedImage.get());
	if(!jpg)
	{
		llwarns << "Formatted image not a JPEG" << llendl;
		return NULL;
	}
	LLFloaterPostcard* floater = LLFloaterPostcard::showFromSnapshot(jpg, mViewerImage[0], image_scale, mPosTakenGlobal);
	// relinquish lifetime of jpeg image to postcard floater
	mFormattedImage = NULL;
	mFormattedDataSize = 0;
	updateSnapshot(FALSE, FALSE);

	addUsedBy(SNAPSHOT_POSTCARD);
	return floater;
}

void LLSnapshotLivePreview::saveTexture()
{
	// gen a new uuid for this asset
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
		
	LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
	LLPointer<LLImageRaw> scaled = new LLImageRaw(mPreviewImage->getData(),
												  mPreviewImage->getWidth(),
												  mPreviewImage->getHeight(),
												  mPreviewImage->getComponents());
	
	scaled->biasedScaleToPowerOfTwo(512);
			
	if (formatted->encode(scaled, 0.0f))
	{
		LLVFile::writeFile(formatted->getData(), formatted->getDataSize(), gVFS, new_asset_id, LLAssetType::AT_TEXTURE);
		std::string pos_string;
		LLAgentUI::buildLocationString(pos_string, LLAgentUI::LOCATION_FORMAT_FULL);
		std::string who_took_it;
		LLAgentUI::buildFullname(who_took_it);
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
		void *userdata = NULL;
		upload_new_resource(tid,	// tid
				    LLAssetType::AT_TEXTURE,
				    "Snapshot : " + pos_string,
				    "Taken by " + who_took_it + " at " + pos_string,
				    0,
				    LLFolderType::FT_SNAPSHOT_CATEGORY,
				    LLInventoryType::IT_SNAPSHOT,
				    PERM_ALL,  // Note: Snapshots to inventory is a special case of content upload
				    LLFloaterPerms::getGroupPerms(), // that is more permissive than other uploads
				    LLFloaterPerms::getEveryonePerms(),
				    "Snapshot : " + pos_string,
				    callback, expected_upload_cost, userdata);
		gViewerWindow->playSnapshotAnimAndSound();
	}
	else
	{
		LLNotificationsUtil::add("ErrorEncodingSnapshot");
		llwarns << "Error encoding snapshot" << llendl;
	}

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_SNAPSHOT_COUNT );
	
	mFormattedDataSize = 0;
	addUsedBy(SNAPSHOT_TEXTURE);
}

void LLSnapshotLivePreview::saveLocal()
{
	mCallbackHappened = false;
	mSaveLocalSuccessful = false;
	mCloseCalled = NULL;
	gViewerWindow->saveImageNumbered(mFormattedImage);	// This calls saveLocalDone() immediately, or later.
}

void LLSnapshotLivePreview::close(LLFloaterSnapshot* view)
{
	if (getSnapshotType() != LLSnapshotLivePreview::SNAPSHOT_LOCAL)
	{
		view->close();
	}
	else
	{
		mCloseCalled = view;
		if (mCallbackHappened)
		{
			doCloseAfterSave();
		}
		else
		{
			view->setEnabled(FALSE);
		}
	}
}

void LLSnapshotLivePreview::saveLocalDone(bool success)
{
	mCallbackHappened = true;
	mSaveLocalSuccessful = success;
	if (success)
	{
		// Disable Save button.
		addUsedBy(LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	}
	if (mCloseCalled)
	{
		doCloseAfterSave();
	}
}

void LLSnapshotLivePreview::doCloseAfterSave()
{
	if (mSaveLocalSuccessful)
	{
		// Relinquish image memory.
		mFormattedImage = NULL;
		mFormattedDataSize = 0;
		updateSnapshot(FALSE, FALSE);
		mCloseCalled->close();
	}
	else
	{
		mCloseCalled->setEnabled(TRUE);
	}
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSnapshot::Impl
{
public:
	Impl()
	:	mAvatarPauseHandles(),
		mLastToolset(NULL)
	{
	}
	~Impl()
	{
		//unpause avatars
		mAvatarPauseHandles.clear();

	}
	static void onClickDiscard(void* data);
	static void onClickKeep(void* data);
	static void onCommitSave(LLUICtrl* ctrl, void* data);
	static void onClickNewSnapshot(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	static void onClickTemporaryImage(LLUICtrl *ctrl, void* data);
	//static void onClickAdvanceSnap(LLUICtrl *ctrl, void* data);
	static void onClickLess(void* data) ;
	static void onClickMore(void* data) ;
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void onClickKeepOpenCheck(LLUICtrl *ctrl, void* data);
	static void onCommitQuality(LLUICtrl* ctrl, void* data);
	static void onCommitFeedResolution(LLUICtrl* ctrl, void* data);
	static void onCommitPostcardResolution(LLUICtrl* ctrl, void* data);
	static void onCommitTextureResolution(LLUICtrl* ctrl, void* data);
	static void onCommitLocalResolution(LLUICtrl* ctrl, void* data);
	static void onCommitFeedAspect(LLUICtrl* ctrl, void* data);
	static void onCommitPostcardAspect(LLUICtrl* ctrl, void* data);
	static void onCommitTextureAspect(LLUICtrl* ctrl, void* data);
	static void onCommitLocalAspect(LLUICtrl* ctrl, void* data);
	static void updateResolution(LLUICtrl* ctrl, void* data);
	static void updateAspect(LLUICtrl* ctrl, void* data);
	static void onCommitFreezeFrame(LLUICtrl* ctrl, void* data);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	static void onCommitSnapshotType(LLUICtrl* ctrl, void* data);
	static void onCommitSnapshotFormat(LLUICtrl* ctrl, void* data);
	static void onCommitCustomResolution(LLUICtrl *ctrl, void* data);
	static void onCommitCustomAspect(LLUICtrl *ctrl, void* data);

	static LLSnapshotLivePreview* getPreviewView(LLFloaterSnapshot *floater);
	static void setResolution(LLFloaterSnapshot* floater, const std::string& comboname);
	static void setAspect(LLFloaterSnapshot* floater, const std::string& comboname);
	static void updateControls(LLFloaterSnapshot* floater);
	static void updateLayout(LLFloaterSnapshot* floater);

	static LLHandle<LLView> sPreviewHandle;
	
private:
	static LLSnapshotLivePreview::ESnapshotType getTypeIndex(LLFloaterSnapshot* floater);
	static ESnapshotFormat getFormatIndex(LLFloaterSnapshot* floater);
	static void comboSetCustom(LLFloaterSnapshot *floater, const std::string& comboname);
	static void checkAutoSnapshot(LLSnapshotLivePreview* floater, BOOL update_thumbnail = FALSE);

public:
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;

	LLToolset*	mLastToolset;
};

// static
LLHandle<LLView> LLFloaterSnapshot::Impl::sPreviewHandle;

// static
LLSnapshotLivePreview* LLFloaterSnapshot::Impl::getPreviewView(LLFloaterSnapshot *floater)
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)sPreviewHandle.get();
	return previewp;
}

// static
LLSnapshotLivePreview::ESnapshotType LLFloaterSnapshot::Impl::getTypeIndex(LLFloaterSnapshot* floater)
{
	LLSnapshotLivePreview::ESnapshotType index = LLSnapshotLivePreview::SNAPSHOT_FEED;
	LLSD value = floater->childGetValue("snapshot_type_radio");
	const std::string id = value.asString();
	if (id == "feed")
		index = LLSnapshotLivePreview::SNAPSHOT_FEED;
	else if (id == "postcard")
		index = LLSnapshotLivePreview::SNAPSHOT_POSTCARD;
	else if (id == "texture")
		index = LLSnapshotLivePreview::SNAPSHOT_TEXTURE;
	else if (id == "local")
		index = LLSnapshotLivePreview::SNAPSHOT_LOCAL;
	return index;
}


// static
LLFloaterSnapshot::ESnapshotFormat LLFloaterSnapshot::Impl::getFormatIndex(LLFloaterSnapshot* floater)
{
	ESnapshotFormat index = SNAPSHOT_FORMAT_PNG;
	LLSD value = floater->childGetValue("local_format_combo");
	const std::string id = value.asString();
	if (id == "PNG")
		index = SNAPSHOT_FORMAT_PNG;
	else if (id == "JPEG")
		index = SNAPSHOT_FORMAT_JPEG;
	else if (id == "BMP")
		index = SNAPSHOT_FORMAT_BMP;
	return index;
}

// static
void LLFloaterSnapshot::Impl::setResolution(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setVisible(TRUE);
	updateResolution(combo, floater);	// to sync spinners with combo
}

// static
void LLFloaterSnapshot::Impl::setAspect(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setVisible(TRUE);
	updateAspect(combo, floater); 		// to sync spinner with combo
}

//static 
void LLFloaterSnapshot::Impl::updateLayout(LLFloaterSnapshot* floaterp)
{
	LLSnapshotLivePreview* previewp = getPreviewView(floaterp);

	S32 delta_height = gSavedSettings.getBOOL("AdvanceSnapshot") ? 0 : floaterp->getUIWinHeightShort() - floaterp->getUIWinHeightLong() ;

	if(!gSavedSettings.getBOOL("AdvanceSnapshot")) //set to original window resolution
	{
		floaterp->getChild<LLComboBox>("feed_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotFeedLastResolution", 0);
		floaterp->getChild<LLComboBox>("feed_aspect_combo")->setCurrentByIndex(0);	// 4:3
		gSavedSettings.setS32("SnapshotFeedLastAspect", 0);

		floaterp->getChild<LLComboBox>("postcard_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotPostcardLastResolution", 0);
		floaterp->getChild<LLComboBox>("postcard_aspect_combo")->setCurrentByIndex(0);	// Default (current window)
		gSavedSettings.setS32("SnapshotPostcardLastAspect", 0);

		floaterp->getChild<LLComboBox>("texture_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotTextureLastResolution", 0);
		floaterp->getChild<LLComboBox>("texture_aspect_combo")->setCurrentByIndex(0);	// Current window
		gSavedSettings.setS32("SnapshotTextureLastAspect", 0);

		floaterp->getChild<LLComboBox>("local_size_combo")->setCurrentByIndex(0);
		gSavedSettings.setS32("SnapshotLocalLastResolution", 0);
		floaterp->getChild<LLComboBox>("local_aspect_combo")->setCurrentByIndex(0);		// Current window
		gSavedSettings.setS32("SnapshotLocalLastAspect", 0);

		LLSnapshotLivePreview* previewp = getPreviewView(floaterp);
		S32 w, h;
		previewp->getSize(w, h);
		if (gViewerWindow->getWindowDisplayWidth() != w || gViewerWindow->getWindowDisplayHeight() != h)
		{
			previewp->setSize(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
			updateControls(floaterp);
		}
	}

	if (gSavedSettings.getBOOL("UseFreezeFrame"))
	{
		// stop all mouse events at fullscreen preview layer
		floaterp->getParent()->setMouseOpaque(TRUE);
		
		// shrink to smaller layout
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getUIWinHeightLong() + delta_height);

		// can see and interact with fullscreen preview now
		if (previewp)
		{
			previewp->setVisible(TRUE);
			previewp->setEnabled(TRUE);
		}

		//RN: freeze all avatars
		LLCharacter* avatarp;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			iter != LLCharacter::sInstances.end(); ++iter)
		{
			avatarp = *iter;
			sInstance->impl.mAvatarPauseHandles.push_back(avatarp->requestPause());
		}

		// freeze everything else
		gSavedSettings.setBOOL("FreezeTime", TRUE);

		if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
		{
			sInstance->impl.mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
			LLToolMgr::getInstance()->setCurrentToolset(gCameraToolset);
		}
	}
	else // turning off freeze frame mode
	{
		floaterp->getParent()->setMouseOpaque(FALSE);
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getUIWinHeightLong() + delta_height);
		if (previewp)
		{
			previewp->setVisible(FALSE);
			previewp->setEnabled(FALSE);
		}

		//RN: thaw all avatars
		sInstance->impl.mAvatarPauseHandles.clear();

		// thaw everything else
		gSavedSettings.setBOOL("FreezeTime", FALSE);

		// restore last tool (e.g. pie menu, etc)
		if (sInstance->impl.mLastToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(sInstance->impl.mLastToolset);
		}
	}
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// static
void LLFloaterSnapshot::Impl::updateControls(LLFloaterSnapshot* floater)
{
	DoutEntering(dc::notice, "LLFloaterSnapshot::Impl::updateControls()");
	std::string fee = gHippoGridManager->getConnectedGrid()->getUploadFee();
	if (gSavedSettings.getBOOL("TemporaryUpload"))
	{
		fee = fee.substr(0, fee.find_first_of("0123456789")) + "0";
	}
	floater->childSetLabelArg("upload_btn", "[UPLOADFEE]", fee);

	LLSnapshotLivePreview::ESnapshotType shot_type = (LLSnapshotLivePreview::ESnapshotType)gSavedSettings.getS32("LastSnapshotType");
	LLRadioGroup* snapshot_type_radio = floater->getChild<LLRadioGroup>("snapshot_type_radio");
	if (snapshot_type_radio) 
	{
		snapshot_type_radio->setSelectedIndex(shot_type);

		const child_list_t *childs = snapshot_type_radio->getChildList();
		if (childs) 
		{
			child_list_t::const_iterator it, end=childs->end();
			for (it=childs->begin(); it!=end; ++it) 
			{
				LLRadioCtrl *ctrl = dynamic_cast<LLRadioCtrl*>(*it);
				if (ctrl && (ctrl->getName() == "texture"))
				{
					ctrl->setLabelArg("[UPLOADFEE]", fee);
				}
			}
		}
	}
	ESnapshotFormat shot_format = (ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");

	floater->childSetVisible("feed_size_combo", FALSE);
	floater->childSetVisible("feed_aspect_combo", FALSE);
	floater->childSetVisible("postcard_size_combo", FALSE);
	floater->childSetVisible("postcard_aspect_combo", FALSE);
	floater->childSetVisible("texture_size_combo", FALSE);
	floater->childSetVisible("texture_aspect_combo", FALSE);
	floater->childSetVisible("local_size_combo", FALSE);
	floater->childSetVisible("local_aspect_combo", FALSE);

	LLSnapshotLivePreview* previewp = getPreviewView(floater);
	if (previewp)
	{
		LLViewerWindow::ESnapshotType layer_type =
			(shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL) ?
			(LLViewerWindow::ESnapshotType)gSavedSettings.getS32("SnapshotLayerType") :
			LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		// Do this before calling setResolution().
		previewp->setSnapshotBufferType(floater, layer_type);
	}
	BOOL is_advance = gSavedSettings.getBOOL("AdvanceSnapshot");
	if (is_advance)
	{
		switch(shot_type)
		{
		  case LLSnapshotLivePreview::SNAPSHOT_FEED:
			setResolution(floater, "feed_size_combo");
			setAspect(floater, "feed_aspect_combo");
			break;
		  case LLSnapshotLivePreview::SNAPSHOT_POSTCARD:
			setResolution(floater, "postcard_size_combo");
			setAspect(floater, "postcard_aspect_combo");
			break;
		  case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:
			setResolution(floater, "texture_size_combo");			
			setAspect(floater, "texture_aspect_combo");			
			break;
		  case  LLSnapshotLivePreview::SNAPSHOT_LOCAL:
			setResolution(floater, "local_size_combo");
			setAspect(floater, "local_aspect_combo");
			break;
		}
	}

	floater->getChild<LLComboBox>("local_format_combo")->selectNthItem(shot_format);

	floater->childSetVisible("upload_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	floater->childSetVisible("send_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	floater->childSetVisible("feed_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_FEED);
	floater->childSetVisible("save_btn",			shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	floater->childSetEnabled("layer_types",			shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL);

	BOOL is_local = shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL;
	BOOL show_slider = 
		shot_type == LLSnapshotLivePreview::SNAPSHOT_FEED ||
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD ||
		(is_local && shot_format == LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);

	floater->childSetVisible("more_btn", !is_advance); // the only item hidden in advanced mode
	floater->childSetVisible("less_btn",				is_advance);
	floater->childSetVisible("type_label2",				is_advance);
	floater->childSetVisible("type_label3",				is_advance);
	floater->childSetVisible("format_label",			is_advance && is_local);
	floater->childSetVisible("local_format_combo",		is_advance && is_local);
	floater->childSetVisible("layer_types",				is_advance);
	floater->childSetVisible("layer_type_label",		is_advance);
	floater->childSetVisible("snapshot_width",			is_advance);
	floater->childSetVisible("snapshot_height",			is_advance);
	floater->childSetVisible("aspect_ratio",			is_advance);
	floater->childSetVisible("ui_check",				is_advance);
	floater->childSetVisible("hud_check",				is_advance);
	floater->childSetVisible("keep_open_check",			is_advance);
	floater->childSetVisible("freeze_frame_check",		is_advance);
	floater->childSetVisible("auto_snapshot_check",		is_advance);
	floater->childSetVisible("image_quality_slider",	is_advance && show_slider);
	floater->childSetVisible("temp_check",				is_advance);

	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
	BOOL is_texture = shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_bytes)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}
	else
	{
		bytes_string = floater->getString("unknown");
	}
	S32 upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
	floater->childSetLabelArg("texture", "[AMOUNT]", llformat("%d",upload_cost));
	floater->childSetLabelArg("upload_btn", "[AMOUNT]", llformat("%d",upload_cost));
	floater->childSetTextArg("file_size_label", "[SIZE]", bytes_string);
	floater->childSetColor("file_size_label", 
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD 
		&& got_bytes
		&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLColor4::red : gColors.getColor( "LabelTextColor" ));

	if (previewp)
	{
		previewp->setSnapshotType(shot_type);
		previewp->setSnapshotFormat(shot_format);
	}

	bool up_to_date = previewp && previewp->getSnapshotUpToDate();
	bool can_upload = up_to_date && !previewp->isUsedBy(shot_type);
	floater->childSetEnabled("feed_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_FEED     && can_upload);
	floater->childSetEnabled("send_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD && can_upload && previewp->getDataSize() <= MAX_POSTCARD_DATASIZE);
	floater->childSetEnabled("upload_btn", shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE  && can_upload);
	floater->childSetEnabled("save_btn",   shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL    && can_upload);
	floater->childSetEnabled("temp_check", is_advance && is_texture);

	if (previewp)
	{
		previewp->showFreezeFrameSnapshot(up_to_date);
	}
}

// static
void LLFloaterSnapshot::Impl::checkAutoSnapshot(LLSnapshotLivePreview* previewp, BOOL update_thumbnail)
{
	if (previewp)
	{
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		previewp->updateSnapshot(autosnap, update_thumbnail, autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickDiscard(void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		view->close();
	}
}

// static
void LLFloaterSnapshot::Impl::onCommitFeedResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotFeedLastResolution", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_FEED);
	}
	updateResolution(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitPostcardResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotPostcardLastResolution", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	}
	updateResolution(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitTextureResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotTextureLastResolution", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	}
	updateResolution(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitLocalResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotLocalLastResolution", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	}
	updateResolution(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitFeedAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotFeedLastAspect", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_FEED);
	}
	updateAspect(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitPostcardAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotPostcardLastAspect", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_POSTCARD);
	}
	updateAspect(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitTextureAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotTextureLastAspect", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_TEXTURE);
	}
	updateAspect(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitLocalAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	gSavedSettings.setS32("SnapshotLocalLastAspect", combobox->getCurrentIndex());
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && previewp->isUsed())
	{
		previewp->addManualOverride(LLSnapshotLivePreview::SNAPSHOT_LOCAL);
	}
	updateAspect(ctrl, data);
}

// static
void LLFloaterSnapshot::Impl::onCommitSave(LLUICtrl* ctrl, void* data)
{
	if (ctrl->getValue().asString() == "saveas")
	{
		gViewerWindow->resetSnapshotLoc();
	}
	onClickKeep(data);
}

// Called from LLViewerWindow::saveImageNumbered, LLViewerWindow::saveImageNumbered_continued1 and LLViewerWindow::saveImageNumbered_continued2.
//static
void LLFloaterSnapshot::saveLocalDone(bool success)
{
	LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView(sInstance);
	if (previewp)
	{
		previewp->saveLocalDone(success);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickKeep(void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	
	if (previewp)
	{
		if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_FEED)
		{
			previewp->saveFeed();
		}
		else if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_POSTCARD)
		{
			LLFloaterPostcard* floater = previewp->savePostcard();
			// if still in snapshot mode, put postcard floater in snapshot floaterview
			// and link it to snapshot floater
			if (floater && !gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
			{
				gFloaterView->removeChild(floater);
				gSnapshotFloaterView->addChild(floater);
				view->addDependentFloater(floater, FALSE);
			}
		}
		else if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE)
		{
			previewp->saveTexture();
		}
		else
		{
			previewp->saveLocal();
		}

		if (gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
		{
			previewp->close(view);
		}
		else
		{
			checkAutoSnapshot(previewp);
		}

		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickNewSnapshot(void* data)
{
	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterSnapshot *)data);
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (previewp && view)
	{
		previewp->updateSnapshot(TRUE);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickAutoSnap(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "AutoSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickTemporaryImage(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		updateControls(view);
	}
}

void LLFloaterSnapshot::Impl::onClickMore(void* data)
{
	gSavedSettings.setBOOL( "AdvanceSnapshot", TRUE );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		view->translate( 0, view->getUIWinHeightShort() - view->getUIWinHeightLong() );
		view->reshape(view->getRect().getWidth(), view->getUIWinHeightLong());
		updateControls(view) ;
		updateLayout(view) ;
		if(getPreviewView(view))
		{
			getPreviewView(view)->setThumbnailImageSize() ;
		}
	}
}
void LLFloaterSnapshot::Impl::onClickLess(void* data)
{
	gSavedSettings.setBOOL( "AdvanceSnapshot", FALSE );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		view->translate( 0, view->getUIWinHeightLong() - view->getUIWinHeightShort() );
		view->reshape(view->getRect().getWidth(), view->getUIWinHeightShort());
		updateControls(view) ;
		updateLayout(view) ;
		if(getPreviewView(view))
		{
			getPreviewView(view)->setThumbnailImageSize() ;
		}
	}
}

// static
void LLFloaterSnapshot::Impl::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderUIInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view), TRUE);
		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickHUDCheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderHUDInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view), TRUE);
		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickKeepOpenCheck(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;

	gSavedSettings.setBOOL( "CloseSnapshotOnKeep", !check->get() );
}

// static
void LLFloaterSnapshot::Impl::onCommitQuality(LLUICtrl* ctrl, void* data)
{
	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());

	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterSnapshot *)data);
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
	checkAutoSnapshot(previewp, TRUE);
}

// static
void LLFloaterSnapshot::Impl::onCommitFreezeFrame(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !check_box)
	{
		return;
	}

	gSavedSettings.setBOOL("UseFreezeFrame", check_box->get());

	updateLayout(view);
}

static std::string lastSnapshotWidthName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedWidth";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailWidth";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryWidth";
	default:                                       return "LastSnapshotToDiskWidth";
	}
}

static std::string lastSnapshotHeightName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedHeight";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailHeight";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryHeight";
	default:                                       return "LastSnapshotToDiskHeight";
	}
}

static std::string lastSnapshotAspectName()
{
	switch(gSavedSettings.getS32("LastSnapshotType"))
	{
	case LLSnapshotLivePreview::SNAPSHOT_FEED:     return "LastSnapshotToFeedAspect";
	case LLSnapshotLivePreview::SNAPSHOT_POSTCARD: return "LastSnapshotToEmailAspect";
	case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:  return "LastSnapshotToInventoryAspect";
	default:                                       return "LastSnapshotToDiskAspect";
	}
}

// static
void LLFloaterSnapshot::Impl::updateResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !combobox)
	{
		return;
	}

	S32 used_w = 0;
	S32 used_h = 0;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	LLSnapshotLivePreview::ESnapshotType shot_type = (LLSnapshotLivePreview::ESnapshotType)gSavedSettings.getS32("LastSnapshotType");

#if 0 // AIFIXME
	bool up_to_date_now = false;
	bool up_to_date_after_toggling = false;
	if (previewp && previewp->isUsed() && !previewp->isOverriddenBy(shot_type))
	{
		up_to_date_now = previewp->getSnapshotUpToDateExceptForSize();
		previewp->mScaleInsteadOfCrop = !previewp->mScaleInsteadOfCrop;
		up_to_date_after_toggling = previewp->getSnapshotUpToDateExceptForSize();
		previewp->mScaleInsteadOfCrop = !previewp->mScaleInsteadOfCrop;
	}
	if (up_to_date_now || up_to_date_after_toggling)
	{
		// need_keep_aspect is set when mScaleInsteadOfCrop makes a difference.
		bool need_keep_aspect = up_to_date_now != up_to_date_after_toggling;

		// Force the size, and possibly mScaleInsteadOfCrop, to be the one of the existing snapshot.
		previewp->getUsedSize(used_w, used_h);
		LLComboBox* size_combo_box = NULL;
		LLComboBox* aspect_combo_box = NULL;
		switch(shot_type)
		{
			case LLSnapshotLivePreview::SNAPSHOT_FEED:
				size_combo_box = view->getChild<LLComboBox>("feed_size_combo");
				aspect_combo_box = view->getChild<LLComboBox>("feed_aspect_combo");
				break;
			case LLSnapshotLivePreview::SNAPSHOT_POSTCARD:
				size_combo_box = view->getChild<LLComboBox>("postcard_size_combo");
				aspect_combo_box = view->getChild<LLComboBox>("postcard_aspect_combo");
				break;
			case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:
				size_combo_box = view->getChild<LLComboBox>("texture_size_combo");
				aspect_combo_box = view->getChild<LLComboBox>("texture_aspect_combo");
				break;
			case LLSnapshotLivePreview::SNAPSHOT_LOCAL:
				size_combo_box = view->getChild<LLComboBox>("local_size_combo");
				aspect_combo_box = view->getChild<LLComboBox>("local_aspect_combo");
				break;
		}
		S32 index = 0;
		bool keep_aspect = true;
		bool needed_keep_aspect = up_to_date_now ? previewp->mScaleInsteadOfCrop : !previewp->mScaleInsteadOfCrop;	// Only valid if need_keep_aspect is true.
		S32 const custom = size_combo_box->getItemCount() - (shot_type == LLSnapshotLivePreview::SNAPSHOT_TEXTURE ? 0 : 1);	// Texture does not end on 'Custom'.
		while (index < custom)
		{
			if (!need_keep_aspect || keep_aspect == needed_keep_aspect)
			{
				size_combo_box->setCurrentByIndex(index);
				std::string sdstring = combobox->getSelectedValue();
				LLSD sdres;
				std::stringstream sstream(sdstring);
				LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());
				S32 width = sdres[0];
				S32 height = sdres[1];
				if (width == 0 || height == 0)
				{
					width = gViewerWindow->getWindowDisplayWidth();
					height = gViewerWindow->getWindowDisplayHeight();
				}
				if (width == used_w && height == used_h)
				{
					break;
				}
			}
			++index;
			keep_aspect = false;
		}
		if (index == custom)
		{
			size_combo_box->setCurrentByIndex(index);
			if (need_keep_aspect)
			{
				// Change mScaleInsteadOfCrop to needed_keep_aspect.
				gSavedSettings.setBOOL("KeepAspectForSnapshot", needed_keep_aspect);
			}
		}
	}
	else
#endif
	{
	    view->getChild<LLComboBox>("feed_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFeedLastResolution"));
	    view->getChild<LLComboBox>("feed_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFeedLastAspect"));
	    view->getChild<LLComboBox>("postcard_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastResolution"));
	    view->getChild<LLComboBox>("postcard_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastAspect"));
	    view->getChild<LLComboBox>("texture_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastResolution"));
	    view->getChild<LLComboBox>("texture_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastAspect"));
	    view->getChild<LLComboBox>("local_size_combo")->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastResolution"));
	    view->getChild<LLComboBox>("local_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastAspect"));
	}

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());
		
	S32 width = sdres[0];
	S32 height = sdres[1];
	
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;
		if (width == 0 || height == 0 || gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{
			// take resolution from current window size
			previewp->setSize(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		}
		else if (width == -1 || height == -1)
		{
			if (previewp->isUsed() && used_w != 0 && used_h != 0)
			{
				previewp->setSize(used_w, used_h);
			}
			else
			{
				// load last custom value
				previewp->setSize(gSavedSettings.getS32(lastSnapshotWidthName()), gSavedSettings.getS32(lastSnapshotHeightName()));
			}
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			previewp->setSize(width, height);
		}

		previewp->getSize(width, height);
	
		if(view->childGetValue("snapshot_width").asInteger() != width || view->childGetValue("snapshot_height").asInteger() != height)
		{
			view->childSetValue("snapshot_width", width);
			view->childSetValue("snapshot_height", height);
		}
	}

	// Set whether or not the spinners can be changed.

	LLSpinCtrl* width_spinner = view->getChild<LLSpinCtrl>("snapshot_width");
	LLSpinCtrl* height_spinner = view->getChild<LLSpinCtrl>("snapshot_height");

	if (gSavedSettings.getS32("LastSnapshotType") == LLSnapshotLivePreview::SNAPSHOT_TEXTURE ||
		gSavedSettings.getBOOL("RenderUIInSnapshot") ||
		gSavedSettings.getBOOL("RenderHUDInSnapshot"))
	{
		// Disable without making label gray.
		width_spinner->setAllowEdit(FALSE);
		width_spinner->setIncrement(0);
		height_spinner->setAllowEdit(FALSE);
		height_spinner->setIncrement(0);
	}
	else
	{
		width_spinner->setAllowEdit(TRUE);
		width_spinner->setIncrement(32);
		height_spinner->setAllowEdit(TRUE);
		height_spinner->setIncrement(32);
	}
}

// static
void LLFloaterSnapshot::Impl::updateAspect(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !combobox)
	{
		return;
	}

	LLSnapshotLivePreview* previewp = getPreviewView(view);
	//LLSnapshotLivePreview::ESnapshotType shot_type = (LLSnapshotLivePreview::ESnapshotType)gSavedSettings.getS32("LastSnapshotType");

	view->getChild<LLComboBox>("feed_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFeedLastAspect"));
	view->getChild<LLComboBox>("postcard_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastAspect"));
	view->getChild<LLComboBox>("texture_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastAspect"));
	view->getChild<LLComboBox>("local_aspect_combo")->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastAspect"));

	std::string sdstring = combobox->getSelectedValue();
	std::stringstream sstream;
	sstream << sdstring;
	F32 aspect;
	sstream >> aspect;

	if (aspect == -1)		// Custom
	{
		aspect = gSavedSettings.getF32(lastSnapshotAspectName());
	}
	if (aspect == 0)		// Current window
	{
		aspect = (F32)gViewerWindow->getWindowDisplayWidth() / gViewerWindow->getWindowDisplayHeight();
	}

	LLSpinCtrl* aspect_spinner = view->getChild<LLSpinCtrl>("aspect_ratio");

	// Sync the spinner and cache value.
	aspect_spinner->set(aspect);
	if (previewp)
	{
		previewp->setAspect(aspect);
		checkAutoSnapshot(previewp, TRUE);
	}

	// Set whether or not the spinners can be changed.
	if (gSavedSettings.getBOOL("RenderUIInSnapshot") ||
		gSavedSettings.getBOOL("RenderHUDInSnapshot"))
	{
		// Disable without making label gray.
		aspect_spinner->setAllowEdit(FALSE);
		aspect_spinner->setIncrement(0);
	}
	else
	{
		aspect_spinner->setAllowEdit(TRUE);
		aspect_spinner->setIncrement(llmax(0.01f, lltrunc(aspect) / 100.0f));
	}
}

// static
void LLFloaterSnapshot::Impl::onCommitLayerTypes(LLUICtrl* ctrl, void*data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLComboBox* combobox = (LLComboBox*)ctrl;
		gSavedSettings.setS32("SnapshotLayerType", combobox->getCurrentIndex());
		updateControls(view);
	}
}

//static 
void LLFloaterSnapshot::Impl::onCommitSnapshotType(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		gSavedSettings.setS32("LastSnapshotType", getTypeIndex(view));
		updateControls(view);
	}
}


//static 
void LLFloaterSnapshot::Impl::onCommitSnapshotFormat(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		gSavedSettings.setS32("SnapshotFormat", getFormatIndex(view));
		updateControls(view);
	}
}



// Sets the named size combo to "custom" mode.
// static
void LLFloaterSnapshot::Impl::comboSetCustom(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);

	combo->setCurrentByIndex(combo->getItemCount() - 1); // "custom" is always the last index

	if(comboname == "feed_size_combo") 
	{
		gSavedSettings.setS32("SnapshotFeedLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "postcard_size_combo") 
	{
		gSavedSettings.setS32("SnapshotPostcardLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "local_size_combo") 
	{
		gSavedSettings.setS32("SnapshotLocalLastResolution", combo->getCurrentIndex());
	}
	else if(comboname == "feed_aspect_combo")
	{
		gSavedSettings.setS32("SnapshotFeedLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "postcard_aspect_combo") 
	{
		gSavedSettings.setS32("SnapshotPostcardLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "texture_aspect_combo") 
	{
		gSavedSettings.setS32("SnapshotTextureLastAspect", combo->getCurrentIndex());
	}
	else if(comboname == "local_aspect_combo") 
	{
		gSavedSettings.setS32("SnapshotLocalLastAspect", combo->getCurrentIndex());
	}
}

//static
void LLFloaterSnapshot::Impl::onCommitCustomResolution(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		S32 w = llround(view->childGetValue("snapshot_width").asReal(), 1.0);
		S32 h = llround(view->childGetValue("snapshot_height").asReal(), 1.0);

		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if (previewp)
		{
			S32 curw,curh;
			previewp->getSize(curw, curh);
			
			if (w != curw || h != curh)
			{
				previewp->setMaxImageSize((S32)((LLSpinCtrl *)ctrl)->getMaxValue()) ;
				previewp->setSize(w,h);
				checkAutoSnapshot(previewp, FALSE);
				previewp->updateSnapshot(FALSE, TRUE);
				comboSetCustom(view, "feed_size_combo");
				comboSetCustom(view, "postcard_size_combo");
				comboSetCustom(view, "local_size_combo");
			}
		}

		gSavedSettings.setS32(lastSnapshotWidthName(), w);
		gSavedSettings.setS32(lastSnapshotHeightName(), h);

		updateControls(view);
	}
}

char const* LLSnapshotLivePreview::aspectComboName() const
{
	char const* result;
	switch(mSnapshotType)
	{
		case SNAPSHOT_FEED:
			result = "feed_aspect_combo";
			break;
		case SNAPSHOT_POSTCARD:
			result = "postcard_aspect_combo";
			break;
		case SNAPSHOT_TEXTURE:
			result = "texture_aspect_combo";
			break;
		case SNAPSHOT_LOCAL:
			result = "local_aspect_combo";
			break;
	}
	return result;
}

//static
void LLFloaterSnapshot::Impl::onCommitCustomAspect(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		F32 a = view->childGetValue("aspect_ratio").asReal();

		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if (previewp)
		{
			F32 cura = previewp->getAspect();
			
			if (a != cura)
			{
				previewp->setAspect(a);
				previewp->updateSnapshot(FALSE, TRUE);
				comboSetCustom(view, previewp->aspectComboName());
			}
		}

		gSavedSettings.setF32(lastSnapshotAspectName(), a);

		updateControls(view);
	}
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot()
	: LLFloater(std::string("Snapshot Floater")),
	  impl (*(new Impl))
{
}

// Destroys the object
LLFloaterSnapshot::~LLFloaterSnapshot()
{
	if (sInstance == this)
	{
		LLView::deleteViewByHandle(Impl::sPreviewHandle);
		Impl::sPreviewHandle = LLHandle<LLView>();
		sInstance = NULL;
	}

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl.mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl.mLastToolset);
	}

	delete &impl;
}


BOOL LLFloaterSnapshot::postBuild()
{
	childSetCommitCallback("snapshot_type_radio", Impl::onCommitSnapshotType, this);
	childSetCommitCallback("local_format_combo", Impl::onCommitSnapshotFormat, this);
	
	childSetAction("new_snapshot_btn", Impl::onClickNewSnapshot, this);

	childSetAction("more_btn", Impl::onClickMore, this);
	childSetAction("less_btn", Impl::onClickLess, this);

	childSetAction("upload_btn", Impl::onClickKeep, this);
	childSetAction("send_btn", Impl::onClickKeep, this);
	childSetAction("feed_btn", Impl::onClickKeep, this);
	childSetCommitCallback("save_btn", Impl::onCommitSave, this);
	childSetAction("discard_btn", Impl::onClickDiscard, this);

	childSetCommitCallback("image_quality_slider", Impl::onCommitQuality, this);
	childSetValue("image_quality_slider", gSavedSettings.getS32("SnapshotQuality"));

	childSetCommitCallback("snapshot_width", Impl::onCommitCustomResolution, this);
	childSetCommitCallback("snapshot_height", Impl::onCommitCustomResolution, this);

	childSetCommitCallback("aspect_ratio", Impl::onCommitCustomAspect, this);

	childSetCommitCallback("ui_check", Impl::onClickUICheck, this);
	childSetValue("ui_check", gSavedSettings.getBOOL("RenderUIInSnapshot"));

	childSetCommitCallback("hud_check", Impl::onClickHUDCheck, this);
	childSetValue("hud_check", gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	childSetCommitCallback("keep_open_check", Impl::onClickKeepOpenCheck, this);
	childSetValue("keep_open_check", !gSavedSettings.getBOOL("CloseSnapshotOnKeep"));

	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	childSetValue("layer_types", "colors");
	childSetEnabled("layer_types", FALSE);

	childSetValue("snapshot_width", gSavedSettings.getS32(lastSnapshotWidthName()));
	childSetValue("snapshot_height", gSavedSettings.getS32(lastSnapshotHeightName()));

	childSetValue("freeze_frame_check", gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", Impl::onCommitFreezeFrame, this);

	childSetValue("auto_snapshot_check", gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", Impl::onClickAutoSnap, this);
	childSetCommitCallback("temp_check", Impl::onClickTemporaryImage, this);

	childSetCommitCallback("feed_size_combo", Impl::onCommitFeedResolution, this);
	childSetCommitCallback("postcard_size_combo", Impl::onCommitPostcardResolution, this);
	childSetCommitCallback("texture_size_combo", Impl::onCommitTextureResolution, this);
	childSetCommitCallback("local_size_combo", Impl::onCommitLocalResolution, this);

	childSetCommitCallback("feed_aspect_combo", Impl::onCommitFeedAspect, this);
	childSetCommitCallback("postcard_aspect_combo", Impl::onCommitPostcardAspect, this);
	childSetCommitCallback("texture_aspect_combo", Impl::onCommitTextureAspect, this);
	childSetCommitCallback("local_aspect_combo", Impl::onCommitLocalAspect, this);

	// create preview window
	LLRect full_screen_rect = sInstance->getRootView()->getRect();
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(full_screen_rect);
	sInstance->getRootView()->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	sInstance->getRootView()->addChild(previewp);
	sInstance->getRootView()->addChild(gSnapshotFloaterView);

	gSavedSettings.setBOOL("TemporaryUpload",FALSE);
	childSetValue("temp_check",FALSE);

	Impl::sPreviewHandle = previewp->getHandle();

	impl.updateControls(this);

	return TRUE;
}

void LLFloaterSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = impl.getPreviewView(this);

	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	LLFloater::draw();

	if (previewp)
	{		
		if(previewp->getThumbnailImage())
		{
			// getRect() includes shadows and the title bar, therefore the real corners are as follows:
			S32 const left = 1;
			S32 const right = getRect().getWidth() - 1;
			S32 const top = getRect().getHeight() - 17;
			S32 const bottom = top - THUMBHEIGHT;
			// The offset needed to center the thumbnail is: center - thumbnailSize/2 =
			S32 offset_x = (left + right - previewp->getThumbnailWidth()) / 2;
			S32 offset_y = (bottom + top - previewp->getThumbnailHeight()) / 2;

			gGL.matrixMode(LLRender::MM_MODELVIEW);
			gl_rect_2d(left, top, right, bottom, LLColor4::grey4, true);
			gl_draw_scaled_image(offset_x, offset_y, 
					previewp->getThumbnailWidth(), previewp->getThumbnailHeight(), 
					previewp->getThumbnailImage(), LLColor4::white);	

			previewp->drawPreviewRect(offset_x, offset_y) ;
		}
	}
}

void LLFloaterSnapshot::onOpen()
{
	gSavedSettings.setBOOL("SnapshotBtnState", TRUE);
}

void LLFloaterSnapshot::onClose(bool app_quitting)
{
	gSnapshotFloaterView->setEnabled(FALSE);
	// Set invisible so it doesn't eat tooltips. JC
	gSnapshotFloaterView->setVisible(FALSE);
	gSavedSettings.setBOOL("SnapshotBtnState", FALSE);
	destroy();
}

// static
void LLFloaterSnapshot::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterSnapshot();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_snapshot.xml", NULL, FALSE);
		//move snapshot floater to special purpose snapshotfloaterview
		gFloaterView->removeChild(sInstance);
		gSnapshotFloaterView->addChild(sInstance);

		sInstance->impl.updateLayout(sInstance);
	}
	
	sInstance->open();		/* Flawfinder: ignore */
	sInstance->focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(sInstance, FALSE);
}

void LLFloaterSnapshot::hide(void*)
{
	if (sInstance && !sInstance->isDead())
	{
		sInstance->close();
	}
}

//static 
void LLFloaterSnapshot::update()
{
	BOOL changed = FALSE;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		 iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
	if(changed)
	{
		sInstance->impl.updateControls(sInstance);
	}
}

BOOL LLFloaterSnapshot::handleKeyHere(KEY key, MASK mask)
{
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if (freeze_time && key == KEY_ESCAPE && mask == MASK_NONE)
	{
		// Ignore trying to defocus the snapshot floater in Freeze Time mode.
		// However, if we're showing the fullscreen frozen preview, drop out of it;
		// otherwise, reset the cam.
		LLSnapshotLivePreview* previewp = LLFloaterSnapshot::Impl::getPreviewView(this);
		if (previewp && previewp->getShowFreezeFrameSnapshot())
		{
			previewp->showFreezeFrameSnapshot(false);
		}
		else
		{
			gAgentCamera.resetCamera();		// AIFIXME: This doesn't work! It should reset the camera view to behind the avatar again.
		}
		return TRUE;
	}
	return LLFloater::handleKeyHere(key, mask);
}


///----------------------------------------------------------------------------
/// Class LLSnapshotFloaterView
///----------------------------------------------------------------------------

LLSnapshotFloaterView::LLSnapshotFloaterView( const std::string& name, const LLRect& rect ) : LLFloaterView(name, rect)
{
	setMouseOpaque(TRUE);
	setEnabled(FALSE);
}

LLSnapshotFloaterView::~LLSnapshotFloaterView()
{
}

BOOL LLSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	// use default handler when not in freeze-frame mode
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleKey(key, mask, called_from_parent);
	}

	if (called_from_parent)
	{
		// pass all keystrokes down
		LLFloaterView::handleKey(key, mask, called_from_parent);
	}
	else
	{
		// bounce keystrokes back down
		LLFloaterView::handleKey(key, mask, TRUE);
	}
	return TRUE;
}

BOOL LLSnapshotFloaterView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleMouseDown(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseDown(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseDown( x, y, mask );
	}
	return TRUE;
}

BOOL LLSnapshotFloaterView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleMouseUp(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseUp(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseUp( x, y, mask );
	}
	return TRUE;
}

BOOL LLSnapshotFloaterView::handleHover(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	static const LLCachedControl<bool> freeze_time("FreezeTime",false);
	if(!freeze_time)
	{
		return LLFloaterView::handleHover(x, y, mask);
	}	
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleHover(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleHover( x, y, mask );
	}
	return TRUE;
}
