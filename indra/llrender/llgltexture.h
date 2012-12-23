/** 
 * @file llglviewertexture.h
 * @brief Object for managing opengl textures
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


#ifndef LL_GL_TEXTURE_H
#define LL_GL_TEXTURE_H

#include "lltexture.h"
#include "llgl.h"

class LLImageRaw;

//
//this the parent for the class LLViewerTexture
//through the following virtual functions, the class LLViewerTexture can be reached from /llrender.
//
class LLGLTexture : public LLTexture
{
public:
	enum
	{
		MAX_IMAGE_SIZE_DEFAULT = 1024,
		INVALID_DISCARD_LEVEL = 0x7fff
	};

	enum EBoostLevel
	{
		BOOST_NONE 			= 0,
		BOOST_AVATAR_BAKED	,
		BOOST_AVATAR		,
		BOOST_CLOUDS		,
		BOOST_SCULPTED      ,
		
		BOOST_HIGH 			= 10,
		BOOST_BUMP          ,
		BOOST_TERRAIN		, // has to be high priority for minimap / low detail
		BOOST_SELECTED		,		
		BOOST_AVATAR_BAKED_SELF	,
		BOOST_AVATAR_SELF	, // needed for baking avatar
		BOOST_SUPER_HIGH    , //textures higher than this need to be downloaded at the required resolution without delay.
		BOOST_HUD			,
		BOOST_ICON			,
		BOOST_UI			,
		BOOST_PREVIEW		,
		BOOST_MAP			,
		BOOST_MAP_VISIBLE	,		
		BOOST_MAX_LEVEL,

		//other texture Categories
		LOCAL = BOOST_MAX_LEVEL,
		AVATAR_SCRATCH_TEX,
		DYNAMIC_TEX,
		MEDIA,
		//ATLAS,
		OTHER,
		MAX_GL_IMAGE_CATEGORY
	};

	//static S32 getTotalNumOfCategories() ;
	//static S32 getIndexFromCategory(S32 category) ;
	//static S32 getCategoryFromIndex(S32 index) ;

protected:
	virtual ~LLGLTexture() {};
	LOG_CLASS(LLGLTexture);

public:
	//LLGLTexture(BOOL usemipmaps = TRUE);
	//LLGLTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	//LLGLTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) ;

	virtual void dump() = 0;	// debug info to llinfos

	virtual const LLUUID& getID() const = 0;

	virtual void setBoostLevel(S32 level) = 0;
	virtual S32  getBoostLevel() = 0;

	virtual S32 getFullWidth() const = 0;
	virtual S32 getFullHeight() const = 0;

	virtual void generateGLTexture() = 0;
	virtual void destroyGLTexture() = 0;

	//---------------------------------------------------------------------------------------------
	//functions to access LLImageGL
	//---------------------------------------------------------------------------------------------
	//*virtual*/S32	       getWidth(S32 discard_level = -1) const;
	//*virtual*/S32	       getHeight(S32 discard_level = -1) const;

	virtual BOOL       hasGLTexture() const = 0;
	virtual LLGLuint   getTexName() const = 0;	
	virtual BOOL       createGLTexture() = 0;
	virtual BOOL       createGLTexture(S32 discard_level, const LLImageRaw* imageraw, S32 usename = 0, BOOL to_create = TRUE, S32 category = LLGLTexture::OTHER) = 0;

	virtual void       setFilteringOption(LLTexUnit::eTextureFilterOptions option) = 0;
	virtual void       setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, BOOL swap_bytes = FALSE) = 0;
	virtual void       setAddressMode(LLTexUnit::eTextureAddressMode mode) = 0;
	virtual BOOL       setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, bool fast_update = false) = 0;
	virtual BOOL       setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool fast_update = false) = 0;
	virtual void       setGLTextureCreated (bool initialized) = 0;
	virtual void       setCategory(S32 category)  = 0;

	virtual LLTexUnit::eTextureAddressMode getAddressMode(void) const  = 0;
	virtual S32        getMaxDiscardLevel() const = 0;
	virtual S32        getDiscardLevel() const = 0;
	virtual S8         getComponents() const = 0;
	virtual BOOL       getBoundRecently() const = 0;
	virtual S32        getTextureMemory() const  = 0;
	virtual LLGLenum   getPrimaryFormat() const = 0;
	virtual BOOL       getIsAlphaMask() const  = 0;
	virtual LLTexUnit::eTextureType getTarget(void) const  = 0;
	virtual BOOL       getMask(const LLVector2 &tc) = 0;
	virtual F32        getTimePassedSinceLastBound() = 0;
	virtual BOOL       getMissed() const  = 0;
	virtual BOOL       isJustBound()const  = 0;
	virtual void       forceUpdateBindStats(void) const = 0;

	//virtual U32        getTexelsInAtlas() const  = 0;
	//virtual U32        getTexelsInGLTexture() const  = 0;
	//virtual BOOL       isGLTextureCreated() const  = 0;
	//virtual S32        getDiscardLevelInAtlas() const  = 0;
	//---------------------------------------------------------------------------------------------
	//end of functions to access LLImageGL
	//---------------------------------------------------------------------------------------------

	//-----------------
	//*virtual*/ void setActive()  = 0;
	virtual void forceActive()  = 0;
	virtual void setNoDelete()  = 0;
	virtual void dontDiscard()  = 0;
	virtual BOOL getDontDiscard() const  = 0;
	//-----------------	

private:
	virtual void cleanup() = 0;
	virtual void init(bool firstinit) = 0;

protected:

	virtual void setTexelsPerImage() = 0;

	//note: do not make this function public.
	//*virtual*/ LLImageGL* getGLTexture() const  = 0;

protected:	
	typedef enum 
	{
		DELETED = 0,         //removed from memory
		DELETION_CANDIDATE,  //ready to be removed from memory
		INACTIVE,            //not be used for the last certain period (i.e., 30 seconds).
		ACTIVE,              //just being used, can become inactive if not being used for a certain time (10 seconds).
		NO_DELETE = 99       //stay in memory, can not be removed.
	} LLGLTextureState;

};

#endif // LL_GL_TEXTURE_H

