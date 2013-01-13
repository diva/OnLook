/** 
 * @file llfloaterfeed.h
 * @brief Feed send floater, allows setting caption and whether or not to include location.
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * Copyright (c) 2012, Aleric Inglewood.
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

#ifndef LL_LLFLOATERFEED_H
#define LL_LLFLOATERFEED_H

#include "llfloater.h"			// LLFloater
#include "v2math.h"				// LLVector2
#include "llpointer.h"			// LLPointer

class LLImagePNG;
class LLViewerTexture;
class LLFocusableElement;
class LLTextEditor;

class LLFloaterFeed : public LLFloater
{
public:
	LLFloaterFeed(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& img_scale, int index);
	virtual ~LLFloaterFeed();

	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	static LLFloaterFeed* showFromSnapshot(LLImagePNG* png, LLViewerTexture* img, LLVector2 const& img_scale, int index);

	void onClickCancel();
	void onClickPost();
	void onMsgFormFocusRecieved(LLFocusableElement* receiver, LLTextEditor* caption);

protected:
	LLPointer<LLImagePNG> mPNGImage;
	LLPointer<LLViewerTexture> mViewerImage;
	LLVector2 const mImageScale;
	int mSnapshotIndex;
	bool mHasFirstMsgFocus;
};

#endif // LL_LLFLOATERFEED_H
