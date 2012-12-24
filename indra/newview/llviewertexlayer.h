/** 
 * @file llviewertexlayer.h
 * @brief Viewer Texture layer classes. Used for avatars.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_VIEWER_TEXLAYER_H
#define LL_VIEWER_TEXLAYER_H

#include <deque>
#include "lldynamictexture.h"
#include "llavatarappearancedefines.h"
#include "lltexlayerparams.h"
#include "lltexlayer.h"

class LLVOAvatar;
class LLVOAvatarSelf;
class LLImageTGA;
class LLImageRaw;
class LLXmlTreeNode;
class LLTexLayerSet;
class LLTexLayerSetInfo;
class LLTexLayerInfo;
class LLViewerTexLayerSetBuffer;
class LLWearable;
class LLViewerVisualParam;
class LLLocalTextureObject;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerInterface
//
// Interface class to generalize functionality shared by LLTexLayer 
// and LLTexLayerTemplate.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerInterface : public LLTexLayerInterfaceTMP
{
public:
	enum ERenderPass
	{
		RP_COLOR,
		RP_BUMP,
		RP_SHINE
	};

	LLTexLayerInterface(LLTexLayerSet* const layer_set);
	LLTexLayerInterface(const LLTexLayerInterface &layer, LLWearable *wearable);
	virtual ~LLTexLayerInterface() {}

	virtual BOOL			render(S32 x, S32 y, S32 width, S32 height) = 0;
	virtual void			deleteCaches() = 0;
	virtual BOOL			blendAlphaTexture(S32 x, S32 y, S32 width, S32 height) = 0;
	virtual BOOL			isInvisibleAlphaMask() const = 0;

	const LLTexLayerInfo* 	getInfo() const 			{ return mInfo; }
	virtual BOOL			setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // sets mInfo, calls initialization functions

	const std::string&		getName() const;
	const LLTexLayerSet* const getTexLayerSet() const 	{ return mTexLayerSet; }
	LLTexLayerSet* const 	getTexLayerSet() 			{ return mTexLayerSet; }

	void					invalidateMorphMasks();
	virtual void			setHasMorph(BOOL newval) 	{ mHasMorph = newval; }
	BOOL					hasMorph() const			{ return mHasMorph; }
	BOOL					isMorphValid() const		{ return mMorphMasksValid; }

	void					requestUpdate();
	virtual void			gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height) = 0;
	BOOL					hasAlphaParams() const 		{ return !mParamAlphaList.empty(); }

	ERenderPass				getRenderPass() const;
	BOOL					isVisibilityMask() const;

protected:
	const std::string&		getGlobalColor() const;
	LLViewerVisualParam*	getVisualParamPtr(S32 index) const;

protected:
	LLTexLayerSet* const	mTexLayerSet;
	const LLTexLayerInfo*	mInfo;
	BOOL					mMorphMasksValid;
	BOOL					mHasMorph;

	// Layers can have either mParamColorList, mGlobalColor, or mFixedColor.  They are looked for in that order.
	param_color_list_t		mParamColorList;
	param_alpha_list_t		mParamAlphaList;
	// 						mGlobalColor name stored in mInfo
	// 						mFixedColor value stored in mInfo
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerTemplate
//
// Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerTemplate : public LLTexLayerInterface
{
public:
	LLTexLayerTemplate(LLTexLayerSet* const layer_set);
	LLTexLayerTemplate(const LLTexLayerTemplate &layer);
	/*virtual*/ ~LLTexLayerTemplate();
	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);
	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		blendAlphaTexture(S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ void		setHasMorph(BOOL newval);
	/*virtual*/ void		deleteCaches();
	/*virtual*/ BOOL		isInvisibleAlphaMask() const;
protected:
	U32 					updateWearableCache() const;
	LLTexLayer* 			getLayer(U32 i) const;
private:
	typedef std::vector<LLWearable*> wearable_cache_t;
	mutable wearable_cache_t mWearableCache; // mutable b/c most get- require updating this cache
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayer
//
// A single texture layer.  Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayer : public LLTexLayerInterface
{
public:
	LLTexLayer(LLTexLayerSet* const layer_set);
	LLTexLayer(const LLTexLayer &layer, LLWearable *wearable);
	LLTexLayer(const LLTexLayerTemplate &layer_template, LLLocalTextureObject *lto, LLWearable *wearable);
	/*virtual*/ ~LLTexLayer();

	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);

	/*virtual*/ void		deleteCaches();
	const U8*				getAlphaData() const;

	BOOL					findNetColor(LLColor4* color) const;
	/*virtual*/ BOOL		blendAlphaTexture(S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	BOOL					renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color);
	void					addAlphaMask(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ BOOL		isInvisibleAlphaMask() const;

	void					setLTO(LLLocalTextureObject *lto) 	{ mLocalTextureObject = lto; }
	LLLocalTextureObject* 	getLTO() 							{ return mLocalTextureObject; }

	static void 			calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color);
protected:
	LLUUID					getUUID() const;
private:
	typedef std::map<U32, U8*> alpha_cache_t;
	alpha_cache_t			mAlphaCache;
	LLLocalTextureObject* 	mLocalTextureObject;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLViewerTexLayerSet
//
// An ordered set of texture layers that gets composited into a single texture.
// Only exists for llavatarappearanceself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerTexLayerSet : public LLTexLayerSet
{
	friend class LLViewerTexLayerSetBuffer;
public:
	LLViewerTexLayerSet(LLAvatarAppearance* const appearance);
	virtual ~LLViewerTexLayerSet();
	
	/*virtual*/ const LLTexLayerSetInfo* 	getInfo() const 			{ return mInfo; }
	/*virtual*/ BOOL						setInfo(const LLTexLayerSetInfo *info); // This sets mInfo and calls initialization functions

	/*virtual*/ BOOL						render(S32 x, S32 y, S32 width, S32 height);
	/*virtual*/ void						renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear = false);

	/*virtual*/ BOOL						isBodyRegion(const std::string& region) const;
	/*virtual*/ LLTexLayerSetBuffer*		getComposite();
	/*virtual*/ const LLTexLayerSetBuffer* 	getComposite() const; // Do not create one if it doesn't exist.
	/*virtual*/	void						destroyComposite();
	/*virtual*/	void						gatherMorphMaskAlpha(U8 *data, S32 width, S32 height);
	/*virtual*/	void						applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);
	/*virtual*/	BOOL						isMorphValid() const;
	/*virtual*/	void						invalidateMorphMasks();
	/*virtual*/	void						deleteCaches();
	/*virtual*/	LLTexLayerInterface*		findLayerByName(const std::string& name);
	/*virtual*/	void						cloneTemplates(LLLocalTextureObject *lto, LLAvatarAppearanceDefines::ETextureIndex tex_index, LLWearable* wearable);
	/*virtual*/ const std::string			getBodyRegionName() const;					
	/*virtual*/ BOOL						hasComposite() const 		{ return (mComposite.notNull()); }
	/*virtual*/ LLAvatarAppearanceDefines::EBakedTextureIndex getBakedTexIndex() const { return mBakedTexIndex; }
	/*virtual*/ void						setBakedTexIndex(LLAvatarAppearanceDefines::EBakedTextureIndex index) { mBakedTexIndex = index; }
	/*virtual*/ BOOL						isVisible() const 			{ return mIsVisible; }
	
	/*virtual*/void						requestUpdate();
	void						requestUpload();
	void						cancelUpload();
	BOOL						isLocalTextureDataAvailable() const;
	BOOL						isLocalTextureDataFinal() const;
	void						updateComposite();
	/*virtual*/void				createComposite();
	void						setUpdatesEnabled(BOOL b);
	BOOL						getUpdatesEnabled()	const 	{ return mUpdatesEnabled; }

	LLVOAvatarSelf*				getAvatar();
	const LLVOAvatarSelf*		getAvatar()	const;
	LLViewerTexLayerSetBuffer*	getViewerComposite();
	const LLViewerTexLayerSetBuffer*	getViewerComposite() const;

