/** 
 * @file lltexlayer.h
 * @brief Texture layer classes. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLTEXLAYER_H
#define LL_LLTEXLAYER_H

#include <deque>
#include "llglslshader.h"
#include "llgltexture.h"
#include "llavatarappearancedefines.h"
#include "lltexlayerparams.h"

class LLAvatarAppearance;
class LLImageTGA;
class LLImageRaw;
class LLLocalTextureObject;
class LLXmlTreeNode;
class LLTexLayerSet;
class LLTexLayerSetInfo;
class LLTexLayerInfo;
class LLTexLayerSetBuffer;
class LLWearable;
class LLViewerVisualParam;
class LLLocalTextureObject;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSet
//
// An ordered set of texture layers that gets composited into a single texture.
// Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSet
{
	friend class LLTexLayerSetBuffer;
public:
	LLTexLayerSet(LLAvatarAppearance* const appearance);
	virtual ~LLTexLayerSet() {};

	virtual LLTexLayerSetBuffer*		getComposite() = 0;
	virtual const LLTexLayerSetBuffer* 	getComposite() const = 0;
	virtual void				createComposite() = 0;
	virtual void						destroyComposite() = 0;
	virtual void						gatherMorphMaskAlpha(U8 *data, S32 width, S32 height) = 0;

	virtual const LLTexLayerSetInfo* 	getInfo() const = 0;
	virtual BOOL						setInfo(const LLTexLayerSetInfo *info) = 0;

	virtual BOOL						render(S32 x, S32 y, S32 width, S32 height) = 0;
	virtual void						renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear = false) = 0;

	virtual BOOL						isBodyRegion(const std::string& region) const = 0;
	virtual void						applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components) = 0;
	virtual BOOL						isMorphValid() const = 0;
	virtual void						requestUpdate() = 0;
	virtual void						invalidateMorphMasks() = 0;
	virtual void						deleteCaches() = 0;
	virtual	LLTexLayerInterface*		findLayerByName(const std::string& name) = 0;
	virtual void						cloneTemplates(LLLocalTextureObject *lto, LLAvatarAppearanceDefines::ETextureIndex tex_index, LLWearable* wearable) = 0;

	LLAvatarAppearance*			getAvatarAppearance()	const		{ return mAvatarAppearance; }
	virtual const std::string			getBodyRegionName() const = 0;
	virtual BOOL						hasComposite() const = 0;
	virtual LLAvatarAppearanceDefines::EBakedTextureIndex getBakedTexIndex() const = 0;
	virtual void						setBakedTexIndex(LLAvatarAppearanceDefines::EBakedTextureIndex index) = 0;
	virtual	BOOL						isVisible() const = 0;

	static BOOL					sHasCaches;

	LLAvatarAppearance*	const	mAvatarAppearance; // note: backlink only; don't make this an LLPointer.
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSetBuffer
//
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSetBuffer : public virtual LLRefCount
{
	LOG_CLASS(LLTexLayerSetBuffer);

public:
	LLTexLayerSetBuffer(LLTexLayerSet* const owner) : mTexLayerSet(owner) {};
	virtual ~LLTexLayerSetBuffer() {}
	LLTexLayerSet* const	mTexLayerSet;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerStaticImageList
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerStaticImageList : public LLSingleton<LLTexLayerStaticImageList>
{
public:
	LLTexLayerStaticImageList();
	~LLTexLayerStaticImageList();
	LLGLTexture*		getTexture(const std::string& file_name, BOOL is_mask);
	LLImageTGA*			getImageTGA(const std::string& file_name);
	void				deleteCachedImages();
	void				dumpByteCount() const;
protected:
	BOOL				loadImageRaw(const std::string& file_name, LLImageRaw* image_raw);
private:
	LLStringTable 		mImageNames;
	typedef std::map<const char*, LLPointer<LLGLTexture> > texture_map_t;
	texture_map_t 		mStaticImageList;
	typedef std::map<const char*, LLPointer<LLImageTGA> > image_tga_map_t;
	image_tga_map_t 	mStaticImageListTGA;
	S32 				mGLBytes;
	S32 				mTGABytes;
};

#endif  // LL_LLTEXLAYER_H