private:
	typedef std::vector<LLTexLayerInterface *> layer_list_t;
	layer_list_t				mLayerList;
	layer_list_t				mMaskLayerList;
	LLPointer<LLViewerTexLayerSetBuffer>	mComposite;
	BOOL						mUpdatesEnabled;
	BOOL						mIsVisible;

	LLAvatarAppearanceDefines::EBakedTextureIndex mBakedTexIndex;
	const LLTexLayerSetInfo* 	mInfo;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSetInfo
//
// Contains shared layer set data.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSetInfo
{
	friend class LLViewerTexLayerSet;
public:
	LLTexLayerSetInfo();
	~LLTexLayerSetInfo();
	BOOL parseXml(LLXmlTreeNode* node);
	void createVisualParams(LLAvatarAppearance *appearance);
	S32 getWidth() const { return mWidth; }
	S32 getHeight() const { return mHeight; }
private:
	std::string				mBodyRegion;
	S32						mWidth;
	S32						mHeight;
	std::string				mStaticAlphaFileName;
	BOOL					mClearAlpha; // Set alpha to 1 for this layerset (if there is no mStaticAlphaFileName)
	typedef std::vector<LLTexLayerInfo*> layer_info_list_t;
	layer_info_list_t		mLayerInfoList;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLViewerTexLayerSetBuffer
//
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerTexLayerSetBuffer : public LLTexLayerSetBuffer, public LLViewerDynamicTexture
{
	LOG_CLASS(LLViewerTexLayerSetBuffer);

public:
	LLViewerTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height);
	virtual ~LLViewerTexLayerSetBuffer();

public:
	/*virtual*/ S8          getType() const;
	BOOL					isInitialized(void) const;
	static void				dumpTotalByteCount();
	const std::string		dumpTextureInfo() const;
	virtual void 			restoreGLTexture();
	virtual void 			destroyGLTexture();
protected:
	void					pushProjection() const;
	void					popProjection() const;
private:
	LLViewerTexLayerSet*	getViewerTexLayerSet() 
		{ return dynamic_cast<LLViewerTexLayerSet*> (mTexLayerSet); }
	const LLViewerTexLayerSet*	getViewerTexLayerSet() const
		{ return dynamic_cast<const LLViewerTexLayerSet*> (mTexLayerSet); }
	static S32				sGLByteCount;

	//--------------------------------------------------------------------
	// Render
	//--------------------------------------------------------------------
public:
	/*virtual*/ BOOL		needsRender();
protected:
	BOOL					render(S32 x, S32 y, S32 width, S32 height);
	virtual void			preRender(BOOL clear_depth);
	virtual void			postRender(BOOL success);
	virtual BOOL			render();	
	
	//--------------------------------------------------------------------
	// Uploads
	//--------------------------------------------------------------------
public:
	void					requestUpload();
	void					cancelUpload();
	BOOL					uploadNeeded() const; 			// We need to upload a new texture
	BOOL					uploadInProgress() const; 		// We have started uploading a new texture and are awaiting the result
	BOOL					uploadPending() const; 			// We are expecting a new texture to be uploaded at some point
	static void				onTextureUploadComplete(const LLUUID& uuid,
													void* userdata,
													S32 result, LLExtStat ext_status);
protected:
	BOOL					isReadyToUpload() const;
	void					doUpload(); 					// Does a read back and upload.
	void					conditionalRestartUploadTimer();
private:
	BOOL					mNeedsUpload; 					// Whether we need to send our baked textures to the server
	U32						mNumLowresUploads; 				// Number of times we've sent a lowres version of our baked textures to the server
	BOOL					mUploadPending; 				// Whether we have received back the new baked textures
	LLUUID					mUploadID; 						// The current upload process (null if none).
	LLFrameTimer    		mNeedsUploadTimer; 				// Tracks time since upload was requested and performed.
	S32						mUploadFailCount;				// Number of consecutive upload failures
	LLFrameTimer    		mUploadRetryTimer; 				// Tracks time since last upload failure.

	//--------------------------------------------------------------------
	// Updates
	//--------------------------------------------------------------------
public:
	void					requestUpdate();
	BOOL					requestUpdateImmediate();
protected:
	BOOL					isReadyToUpdate() const;
	void					doUpdate();
	void					restartUpdateTimer();
private:
	BOOL					mNeedsUpdate; 					// Whether we need to locally update our baked textures
	U32						mNumLowresUpdates; 				// Number of times we've locally updated with lowres version of our baked textures
	LLFrameTimer    		mNeedsUpdateTimer; 				// Tracks time since update was requested and performed.
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLBakedUploadData
//
// Used by LLViewerTexLayerSetBuffer for a callback.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct LLBakedUploadData
{
	LLBakedUploadData(const LLVOAvatarSelf* avatar,
					  LLViewerTexLayerSet* layerset, 
					  const LLUUID& id,
					  bool highest_res);
	~LLBakedUploadData() {}
	const LLUUID				mID;
	const LLVOAvatarSelf*		mAvatar; // note: backlink only; don't LLPointer 
	LLViewerTexLayerSet*		mTexLayerSet;
   	const U64					mStartTime;	// for measuring baked texture upload time
   	const bool					mIsHighestRes; // whether this is a "final" bake, or intermediate low res
};

#endif  // LL_VIEWER_TEXLAYER_H

