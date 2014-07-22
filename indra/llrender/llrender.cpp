 /** 
 * @file llrender.cpp
 * @brief LLRender implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llrender.h"

#include "llvertexbuffer.h"
#include "llcubemap.h"
#include "llglslshader.h"
#include "llimagegl.h"
#include "llrendertarget.h"
#include "lltexture.h"
#include "llshadermgr.h"
#include "llmatrix4a.h"

LLRender gGL;

// Handy copies of last good GL matrices
//Would be best to migrate these to LLMatrix4a and LLVector4a, but that's too divergent right now.
LLMatrix4a gGLModelView;
LLMatrix4a gGLLastModelView;
LLMatrix4a gGLPreviousModelView;
LLMatrix4a gGLLastProjection;
LLMatrix4a gGLProjection;
S32 gGLViewport[4];

U32 LLRender::sUICalls = 0;
U32 LLRender::sUIVerts = 0;
U32 LLTexUnit::sWhiteTexture = 0;
bool LLRender::sGLCoreProfile = false;

static const U32 LL_NUM_TEXTURE_LAYERS = 32; 
static const U32 LL_NUM_LIGHT_UNITS = 8;

static GLenum sGLTextureType[] =
{
	GL_TEXTURE_2D,
	GL_TEXTURE_RECTANGLE_ARB,
	GL_TEXTURE_CUBE_MAP_ARB
	//,GL_TEXTURE_2D_MULTISAMPLE  Don't use.
};

static GLint sGLAddressMode[] =
{	
	GL_REPEAT,
	GL_MIRRORED_REPEAT,
	GL_CLAMP_TO_EDGE
};

static GLenum sGLCompareFunc[] =
{
	GL_NEVER,
	GL_ALWAYS,
	GL_LESS,
	GL_LEQUAL,
	GL_EQUAL,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_GREATER
};

const U32 immediate_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXCOORD0;

static GLenum sGLBlendFactor[] =
{
	GL_ONE,
	GL_ZERO,
	GL_DST_COLOR,
	GL_SRC_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_ALPHA,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,

	GL_ZERO // 'BF_UNDEF'
};

LLTexUnit::LLTexUnit(S32 index)
: mCurrTexType(TT_NONE), mCurrBlendType(TB_MULT), 
mCurrColorOp(TBO_MULT), mCurrAlphaOp(TBO_MULT),
mCurrColorSrc1(TBS_TEX_COLOR), mCurrColorSrc2(TBS_PREV_COLOR),
mCurrAlphaSrc1(TBS_TEX_ALPHA), mCurrAlphaSrc2(TBS_PREV_ALPHA),
mCurrColorScale(1), mCurrAlphaScale(1), mCurrTexture(0),
mHasMipMaps(false)
{
	llassert_always(index < (S32)LL_NUM_TEXTURE_LAYERS);
	mIndex = index;
}

//static
U32 LLTexUnit::getInternalType(eTextureType type)
{
	return sGLTextureType[type];
}

void LLTexUnit::refreshState(void)
{
	// We set dirty to true so that the tex unit knows to ignore caching
	// and we reset the cached tex unit state

	gGL.flush();
	
	glActiveTextureARB(GL_TEXTURE0_ARB + mIndex);

	//
	// Per apple spec, don't call glEnable/glDisable when index exceeds max texture units
	// http://www.mailinglistarchive.com/html/mac-opengl@lists.apple.com/2008-07/msg00653.html
	//
	bool enableDisable = !LLGLSLShader::sNoFixedFunction && 
		(mIndex < gGLManager.mNumTextureUnits) /*&& mCurrTexType != LLTexUnit::TT_MULTISAMPLE_TEXTURE*/;
		
	if (mCurrTexType != TT_NONE)
	{
		if (enableDisable)
		{
			glEnable(sGLTextureType[mCurrTexType]);
		}
		
		glBindTexture(sGLTextureType[mCurrTexType], mCurrTexture);
	}
	else
	{
		if (enableDisable)
		{
			glDisable(GL_TEXTURE_2D);
		}
		
		glBindTexture(GL_TEXTURE_2D, 0);	
	}

	if (mCurrBlendType != TB_COMBINE)
	{
		setTextureBlendType(mCurrBlendType);
	}
	else
	{
		setTextureCombiner(mCurrColorOp, mCurrColorSrc1, mCurrColorSrc2, false);
		setTextureCombiner(mCurrAlphaOp, mCurrAlphaSrc1, mCurrAlphaSrc2, true);
	}
}

void LLTexUnit::activate(void)
{
	if (mIndex < 0) return;

	if ((S32)gGL.mCurrTextureUnitIndex != mIndex || gGL.mDirty)
	{
		gGL.flush();
		glActiveTextureARB(GL_TEXTURE0_ARB + mIndex);
		gGL.mCurrTextureUnitIndex = mIndex;
	}
}

void LLTexUnit::enable(eTextureType type)
{
	if (mIndex < 0) return;

	if ( (mCurrTexType != type || gGL.mDirty) && (type != TT_NONE) )
	{
		stop_glerror();
		activate();
		stop_glerror();
		if (mCurrTexType != TT_NONE && !gGL.mDirty)
		{
			disable(); // Force a disable of a previous texture type if it's enabled.
			stop_glerror();
		}
		mCurrTexType = type;

		gGL.flush();
		if (!LLGLSLShader::sNoFixedFunction && 
			//type != LLTexUnit::TT_MULTISAMPLE_TEXTURE &&
			mIndex < gGLManager.mNumTextureUnits)
		{
			stop_glerror();
			glEnable(sGLTextureType[type]);
			stop_glerror();
		}
	}
}

void LLTexUnit::disable(void)
{
	if (mIndex < 0) return;

	if (mCurrTexType != TT_NONE)
	{
		activate();
		unbind(mCurrTexType);
		gGL.flush();
		if (!LLGLSLShader::sNoFixedFunction &&
			//mCurrTexType != LLTexUnit::TT_MULTISAMPLE_TEXTURE &&
			mIndex < gGLManager.mNumTextureUnits)
		{
			glDisable(sGLTextureType[mCurrTexType]);
		}
		
		mCurrTexType = TT_NONE;
	}
}

bool LLTexUnit::bind(LLTexture* texture, bool for_rendering, bool forceBind)
{
	stop_glerror();
	if (mIndex < 0) return false;

	LLImageGL* gl_tex = NULL ;
	if (texture == NULL || !(gl_tex = texture->getGLTexture()))
	{
		llwarns << "NULL LLTexUnit::bind texture" << llendl;
		return false;
	}

	if (!gl_tex->getTexName()) //if texture does not exist
	{
		//if deleted, will re-generate it immediately
		texture->forceImmediateUpdate() ;

		gl_tex->forceUpdateBindStats() ;
		return texture->bindDefaultImage(mIndex);
	}

	//in audit, replace the selected texture by the default one.
	if(gAuditTexture && for_rendering && LLImageGL::sCurTexPickSize > 0)
	{
		if(texture->getWidth() * texture->getHeight() == LLImageGL::sCurTexPickSize)
		{
			gl_tex->updateBindStats(gl_tex->mTextureMemory);
			return bind(LLImageGL::sHighlightTexturep.get());
		}
	}
	if ((mCurrTexture != gl_tex->getTexName()) || forceBind)
	{
		gGL.flush();
		activate();
		enable(gl_tex->getTarget());
		mCurrTexture = gl_tex->getTexName();
		glBindTexture(sGLTextureType[gl_tex->getTarget()], mCurrTexture);
		if(gl_tex->updateBindStats(gl_tex->mTextureMemory))
		{
			texture->setActive() ;
			texture->updateBindStatsForTester() ;
		}
		mHasMipMaps = gl_tex->mHasMipMaps;
		if (gl_tex->mTexOptionsDirty)
		{
			gl_tex->mTexOptionsDirty = false;
			setTextureAddressMode(gl_tex->mAddressMode);
			setTextureFilteringOption(gl_tex->mFilterOption);
		}
	}
	return true;
}

bool LLTexUnit::bind(LLImageGL* texture, bool for_rendering, bool forceBind)
{
	stop_glerror();
	if (mIndex < 0) return false;

	if(!texture)
	{
		llwarns << "NULL LLTexUnit::bind texture" << llendl;
		return false;
	}

	if(!texture->getTexName())
	{
		if(LLImageGL::sDefaultGLTexture && LLImageGL::sDefaultGLTexture->getTexName())
		{
			return bind(LLImageGL::sDefaultGLTexture) ;
		}
		stop_glerror();
		return false ;
	}

	if ((mCurrTexture != texture->getTexName()) || forceBind)
	{
		gGL.flush();
		stop_glerror();
		activate();
		stop_glerror();
		enable(texture->getTarget());
		stop_glerror();
		mCurrTexture = texture->getTexName();
		glBindTexture(sGLTextureType[texture->getTarget()], mCurrTexture);
		stop_glerror();
		texture->updateBindStats(texture->mTextureMemory);		
		mHasMipMaps = texture->mHasMipMaps;
		if (texture->mTexOptionsDirty)
		{
			stop_glerror();
			texture->mTexOptionsDirty = false;
			setTextureAddressMode(texture->mAddressMode);
			setTextureFilteringOption(texture->mFilterOption);
			stop_glerror();
		}
	}

	stop_glerror();

	return true;
}

bool LLTexUnit::bind(LLCubeMap* cubeMap)
{
	if (mIndex < 0) return false;

	gGL.flush();

	if (cubeMap == NULL)
	{
		llwarns << "NULL LLTexUnit::bind cubemap" << llendl;
		return false;
	}

	if (cubeMap->mImages[0].isNull())
	{
		llwarns << "NULL LLTexUnit::bind cubeMap->mImages[0]" << llendl;
		return false;
	}
	if (mCurrTexture != cubeMap->mImages[0]->getTexName())
	{
		if (gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps)
		{
			activate();
			enable(LLTexUnit::TT_CUBE_MAP);
			mCurrTexture = cubeMap->mImages[0]->getTexName();
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, mCurrTexture);
			mHasMipMaps = cubeMap->mImages[0]->mHasMipMaps;
			cubeMap->mImages[0]->updateBindStats(cubeMap->mImages[0]->mTextureMemory);
			if (cubeMap->mImages[0]->mTexOptionsDirty)
			{
				cubeMap->mImages[0]->mTexOptionsDirty = false;
				setTextureAddressMode(cubeMap->mImages[0]->mAddressMode);
				setTextureFilteringOption(cubeMap->mImages[0]->mFilterOption);
			}
			return true;
		}
		else
		{
			llwarns << "Using cube map without extension!" << llendl;
			return false;
		}
	}
	return true;
}

// LLRenderTarget is unavailible on the mapserver since it uses FBOs.
#if !LL_MESA_HEADLESS
bool LLTexUnit::bind(LLRenderTarget* renderTarget, bool bindDepth)
{
	if (mIndex < 0) return false;

	gGL.flush();

	if (bindDepth)
	{
		if (renderTarget->hasStencil())
		{
			llerrs << "Cannot bind a render buffer for sampling.  Allocate render target without a stencil buffer if sampling of depth buffer is required." << llendl;
		}

		bindManual(renderTarget->getUsage(), renderTarget->getDepth());
	}
	else
	{
		bindManual(renderTarget->getUsage(), renderTarget->getTexture());
	}

	return true;
}
#endif // LL_MESA_HEADLESS

bool LLTexUnit::bindManual(eTextureType type, U32 texture, bool hasMips)
{
	if (mIndex < 0)  
	{
		return false;
	}
	
	if(mCurrTexture != texture)
	{
		gGL.flush();

		activate();
		enable(type);
		mCurrTexture = texture;
		glBindTexture(sGLTextureType[type], texture);
		mHasMipMaps = hasMips;
	}
	return true;
}

void LLTexUnit::unbind(eTextureType type)
{
	stop_glerror();

	if (mIndex < 0) return;

	//always flush and activate for consistency 
	//   some code paths assume unbind always flushes and sets the active texture
	gGL.flush();
	activate();

	// Disabled caching of binding state.
	if (mCurrTexType == type)
	{
		mCurrTexture = 0;
		if (LLGLSLShader::sNoFixedFunction && type == LLTexUnit::TT_TEXTURE)
		{
			glBindTexture(sGLTextureType[type], sWhiteTexture);
		}
		else
		{
			glBindTexture(sGLTextureType[type], 0);
		}
		stop_glerror();
	}
}

void LLTexUnit::setTextureAddressMode(eTextureAddressMode mode)
{
	if (mIndex < 0 || mCurrTexture == 0) return;

	gGL.flush();

	activate();

	glTexParameteri (sGLTextureType[mCurrTexType], GL_TEXTURE_WRAP_S, sGLAddressMode[mode]);
	glTexParameteri (sGLTextureType[mCurrTexType], GL_TEXTURE_WRAP_T, sGLAddressMode[mode]);
	if (mCurrTexType == TT_CUBE_MAP)
	{
		glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, sGLAddressMode[mode]);
	}
}

void LLTexUnit::setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	if (mIndex < 0 || mCurrTexture == 0 /*|| mCurrTexType == LLTexUnit::TT_MULTISAMPLE_TEXTURE*/) return;

	gGL.flush();

	if (option == TFO_POINT)
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (option >= TFO_TRILINEAR && mHasMipMaps)
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} 
	else if (option >= TFO_BILINEAR)
	{
		if (mHasMipMaps)
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}
		else
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
	else
	{
		if (mHasMipMaps)
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
		else
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (gGLManager.mHasAnisotropic)
	{
		if (LLImageGL::sGlobalUseAnisotropic && option == TFO_ANISOTROPIC)
		{
			if (gGL.mMaxAnisotropy < 1.f)
			{
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gGL.mMaxAnisotropy);

				llinfos << "gGL.mMaxAnisotropy: " << gGL.mMaxAnisotropy << llendl ;
				gGL.mMaxAnisotropy = llmax(1.f, gGL.mMaxAnisotropy) ;
			}
			glTexParameterf(sGLTextureType[mCurrTexType], GL_TEXTURE_MAX_ANISOTROPY_EXT, gGL.mMaxAnisotropy);
		}
		else
		{
			glTexParameterf(sGLTextureType[mCurrTexType], GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);
		}
	}
}

void LLTexUnit::setTextureBlendType(eTextureBlendType type)
{
	if (LLGLSLShader::sNoFixedFunction)
	{ //texture blend type means nothing when using shaders
		return;
	}

	if (mIndex < 0) return;

	// Do nothing if it's already correctly set.
	if (mCurrBlendType == type && !gGL.mDirty)
	{
		return;
	}

	gGL.flush();

	activate();
	mCurrBlendType = type;
	S32 scale_amount = 1;
	switch (type) 
	{
		case TB_REPLACE:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			break;
		case TB_ADD:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			break;
		case TB_MULT:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		case TB_MULT_X2:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			scale_amount = 2;
			break;
		case TB_ALPHA_BLEND:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
			break;
		case TB_COMBINE:
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			break;
		default:
			llerrs << "Unknown Texture Blend Type: " << type << llendl;
			break;
	}
	setColorScale(scale_amount);
	setAlphaScale(1);
}

GLint LLTexUnit::getTextureSource(eTextureBlendSrc src)
{
	switch(src)
	{
		// All four cases should return the same value.
		case TBS_PREV_COLOR:
		case TBS_PREV_ALPHA:
		case TBS_ONE_MINUS_PREV_COLOR:
		case TBS_ONE_MINUS_PREV_ALPHA:
			return GL_PREVIOUS_ARB;

		// All four cases should return the same value.
		case TBS_TEX_COLOR:
		case TBS_TEX_ALPHA:
		case TBS_ONE_MINUS_TEX_COLOR:
		case TBS_ONE_MINUS_TEX_ALPHA:
			return GL_TEXTURE;

		// All four cases should return the same value.
		case TBS_VERT_COLOR:
		case TBS_VERT_ALPHA:
		case TBS_ONE_MINUS_VERT_COLOR:
		case TBS_ONE_MINUS_VERT_ALPHA:
			return GL_PRIMARY_COLOR_ARB;

		// All four cases should return the same value.
		case TBS_CONST_COLOR:
		case TBS_CONST_ALPHA:
		case TBS_ONE_MINUS_CONST_COLOR:
		case TBS_ONE_MINUS_CONST_ALPHA:
			return GL_CONSTANT_ARB;

		default:
			llwarns << "Unknown eTextureBlendSrc: " << src << ".  Using Vertex Color instead." << llendl;
			return GL_PRIMARY_COLOR_ARB;
	}
}

GLint LLTexUnit::getTextureSourceType(eTextureBlendSrc src, bool isAlpha)
{
	switch(src)
	{
		// All four cases should return the same value.
		case TBS_PREV_COLOR:
		case TBS_TEX_COLOR:
		case TBS_VERT_COLOR:
		case TBS_CONST_COLOR:
			return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;

		// All four cases should return the same value.
		case TBS_PREV_ALPHA:
		case TBS_TEX_ALPHA:
		case TBS_VERT_ALPHA:
		case TBS_CONST_ALPHA:
			return GL_SRC_ALPHA;

		// All four cases should return the same value.
		case TBS_ONE_MINUS_PREV_COLOR:
		case TBS_ONE_MINUS_TEX_COLOR:
		case TBS_ONE_MINUS_VERT_COLOR:
		case TBS_ONE_MINUS_CONST_COLOR:
			return (isAlpha) ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE_MINUS_SRC_COLOR;

		// All four cases should return the same value.
		case TBS_ONE_MINUS_PREV_ALPHA:
		case TBS_ONE_MINUS_TEX_ALPHA:
		case TBS_ONE_MINUS_VERT_ALPHA:
		case TBS_ONE_MINUS_CONST_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;

		default:
			llwarns << "Unknown eTextureBlendSrc: " << src << ".  Using Source Color or Alpha instead." << llendl;
			return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;
	}
}

void LLTexUnit::setTextureCombiner(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2, bool isAlpha)
{
	if (LLGLSLShader::sNoFixedFunction)
	{ //register combiners do nothing when not using fixed function
		return;
	}	

	if (mIndex < 0) return;

	activate();
	if (mCurrBlendType != TB_COMBINE || gGL.mDirty)
	{
		mCurrBlendType = TB_COMBINE;
		gGL.flush();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	}

	// We want an early out, because this function does a LOT of stuff.
	if ( ( (isAlpha && (mCurrAlphaOp == op) && (mCurrAlphaSrc1 == src1) && (mCurrAlphaSrc2 == src2))
			|| (!isAlpha && (mCurrColorOp == op) && (mCurrColorSrc1 == src1) && (mCurrColorSrc2 == src2)) ) && !gGL.mDirty)
	{
		return;
	}

	gGL.flush();

	// Get the gl source enums according to the eTextureBlendSrc sources passed in
	GLint source1 = getTextureSource(src1);
	GLint source2 = getTextureSource(src2);
	// Get the gl operand enums according to the eTextureBlendSrc sources passed in
	GLint operand1 = getTextureSourceType(src1, isAlpha);
	GLint operand2 = getTextureSourceType(src2, isAlpha);
	// Default the scale amount to 1
	S32 scale_amount = 1;
	GLenum comb_enum, src0_enum, src1_enum, src2_enum, operand0_enum, operand1_enum, operand2_enum;
	
	if (isAlpha)
	{
		// Set enums to ALPHA ones
		comb_enum = GL_COMBINE_ALPHA_ARB;
		src0_enum = GL_SOURCE0_ALPHA_ARB;
		src1_enum = GL_SOURCE1_ALPHA_ARB;
		src2_enum = GL_SOURCE2_ALPHA_ARB;
		operand0_enum = GL_OPERAND0_ALPHA_ARB;
		operand1_enum = GL_OPERAND1_ALPHA_ARB;
		operand2_enum = GL_OPERAND2_ALPHA_ARB;

		// cache current combiner
		mCurrAlphaOp = op;
		mCurrAlphaSrc1 = src1;
		mCurrAlphaSrc2 = src2;
	}
	else 
	{
		// Set enums to RGB ones
		comb_enum = GL_COMBINE_RGB_ARB;
		src0_enum = GL_SOURCE0_RGB_ARB;
		src1_enum = GL_SOURCE1_RGB_ARB;
		src2_enum = GL_SOURCE2_RGB_ARB;
		operand0_enum = GL_OPERAND0_RGB_ARB;
		operand1_enum = GL_OPERAND1_RGB_ARB;
		operand2_enum = GL_OPERAND2_RGB_ARB;

		// cache current combiner
		mCurrColorOp = op;
		mCurrColorSrc1 = src1;
		mCurrColorSrc2 = src2;
	}

	switch(op)
	{
		case TBO_REPLACE:
			// Slightly special syntax (no second sources), just set all and return.
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, src0_enum, source1);
			glTexEnvi(GL_TEXTURE_ENV, operand0_enum, operand1);
			(isAlpha) ? setAlphaScale(1) : setColorScale(1);
			return;

		case TBO_MULT:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_MODULATE);
			break;

		case TBO_MULT_X2:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_MODULATE);
			scale_amount = 2;
			break;

		case TBO_MULT_X4:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_MODULATE);
			scale_amount = 4;
			break;

		case TBO_ADD:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_ADD);
			break;

		case TBO_ADD_SIGNED:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_ADD_SIGNED_ARB);
			break;

		case TBO_SUBTRACT:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_SUBTRACT_ARB);
			break;

		case TBO_LERP_VERT_ALPHA:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, src2_enum, GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, operand2_enum, GL_SRC_ALPHA);
			break;

		case TBO_LERP_TEX_ALPHA:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, src2_enum, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, operand2_enum, GL_SRC_ALPHA);
			break;

		case TBO_LERP_PREV_ALPHA:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, src2_enum, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, operand2_enum, GL_SRC_ALPHA);
			break;

		case TBO_LERP_CONST_ALPHA:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, src2_enum, GL_CONSTANT_ARB);
			glTexEnvi(GL_TEXTURE_ENV, operand2_enum, GL_SRC_ALPHA);
			break;

		case TBO_LERP_VERT_COLOR:
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_INTERPOLATE);
			glTexEnvi(GL_TEXTURE_ENV, src2_enum, GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, operand2_enum, (isAlpha) ? GL_SRC_ALPHA : GL_SRC_COLOR);
			break;

		default:
			llwarns << "Unknown eTextureBlendOp: " << op << ".  Setting op to replace." << llendl;
			// Slightly special syntax (no second sources), just set all and return.
			glTexEnvi(GL_TEXTURE_ENV, comb_enum, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, src0_enum, source1);
			glTexEnvi(GL_TEXTURE_ENV, operand0_enum, operand1);
			(isAlpha) ? setAlphaScale(1) : setColorScale(1);
			return;
	}

	// Set sources, operands, and scale accordingly
	glTexEnvi(GL_TEXTURE_ENV, src0_enum, source1);
	glTexEnvi(GL_TEXTURE_ENV, operand0_enum, operand1);
	glTexEnvi(GL_TEXTURE_ENV, src1_enum, source2);
	glTexEnvi(GL_TEXTURE_ENV, operand1_enum, operand2);
	(isAlpha) ? setAlphaScale(scale_amount) : setColorScale(scale_amount);
}

void LLTexUnit::setColorScale(S32 scale)
{
	if (mCurrColorScale != scale || gGL.mDirty)
	{
		mCurrColorScale = scale;
		gGL.flush();
		glTexEnvi( GL_TEXTURE_ENV, GL_RGB_SCALE, scale );
	}
}

void LLTexUnit::setAlphaScale(S32 scale)
{
	if (mCurrAlphaScale != scale || gGL.mDirty)
	{
		mCurrAlphaScale = scale;
		gGL.flush();
		glTexEnvi( GL_TEXTURE_ENV, GL_ALPHA_SCALE, scale );
	}
}

// Useful for debugging that you've manually assigned a texture operation to the correct 
// texture unit based on the currently set active texture in opengl.
void LLTexUnit::debugTextureUnit(void)
{
	if (mIndex < 0) return;

	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
	if ((GL_TEXTURE0_ARB + mIndex) != activeTexture)
	{
		U32 set_unit = (activeTexture - GL_TEXTURE0_ARB);
		llwarns << "Incorrect Texture Unit!  Expected: " << set_unit << " Actual: " << mIndex << llendl;
	}
}

LLLightState::LLLightState(S32 index)
: mIndex(index),
  mEnabled(false),
  mConstantAtten(1.f),
  mLinearAtten(0.f),
  mQuadraticAtten(0.f),
  mSpotExponent(0.f),
  mSpotCutoff(180.f)
{
	if (mIndex == 0)
	{
		mDiffuse.set(1,1,1,1);
		mSpecular.set(1,1,1,1);
	}

	mAmbient.set(0,0,0,1);
	mPosition.set(0,0,1,0);
	mSpotDirection.set(0,0,-1);
}

void LLLightState::enable()
{
	if (!mEnabled)
	{
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glEnable(GL_LIGHT0+mIndex);
		}
		mEnabled = true;
	}
}

void LLLightState::disable()
{
	if (mEnabled)
	{
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glDisable(GL_LIGHT0+mIndex);
		}
		mEnabled = false;
	}
}

void LLLightState::setDiffuse(const LLColor4& diffuse)
{
	if (mDiffuse != diffuse)
	{
		++gGL.mLightHash;
		mDiffuse = diffuse;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightfv(GL_LIGHT0+mIndex, GL_DIFFUSE, mDiffuse.mV);
		}
	}
}

void LLLightState::setAmbient(const LLColor4& ambient)
{
	if (mAmbient != ambient)
	{
		++gGL.mLightHash;
		mAmbient = ambient;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightfv(GL_LIGHT0+mIndex, GL_AMBIENT, mAmbient.mV);
		}
	}
}

void LLLightState::setSpecular(const LLColor4& specular)
{
	if (mSpecular != specular)
	{
		++gGL.mLightHash;
		mSpecular = specular;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightfv(GL_LIGHT0+mIndex, GL_SPECULAR, mSpecular.mV);
		}
	}
}

void LLLightState::setPosition(const LLVector4& position)
{
	//always set position because modelview matrix may have changed
	++gGL.mLightHash;
	mPosition = position;
	if (!LLGLSLShader::sNoFixedFunction)
	{
		glLightfv(GL_LIGHT0+mIndex, GL_POSITION, mPosition.mV);
	}
	else
	{ //transform position by current modelview matrix
		LLVector4a pos;
		pos.loadua(position.mV);

		gGL.getModelviewMatrix().rotate4(pos,pos);

		mPosition.set(pos.getF32ptr());
	}

}

void LLLightState::setConstantAttenuation(const F32& atten)
{
	if (mConstantAtten != atten)
	{
		mConstantAtten = atten;
		++gGL.mLightHash;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightf(GL_LIGHT0+mIndex, GL_CONSTANT_ATTENUATION, atten);
		}
	}
}

void LLLightState::setLinearAttenuation(const F32& atten)
{
	if (mLinearAtten != atten)
	{
		++gGL.mLightHash;
		mLinearAtten = atten;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightf(GL_LIGHT0+mIndex, GL_LINEAR_ATTENUATION, atten);
		}
	}
}

void LLLightState::setQuadraticAttenuation(const F32& atten)
{
	if (mQuadraticAtten != atten)
	{
		++gGL.mLightHash;
		mQuadraticAtten = atten;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightf(GL_LIGHT0+mIndex, GL_QUADRATIC_ATTENUATION, atten);
		}
	}
}

void LLLightState::setSpotExponent(const F32& exponent)
{
	if (mSpotExponent != exponent)
	{
		++gGL.mLightHash;
		mSpotExponent = exponent;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightf(GL_LIGHT0+mIndex, GL_SPOT_EXPONENT, exponent);
		}
	}
}

void LLLightState::setSpotCutoff(const F32& cutoff)
{
	if (mSpotCutoff != cutoff)
	{
		++gGL.mLightHash;
		mSpotCutoff = cutoff;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightf(GL_LIGHT0+mIndex, GL_SPOT_CUTOFF, cutoff);
		}
	}
}

void LLLightState::setSpotDirection(const LLVector3& direction)
{
	//always set direction because modelview matrix may have changed
	++gGL.mLightHash;
	mSpotDirection = direction;
	if (!LLGLSLShader::sNoFixedFunction)
	{
		glLightfv(GL_LIGHT0+mIndex, GL_SPOT_DIRECTION, direction.mV);
	}
	else
	{ //transform direction by current modelview matrix
		LLVector4a dir;
		dir.load3(direction.mV);

		gGL.getModelviewMatrix().rotate(dir,dir);

		mSpotDirection.set(dir.getF32ptr());
	}
}

LLRender::eBlendFactor blendfunc_debug[4]={LLRender::BF_UNDEF};
LLRender::LLRender()
  : mDirty(false),
    mCount(0),
	mQuadCycle(0),
    mMode(LLRender::TRIANGLES),
    mCurrTextureUnitIndex(0),
    mMaxAnisotropy(0.f) 
{	
	mTexUnits.reserve(LL_NUM_TEXTURE_LAYERS);
	for (U32 i = 0; i < LL_NUM_TEXTURE_LAYERS; i++)
	{
		mTexUnits.push_back(new LLTexUnit(i));
	}
	mDummyTexUnit = new LLTexUnit(-1);

	for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; ++i)
	{
		mLightState.push_back(new LLLightState(i));
	}

	for (U32 i = 0; i < 4; i++)
	{
		mCurrColorMask[i] = true;
	}

	mCurrAlphaFunc = CF_DEFAULT;
	mCurrAlphaFuncVal = 0.01f;
	mCurrBlendColorSFactor = BF_UNDEF;
	mCurrBlendAlphaSFactor = BF_UNDEF;
	mCurrBlendColorDFactor = BF_UNDEF;
	mCurrBlendAlphaDFactor = BF_UNDEF;

	mMatrixMode = LLRender::MM_MODELVIEW;
	
	for (U32 i = 0; i < NUM_MATRIX_MODES; ++i)
	{
		mMatIdx[i] = 0;
		mMatHash[i] = 0;
		mCurMatHash[i] = 0xFFFFFFFF;
	}

	mLightHash = 0;
	
	//Init base matrix for each mode
	for(S32 i = 0; i < NUM_MATRIX_MODES; ++i)
	{
		mMatrix[i][0].setIdentity();
	}

	gGLModelView.setIdentity();
	gGLLastModelView.setIdentity();
	gGLPreviousModelView.setIdentity();
	gGLLastProjection.setIdentity();
	gGLProjection.setIdentity();
}

LLRender::~LLRender()
{
	shutdown();
}

void LLRender::init()
{
	if (sGLCoreProfile && !LLVertexBuffer::sUseVAO)
	{ //bind a dummy vertex array object so we're core profile compliant
#ifdef GL_ARB_vertex_array_object
		U32 ret;
		glGenVertexArrays(1, &ret);
		glBindVertexArray(ret);
#endif
	}

	llassert_always(mBuffer.isNull()) ;
	stop_glerror();
	mBuffer = new LLVertexBuffer(immediate_mask, 0);
	mBuffer->allocateBuffer(4096, 0, TRUE);
	mBuffer->getVertexStrider(mVerticesp);
	mBuffer->getTexCoord0Strider(mTexcoordsp);
	mBuffer->getColorStrider(mColorsp);
	stop_glerror();
}

void LLRender::shutdown()
{
	for (U32 i = 0; i < mTexUnits.size(); i++)
	{
		delete mTexUnits[i];
	}
	mTexUnits.clear();
	delete mDummyTexUnit;
	mDummyTexUnit = NULL;

	for (U32 i = 0; i < mLightState.size(); ++i)
	{
		delete mLightState[i];
	}
	mLightState.clear();
	mBuffer = NULL ;
}

void LLRender::refreshState(void)
{
	mDirty = true;

	U32 active_unit = mCurrTextureUnitIndex;

	for (U32 i = 0; i < mTexUnits.size(); i++)
	{
		mTexUnits[i]->refreshState();
	}
	
	mTexUnits[active_unit]->activate();

	setColorMask(mCurrColorMask[0], mCurrColorMask[1], mCurrColorMask[2], mCurrColorMask[3]);
	
	setAlphaRejectSettings(mCurrAlphaFunc, mCurrAlphaFuncVal);

	//Singu note: Also reset glBlendFunc
	blendFunc(mCurrBlendColorSFactor,mCurrBlendColorDFactor,mCurrBlendAlphaSFactor,mCurrBlendAlphaDFactor);

	mDirty = false;
}

void LLRender::syncLightState()
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	if (!shader)
	{
		return;
	}

	if (shader->mLightHash != mLightHash)
	{
		shader->mLightHash = mLightHash;

		LLVector4 position[8];
		LLVector3 direction[8];
		LLVector3 attenuation[8];
		LLVector3 diffuse[8];

		for (U32 i = 0; i < 8; i++)
		{
			LLLightState* light = mLightState[i];

			position[i] = light->mPosition;
			direction[i] = light->mSpotDirection;
			attenuation[i].set(light->mLinearAtten, light->mQuadraticAtten, light->mSpecular.mV[3]);
			diffuse[i].set(light->mDiffuse.mV);
		}

		shader->uniform4fv(LLShaderMgr::LIGHT_POSITION, 8, position[0].mV);
		shader->uniform3fv(LLShaderMgr::LIGHT_DIRECTION, 8, direction[0].mV);
		shader->uniform3fv(LLShaderMgr::LIGHT_ATTENUATION, 8, attenuation[0].mV);
		shader->uniform3fv(LLShaderMgr::LIGHT_DIFFUSE, 8, diffuse[0].mV);
		shader->uniform4fv(LLShaderMgr::LIGHT_AMBIENT, 1, mAmbientLightColor.mV);
		//HACK -- duplicate sunlight color for compatibility with drivers that can't deal with multiple shader objects referencing the same uniform
		shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, 1, diffuse[0].mV);
	}
}

void LLRender::syncMatrices()
{
	stop_glerror();

	U32 name[] = 
	{
		LLShaderMgr::MODELVIEW_MATRIX,
		LLShaderMgr::PROJECTION_MATRIX,
		LLShaderMgr::TEXTURE_MATRIX0,
		LLShaderMgr::TEXTURE_MATRIX1,
		LLShaderMgr::TEXTURE_MATRIX2,
		LLShaderMgr::TEXTURE_MATRIX3,
	};

	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	static LLMatrix4a cached_mvp;
	static U32 cached_mvp_mdv_hash = 0xFFFFFFFF;
	static U32 cached_mvp_proj_hash = 0xFFFFFFFF;
	
	static LLMatrix4a cached_normal;
	static U32 cached_normal_hash = 0xFFFFFFFF;

	if (shader)
	{
		llassert(shader);

		bool mvp_done = false;

		U32 i = MM_MODELVIEW;
		if (mMatHash[i] != shader->mMatHash[i])
		{ //update modelview, normal, and MVP
			const LLMatrix4a& mat = mMatrix[i][mMatIdx[i]];
			
			shader->uniformMatrix4fv(name[i], 1, GL_FALSE, mat.getF32ptr());
			shader->mMatHash[i] = mMatHash[i];

			//update normal matrix
			S32 loc = shader->getUniformLocation(LLShaderMgr::NORMAL_MATRIX);
			if (loc > -1)
			{
				if (cached_normal_hash != mMatHash[i])
				{
					cached_normal = mat;
					cached_normal.invert();
					cached_normal.transpose();
					cached_normal_hash = mMatHash[i];
				}
				
				const LLMatrix4a& norm = cached_normal;

				LLVector3 norms[3];
				norms[0].set(norm.getRow<0>().getF32ptr());
				norms[1].set(norm.getRow<1>().getF32ptr());
				norms[2].set(norm.getRow<2>().getF32ptr());

				shader->uniformMatrix3fv(LLShaderMgr::NORMAL_MATRIX, 1, GL_FALSE, norms[0].mV);
			}

			//update MVP matrix
			mvp_done = true;
			loc = shader->getUniformLocation(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX);
			if (loc > -1)
			{
				U32 proj = MM_PROJECTION;

				if (cached_mvp_mdv_hash != mMatHash[i] || cached_mvp_proj_hash != mMatHash[MM_PROJECTION])
				{
					cached_mvp.setMul(mMatrix[proj][mMatIdx[proj]], mat);
					cached_mvp_mdv_hash = mMatHash[i];
					cached_mvp_proj_hash = mMatHash[MM_PROJECTION];
				}

				shader->uniformMatrix4fv(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, cached_mvp.getF32ptr());
			}
		}


		i = MM_PROJECTION;
		if (mMatHash[i] != shader->mMatHash[i])
		{ //update projection matrix, normal, and MVP
			const LLMatrix4a& mat = mMatrix[i][mMatIdx[i]];
			
			shader->uniformMatrix4fv(name[i], 1, GL_FALSE, mat.getF32ptr());
			shader->mMatHash[i] = mMatHash[i];

			if (!mvp_done)
			{
				//update MVP matrix
				S32 loc = shader->getUniformLocation(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX);
				if (loc > -1)
				{
					if (cached_mvp_mdv_hash != mMatHash[i] || cached_mvp_proj_hash != mMatHash[MM_PROJECTION])
					{
						U32 mdv = MM_MODELVIEW;
						cached_mvp.setMul(mat,mMatrix[mdv][mMatIdx[mdv]]);
						cached_mvp_mdv_hash = mMatHash[MM_MODELVIEW];
						cached_mvp_proj_hash = mMatHash[MM_PROJECTION];
					}

					shader->uniformMatrix4fv(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, cached_mvp.getF32ptr());
				}
			}
		}

		for (i = MM_TEXTURE0; i < NUM_MATRIX_MODES; ++i)
		{
			if (mMatHash[i] != shader->mMatHash[i])
			{
				shader->uniformMatrix4fv(name[i], 1, GL_FALSE, mMatrix[i][mMatIdx[i]].getF32ptr());
				shader->mMatHash[i] = mMatHash[i];
			}
		}


		if (shader->mFeatures.hasLighting || shader->mFeatures.calculatesLighting)
		{ //also sync light state
			syncLightState();
		}
	}
	else if (!LLGLSLShader::sNoFixedFunction)
	{
		GLenum mode[] = 
		{
			GL_MODELVIEW,
			GL_PROJECTION,
			GL_TEXTURE,
			GL_TEXTURE,
			GL_TEXTURE,
			GL_TEXTURE,
		};

		for (U32 i = 0; i < 2; ++i)
		{
			if (mMatHash[i] != mCurMatHash[i])
			{
				glMatrixMode(mode[i]);
				glLoadMatrixf(mMatrix[i][mMatIdx[i]].getF32ptr());
				mCurMatHash[i] = mMatHash[i];
			}
		}

		for (U32 i = 2; i < NUM_MATRIX_MODES; ++i)
		{
			if (mMatHash[i] != mCurMatHash[i])
			{
				gGL.getTexUnit(i-2)->activate();
				glMatrixMode(mode[i]);
				glLoadMatrixf(mMatrix[i][mMatIdx[i]].getF32ptr());
				mCurMatHash[i] = mMatHash[i];
			}
		}
	}

	stop_glerror();
}

LLMatrix4a LLRender::genRot(const GLfloat& a, const LLVector4a& axis) const
{
	F32 r = a * DEG_TO_RAD;

	F32 c = cosf(r);
	F32 s = sinf(r);

	F32 ic = 1.f-c;

	const LLVector4a add1(c,axis[VZ]*s,-axis[VY]*s);	//1,z,-y
	const LLVector4a add2(-axis[VZ]*s,c,axis[VX]*s);	//-z,1,x
	const LLVector4a add3(axis[VY]*s,-axis[VX]*s,c);	//y,-x,1

	LLVector4a axis_x;
	axis_x.splat<0>(axis);
	LLVector4a axis_y;
	axis_y.splat<1>(axis);
	LLVector4a axis_z;
	axis_z.splat<2>(axis);

	LLVector4a c_axis;
	c_axis.setMul(axis,ic);

	LLMatrix4a rot_mat;
	rot_mat.getRow<0>().setMul(c_axis,axis_x);
	rot_mat.getRow<0>().add(add1);
	rot_mat.getRow<1>().setMul(c_axis,axis_y);
	rot_mat.getRow<1>().add(add2);
	rot_mat.getRow<2>().setMul(c_axis,axis_z);
	rot_mat.getRow<2>().add(add3);
	rot_mat.setRow<3>(LLVector4a(0,0,0,1));

	return rot_mat;
}
LLMatrix4a LLRender::genOrtho(const GLfloat& left, const GLfloat& right, const GLfloat& bottom, const GLfloat&  top, const GLfloat& zNear, const GLfloat& zFar) const
{
	LLMatrix4a ortho_mat;
	ortho_mat.setRow<0>(LLVector4a(2.f/(right-left),0,0));
	ortho_mat.setRow<1>(LLVector4a(0,2.f/(top-bottom),0));
	ortho_mat.setRow<2>(LLVector4a(0,0,-2.f/(zFar-zNear)));
	ortho_mat.setRow<3>(LLVector4a(-(right+left)/(right-left),-(top+bottom)/(top-bottom),-(zFar+zNear)/(zFar-zNear),1));

	return ortho_mat;
}

LLMatrix4a LLRender::genPersp(const GLfloat& fovy, const GLfloat& aspect, const GLfloat& zNear, const GLfloat& zFar) const
{
	GLfloat f = 1.f/tanf(DEG_TO_RAD*fovy/2.f);

	LLMatrix4a persp_mat;
	persp_mat.setRow<0>(LLVector4a(f/aspect,0,0));
	persp_mat.setRow<1>(LLVector4a(0,f,0));
	persp_mat.setRow<2>(LLVector4a(0,0,(zFar+zNear)/(zNear-zFar),-1.f));
	persp_mat.setRow<3>(LLVector4a(0,0,(2.f*zFar*zNear)/(zNear-zFar),0));

	return persp_mat;
}

LLMatrix4a LLRender::genLook(const LLVector3& pos_in, const LLVector3& dir_in, const LLVector3& up_in) const
{
	const LLVector4a pos(pos_in.mV[VX],pos_in.mV[VY],pos_in.mV[VZ],1.f);
	LLVector4a dir(dir_in.mV[VX],dir_in.mV[VY],dir_in.mV[VZ]);
	const LLVector4a up(up_in.mV[VX],up_in.mV[VY],up_in.mV[VZ]);

	LLVector4a left_norm;
	left_norm.setCross3(dir,up);
	left_norm.normalize3fast();
	LLVector4a up_norm;
	up_norm.setCross3(left_norm,dir);
	up_norm.normalize3fast();
	LLVector4a& dir_norm = dir;
	dir.normalize3fast();
	
	LLVector4a left_dot;
	left_dot.setAllDot3(left_norm,pos);
	left_dot.negate();
	LLVector4a up_dot;
	up_dot.setAllDot3(up_norm,pos);
	up_dot.negate();
	LLVector4a dir_dot;
	dir_dot.setAllDot3(dir_norm,pos);

	dir_norm.negate();

	LLMatrix4a lookat_mat;
	lookat_mat.setRow<0>(left_norm);
	lookat_mat.setRow<1>(up_norm);
	lookat_mat.setRow<2>(dir_norm);
	lookat_mat.setRow<3>(LLVector4a(0,0,0,1));

	lookat_mat.getRow<0>().copyComponent<3>(left_dot);
	lookat_mat.getRow<1>().copyComponent<3>(up_dot);
	lookat_mat.getRow<2>().copyComponent<3>(dir_dot);

	lookat_mat.transpose();

	return lookat_mat;
}

const LLMatrix4a& LLRender::genNDCtoWC() const
{
	static LLMatrix4a mat(
		LLVector4a(.5f,0,0,0),
		LLVector4a(0,.5f,0,0),
		LLVector4a(0,0,.5f,0),
		LLVector4a(.5f,.5f,.5f,1.f));
	return mat;
}

void LLRender::translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	if(	llabs(x) < F_APPROXIMATELY_ZERO &&
		llabs(y) < F_APPROXIMATELY_ZERO &&
		llabs(z) < F_APPROXIMATELY_ZERO)
	{
		return;
	}

	flush();

	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].applyTranslation_affine(x,y,z);
	mMatHash[mMatrixMode]++;

}

void LLRender::scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	if(	(llabs(x-1.f)) < F_APPROXIMATELY_ZERO &&
		(llabs(y-1.f)) < F_APPROXIMATELY_ZERO &&
		(llabs(z-1.f)) < F_APPROXIMATELY_ZERO)
	{
		return;
	}
	flush();
	
	{
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].applyScale_affine(x,y,z);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar)
{
	flush();

	LLMatrix4a ortho_mat;
	ortho_mat.setRow<0>(LLVector4a(2.f/(right-left),0,0));
	ortho_mat.setRow<1>(LLVector4a(0,2.f/(top-bottom),0));
	ortho_mat.setRow<2>(LLVector4a(0,0,-2.f/(zFar-zNear)));
	ortho_mat.setRow<3>(LLVector4a(-(right+left)/(right-left),-(top+bottom)/(top-bottom),-(zFar+zNear)/(zFar-zNear),1));	

	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mul_affine(ortho_mat);
	mMatHash[mMatrixMode]++;
}

void LLRender::rotatef(const LLMatrix4a& rot)
{
	flush();

	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mul_affine(rot);
	mMatHash[mMatrixMode]++;
}

void LLRender::rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	if(	llabs(a) < F_APPROXIMATELY_ZERO ||
		llabs(a-360.f) < F_APPROXIMATELY_ZERO)
	{
		return;
	}
	
	flush();

	rotatef(genRot(a,x,y,z));
}

//LLRender::projectf & LLRender::unprojectf adapted from gluProject & gluUnproject in Mesa's GLU 9.0 library.
//  License/Copyright Statement:
/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

bool LLRender::projectf(const LLVector3& object, const LLMatrix4a& modelview, const LLMatrix4a& projection, const LLRect& viewport, LLVector3& windowCoordinate)
{
	//Begin SSE intrinsics

	// Declare locals
	const LLVector4a obj_vector(object.mV[VX],object.mV[VY],object.mV[VZ]);
	const LLVector4a one(1.f);
	LLVector4a temp_vec;								//Scratch vector
	LLVector4a w;										//Splatted W-component.
	
	modelview.affineTransform(obj_vector, temp_vec);	//temp_vec = modelview * obj_vector;

	//Passing temp_matrix as v and res is safe. res not altered until after all other calculations
	projection.rotate4(temp_vec, temp_vec);				//temp_vec = projection * temp_vec

	w.splat<3>(temp_vec);								//w = temp_vec.wwww

	//If w == 0.f, use 1.f instead.
	LLVector4a div;
	div.setSelectWithMask( w.equal( _mm_setzero_ps() ), one, w );	//float div = (w[N] == 0.f ? 1.f : w[N]);
	temp_vec.div(div);									//temp_vec /= div;

	//Map x, y to range 0-1
	temp_vec.mul(.5f);
	temp_vec.add(.5f);

	LLVector4Logical mask = temp_vec.equal(_mm_setzero_ps());
	if(mask.areAllSet(LLVector4Logical::MASK_W))
		return false;

	//End SSE intrinsics

	//Window coordinates
	windowCoordinate[0]=temp_vec[VX]*viewport.getWidth()+viewport.mLeft;
	windowCoordinate[1]=temp_vec[VY]*viewport.getHeight()+viewport.mBottom;
	//This is only correct when glDepthRange(0.0, 1.0)
	windowCoordinate[2]=temp_vec[VZ];	
	
	return true;
}

bool LLRender::unprojectf(const LLVector3& windowCoordinate, const LLMatrix4a& modelview, const LLMatrix4a& projection, const LLRect& viewport, LLVector3& object)
{
	//Begin SSE intrinsics

	// Declare locals
	static const LLVector4a one(1.f);
	static const LLVector4a two(2.f);
	LLVector4a norm_view(
		((windowCoordinate.mV[VX] - (F32)viewport.mLeft) / (F32)viewport.getWidth()),
		((windowCoordinate.mV[VY] - (F32)viewport.mBottom) / (F32)viewport.getHeight()),
		windowCoordinate.mV[VZ],
		1.f);
	
	LLMatrix4a inv_mat;								//Inverse transformation matrix
	LLVector4a temp_vec;							//Scratch vector
	LLVector4a w;									//Splatted W-component.

	inv_mat.setMul(projection,modelview);			//inv_mat = projection*modelview

	float det = inv_mat.invert();

	//Normalize. -1.0 : +1.0
	norm_view.mul(two);								// norm_view *= vec4(.2f)
	norm_view.sub(one);								// norm_view -= vec4(1.f)

	inv_mat.rotate4(norm_view,temp_vec);			//inv_mat * norm_view

	w.splat<3>(temp_vec);							//w = temp_vec.wwww

	//If w == 0.f, use 1.f instead. Defer return if temp_vec.w == 0.f until after all SSE intrinsics.
	LLVector4a div;
	div.setSelectWithMask( w.equal( _mm_setzero_ps() ), one, w );	//float div = (w[N] == 0.f ? 1.f : w[N]);
	temp_vec.div(div);								//temp_vec /= div;

	LLVector4Logical mask = temp_vec.equal(_mm_setzero_ps());
	if(mask.areAllSet(LLVector4Logical::MASK_W))
		return false;

	//End SSE intrinsics

	if(det == 0.f)	
       return false;

	object.set(temp_vec.getF32ptr());

	return true;
}

void LLRender::pushMatrix()
{
	flush();
	
	{
		if (mMatIdx[mMatrixMode] < LL_MATRIX_STACK_DEPTH-1)
		{
			mMatrix[mMatrixMode][mMatIdx[mMatrixMode]+1] = mMatrix[mMatrixMode][mMatIdx[mMatrixMode]];
			++mMatIdx[mMatrixMode];
		}
		else
		{
			llwarns << "Matrix stack overflow." << llendl;
		}
	}
}

void LLRender::popMatrix()
{
	flush();
	{
		if (mMatIdx[mMatrixMode] > 0)
		{
			--mMatIdx[mMatrixMode];
			mMatHash[mMatrixMode]++;
		}
		else
		{
			llwarns << "Matrix stack underflow." << llendl;
		}
	}
}

void LLRender::loadMatrix(const LLMatrix4a& mat)
{
	flush();
	
	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = mat;
	mMatHash[mMatrixMode]++;
}

void LLRender::multMatrix(const LLMatrix4a& mat)
{
	flush();

	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mul_affine(mat); 
	mMatHash[mMatrixMode]++;
	
}

void LLRender::matrixMode(U32 mode)
{
	if (mode == MM_TEXTURE)
	{
		mode = MM_TEXTURE0 + gGL.getCurrentTexUnitIndex();
	}

	llassert(mode < NUM_MATRIX_MODES);
	mMatrixMode = mode;
}

U32 LLRender::getMatrixMode()
{
	if (mMatrixMode >= MM_TEXTURE0 && mMatrixMode <= MM_TEXTURE3)
	{ //always return MM_TEXTURE if current matrix mode points at any texture matrix
		return MM_TEXTURE;
	}

	return mMatrixMode;
}


void LLRender::loadIdentity()
{
	flush();

	mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].setIdentity();
	mMatHash[mMatrixMode]++;
}

const LLMatrix4a& LLRender::getModelviewMatrix()
{
	return mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
}

const LLMatrix4a& LLRender::getProjectionMatrix()
{
	return mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];
}

void LLRender::translateUI(F32 x, F32 y, F32 z)
{
	if (mUIOffset.empty())
	{
		llerrs << "Need to push a UI translation frame before offsetting" << llendl;
	}

	LLVector4a add(x,y,z);
	mUIOffset.back().add(add);
}

void LLRender::scaleUI(F32 x, F32 y, F32 z)
{
	if (mUIScale.empty())
	{
		llerrs << "Need to push a UI transformation frame before scaling." << llendl;
	}

	LLVector4a scale(x,y,z);
	mUIScale.back().mul(scale);
}

void LLRender::pushUIMatrix()
{
	if (mUIOffset.empty())
	{
		mUIOffset.push_back(LLVector4a(0.f));
	}
	else
	{
		mUIOffset.push_back(mUIOffset.back());
	}
	
	if (mUIScale.empty())
	{
		mUIScale.push_back(LLVector4a(1.f));
	}
	else
	{
		mUIScale.push_back(mUIScale.back());
	}
}

void LLRender::popUIMatrix()
{
	if (mUIOffset.empty())
	{
		llerrs << "UI offset stack blown." << llendl;
	}
	mUIOffset.pop_back();
	mUIScale.pop_back();
}

LLVector3 LLRender::getUITranslation()
{
	if (mUIOffset.empty())
	{
		return LLVector3(0,0,0);
	}
	return LLVector3(mUIOffset.back().getF32ptr());
}

LLVector3 LLRender::getUIScale()
{
	if (mUIScale.empty())
	{
		return LLVector3(1,1,1);
	}
	return LLVector3(mUIScale.back().getF32ptr());
}


void LLRender::loadUIIdentity()
{
	if (mUIOffset.empty() || mUIScale.empty())
	{
		llerrs << "Need to push UI translation frame before clearing offset." << llendl;
	}
	mUIOffset.back().splat(0.f);
	mUIScale.back().splat(1.f);
}

void LLRender::setColorMask(bool writeColor, bool writeAlpha)
{
	setColorMask(writeColor, writeColor, writeColor, writeAlpha);
}

void LLRender::setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha)
{
	flush();

	if (mCurrColorMask[0] != writeColorR ||
		mCurrColorMask[1] != writeColorG ||
		mCurrColorMask[2] != writeColorB ||
		mCurrColorMask[3] != writeAlpha || mDirty)
	{
		mCurrColorMask[0] = writeColorR;
		mCurrColorMask[1] = writeColorG;
		mCurrColorMask[2] = writeColorB;
		mCurrColorMask[3] = writeAlpha;

		glColorMask(writeColorR ? GL_TRUE : GL_FALSE, 
					writeColorG ? GL_TRUE : GL_FALSE,
					writeColorB ? GL_TRUE : GL_FALSE,
					writeAlpha ? GL_TRUE : GL_FALSE);
	}
}

void LLRender::setSceneBlendType(eBlendType type)
{
	switch (type) 
	{
		case BT_ALPHA:
			blendFunc(BF_SOURCE_ALPHA, BF_ONE_MINUS_SOURCE_ALPHA);
			break;
		case BT_ADD:
			blendFunc(BF_ONE, BF_ONE);
			break;
		case BT_ADD_WITH_ALPHA:
			blendFunc(BF_SOURCE_ALPHA, BF_ONE);
			break;
		case BT_MULT:
			blendFunc(BF_DEST_COLOR, BF_ZERO);
			break;
		case BT_MULT_ALPHA:
			blendFunc(BF_DEST_ALPHA, BF_ZERO);
			break;
		case BT_MULT_X2:
			blendFunc(BF_DEST_COLOR, BF_SOURCE_COLOR);
			break;
		case BT_REPLACE:
			blendFunc(BF_ONE, BF_ZERO);
			break;
		default:
			llerrs << "Unknown Scene Blend Type: " << type << llendl;
			break;
	}
}

void LLRender::setAlphaRejectSettings(eCompareFunc func, F32 value)
{
	flush();

	if (LLGLSLShader::sNoFixedFunction)
	{ //glAlphaFunc is deprecated in OpenGL 3.3
		return;
	}

	if (mCurrAlphaFunc != func ||
		mCurrAlphaFuncVal != value || mDirty)
	{
		mCurrAlphaFunc = func;
		mCurrAlphaFuncVal = value;
		if (func == CF_DEFAULT)
		{
			glAlphaFunc(GL_GREATER, 0.01f);
		} 
		else
		{
			glAlphaFunc(sGLCompareFunc[func], value);
		}
	}

	if (gDebugGL)
	{ //make sure cached state is correct
		GLint cur_func = 0;
		glGetIntegerv(GL_ALPHA_TEST_FUNC, &cur_func);

		if (func == CF_DEFAULT)
		{
			func = CF_GREATER;
		}

		if (cur_func != sGLCompareFunc[func])
		{
			llerrs << "Alpha test function corrupted!" << llendl;
		}

		F32 ref = 0.f;
		glGetFloatv(GL_ALPHA_TEST_REF, &ref);

		if (ref != value)
		{
			llerrs << "Alpha test value corrupted!" << llendl;
		}
	}
}

void check_blend_funcs()
{
	llassert_always(blendfunc_debug[0] == LLRender::BF_SOURCE_ALPHA );
	llassert_always(blendfunc_debug[1] == LLRender::BF_SOURCE_ALPHA );
	llassert_always(blendfunc_debug[2] == LLRender::BF_ONE_MINUS_SOURCE_ALPHA );
	llassert_always(blendfunc_debug[3] == LLRender::BF_ONE_MINUS_SOURCE_ALPHA );
}

void LLRender::blendFunc(eBlendFactor sfactor, eBlendFactor dfactor)
{
	llassert(sfactor < BF_UNDEF);
	llassert(dfactor < BF_UNDEF);
	if (mCurrBlendColorSFactor != sfactor || mCurrBlendColorDFactor != dfactor ||
	    mCurrBlendAlphaSFactor != sfactor || mCurrBlendAlphaDFactor != dfactor || mDirty)
	{
		mCurrBlendColorSFactor = sfactor;
		mCurrBlendAlphaSFactor = sfactor;
		mCurrBlendColorDFactor = dfactor;
		mCurrBlendAlphaDFactor = dfactor;
		blendfunc_debug[0]=blendfunc_debug[1]=sfactor;
		blendfunc_debug[2]=blendfunc_debug[3]=dfactor;
		flush();
		glBlendFunc(sGLBlendFactor[sfactor], sGLBlendFactor[dfactor]);
	}
}

void LLRender::blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
			 eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor)
{
	llassert(color_sfactor < BF_UNDEF);
	llassert(color_dfactor < BF_UNDEF);
	llassert(alpha_sfactor < BF_UNDEF);
	llassert(alpha_dfactor < BF_UNDEF);
	if (!gGLManager.mHasBlendFuncSeparate)
	{
		LL_WARNS_ONCE("render") << "no glBlendFuncSeparateEXT(), using color-only blend func" << llendl;
		blendFunc(color_sfactor, color_dfactor);
		return;
	}
	if (mCurrBlendColorSFactor != color_sfactor || mCurrBlendColorDFactor != color_dfactor ||
	    mCurrBlendAlphaSFactor != alpha_sfactor || mCurrBlendAlphaDFactor != alpha_dfactor || mDirty)
	{
		mCurrBlendColorSFactor = color_sfactor;
		mCurrBlendAlphaSFactor = alpha_sfactor;
		mCurrBlendColorDFactor = color_dfactor;
		mCurrBlendAlphaDFactor = alpha_dfactor;
		blendfunc_debug[0]=color_sfactor;
		blendfunc_debug[1]=alpha_sfactor;
		blendfunc_debug[2]=color_dfactor;
		blendfunc_debug[3]=alpha_dfactor;
		flush();
		glBlendFuncSeparateEXT(sGLBlendFactor[color_sfactor], sGLBlendFactor[color_dfactor],
				       sGLBlendFactor[alpha_sfactor], sGLBlendFactor[alpha_dfactor]);
	}
}

LLTexUnit* LLRender::getTexUnit(U32 index)
{
	if (index < mTexUnits.size())
	{
		return mTexUnits[index];
	}
	else 
	{
		lldebugs << "Non-existing texture unit layer requested: " << index << llendl;
		return mDummyTexUnit;
	}
}

LLLightState* LLRender::getLight(U32 index)
{
	if (index < mLightState.size())
	{
		return mLightState[index];
	}
	
	return NULL;
}

void LLRender::setAmbientLightColor(const LLColor4& color)
{
	if (color != mAmbientLightColor)
	{
		++mLightHash;
		mAmbientLightColor = color;
		if (!LLGLSLShader::sNoFixedFunction)
		{
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color.mV);
		}
	}
}

bool LLRender::verifyTexUnitActive(U32 unitToVerify)
{
	if (mCurrTextureUnitIndex == unitToVerify)
	{
		return true;
	}
	else 
	{
		llwarns << "TexUnit currently active: " << mCurrTextureUnitIndex << " (expecting " << unitToVerify << ")" << llendl;
		return false;
	}
}

void LLRender::clearErrors()
{
	while (glGetError())
	{
		//loop until no more error flags left
	}
}

void LLRender::begin(const GLuint& mode)
{
	if (mode != mMode)
	{
		if (mode == LLRender::QUADS)
		{
			mQuadCycle = 1;
		}

		if (mMode == LLRender::QUADS ||
			mMode == LLRender::LINES ||
			mMode == LLRender::TRIANGLES ||
			mMode == LLRender::POINTS)
		{
			flush();
		}
		else if (mCount != 0)
		{
			llerrs << "gGL.begin() called redundantly." << llendl;
		}
		
		mMode = mode;
	}
}

void LLRender::end()
{ 
	if (mCount == 0)
	{
		return;
		//IMM_ERRS << "GL begin and end called with no vertices specified." << llendl;
	}

	if ((mMode != LLRender::QUADS && 
		mMode != LLRender::LINES &&
		mMode != LLRender::TRIANGLES &&
		mMode != LLRender::POINTS) ||
		mCount > 2048)
	{
		flush();
	}
}
void LLRender::flush()
{
	if (mCount > 0)
	{
#if 0
		if (!glIsEnabled(GL_VERTEX_ARRAY))
		{
			llerrs << "foo 1" << llendl;
		}

		if (!glIsEnabled(GL_COLOR_ARRAY))
		{
			llerrs << "foo 2" << llendl;
		}

		if (!glIsEnabled(GL_TEXTURE_COORD_ARRAY))
		{
			llerrs << "foo 3" << llendl;
		}

		if (glIsEnabled(GL_NORMAL_ARRAY))
		{
			llerrs << "foo 7" << llendl;
		}

		GLvoid* pointer;

		glGetPointerv(GL_VERTEX_ARRAY_POINTER, &pointer);
		if (pointer != &(mBuffer[0].v))
		{
			llerrs << "foo 4" << llendl;
		}

		glGetPointerv(GL_COLOR_ARRAY_POINTER, &pointer);
		if (pointer != &(mBuffer[0].c))
		{
			llerrs << "foo 5" << llendl;
		}

		glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &pointer);
		if (pointer != &(mBuffer[0].uv))
		{
			llerrs << "foo 6" << llendl;
		}
#endif
				
		if (!mUIOffset.empty())
		{
			sUICalls++;
			sUIVerts += mCount;
		}
		
		if (gDebugGL)
		{
			if (mMode == LLRender::QUADS && !sGLCoreProfile)
			{
				if (mCount%4 != 0)
				{
					llerrs << "Incomplete quad rendered." << llendl;
				}
			}
			
			if (mMode == LLRender::TRIANGLES)
			{
				if (mCount%3 != 0)
				{
					llerrs << "Incomplete triangle rendered." << llendl;
				}
			}
			
			if (mMode == LLRender::LINES)
			{
				if (mCount%2 != 0)
				{
					llerrs << "Incomplete line rendered." << llendl;
				}
			}
		}

		//store mCount in a local variable to avoid re-entrance (drawArrays may call flush)
		U32 count = mCount;
		mCount = 0;

		if (mBuffer->useVBOs() && !mBuffer->isLocked())
		{ //hack to only flush the part of the buffer that was updated (relies on stream draw using buffersubdata)
			mBuffer->getVertexStrider(mVerticesp, 0, count);
			mBuffer->getTexCoord0Strider(mTexcoordsp, 0, count);
			mBuffer->getColorStrider(mColorsp, 0, count);
		}
		
		mBuffer->flush();
		mBuffer->setBuffer(immediate_mask);

		if (mMode == LLRender::QUADS && sGLCoreProfile)
		{
			mBuffer->drawArrays(LLRender::TRIANGLES, 0, count);
			mQuadCycle = 1;
		}
		else
		{
			mBuffer->drawArrays(mMode, 0, count);
		}
		
		mVerticesp[0] = mVerticesp[count];
		mTexcoordsp[0] = mTexcoordsp[count];
		mColorsp[0] = mColorsp[count];
		
		mCount = 0;
	}
}

void LLRender::vertex4a(const LLVector4a& vertex)
{ 
	//the range of mVerticesp, mColorsp and mTexcoordsp is [0, 4095]
	if (mCount > 2048)
	{ //break when buffer gets reasonably full to keep GL command buffers happy and avoid overflow below
		switch (mMode)
		{
			case LLRender::POINTS: flush(); break;
			case LLRender::TRIANGLES: if (mCount%3==0) flush(); break;
			case LLRender::QUADS: if(mCount%4 == 0) flush(); break; 
			case LLRender::LINES: if (mCount%2 == 0) flush(); break;
		}
	}
			
	if (mCount > 4094)
	{
	//	llwarns << "GL immediate mode overflow.  Some geometry not drawn." << llendl;
		return;
	}

	if (mUIOffset.empty())
	{
		mVerticesp[mCount]=vertex;
	}
	else
	{
		//LLVector3 vert = (LLVector3(x,y,z)+mUIOffset.back()).scaledVec(mUIScale.back());
		mVerticesp[mCount].setAdd(vertex,mUIOffset.back());
		mVerticesp[mCount].mul(mUIScale.back());
	}

	if (mMode == LLRender::QUADS && LLRender::sGLCoreProfile)
	{
		mQuadCycle++;
		if (mQuadCycle == 4)
		{ //copy two vertices so fourth quad element will add a triangle
			mQuadCycle = 0;
	
			mCount++;
			mVerticesp[mCount] = mVerticesp[mCount-3];
			mColorsp[mCount] = mColorsp[mCount-3];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-3];

			mCount++;
			mVerticesp[mCount] = mVerticesp[mCount-2];
			mColorsp[mCount] = mColorsp[mCount-2];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-2];
		}
	}

	mCount++;
	mVerticesp[mCount] = mVerticesp[mCount-1];
	mColorsp[mCount] = mColorsp[mCount-1];
	mTexcoordsp[mCount] = mTexcoordsp[mCount-1];	
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	llwarns << "GL immediate mode overflow.  Some geometry not drawn." << llendl;
		return;
	}

	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;
		
		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy two
			mVerticesp[mCount++] = verts[i-3];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i-1];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
			
			//copy last one
			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];

			mCount++;
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}

	mVerticesp[mCount] = mVerticesp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, LLVector2* uvs, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	llwarns << "GL immediate mode overflow.  Some geometry not drawn." << llendl;
		return;
	}

	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;

		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy last two
			mVerticesp[mCount] = verts[i-3];
			mTexcoordsp[mCount++] = uvs[i-3];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i-1];
			mTexcoordsp[mCount++] = uvs[i-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy last one
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];

			mCount++;
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}

	mVerticesp[mCount] = mVerticesp[mCount-1];
	mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, LLVector2* uvs, LLColor4U* colors, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	llwarns << "GL immediate mode overflow.  Some geometry not drawn." << llendl;
		return;
	}

	
	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;

		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			//copy last two
			mVerticesp[mCount] = verts[i-3];
			mTexcoordsp[mCount] = uvs[i-3];
			mColorsp[mCount++] = colors[i-3];

			mVerticesp[mCount] = verts[i-1];
			mTexcoordsp[mCount] = uvs[i-1];
			mColorsp[mCount++] = colors[i-1];

			//copy last one
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount] = colors[i];

			mCount++;
		}
	}

	mVerticesp[mCount] = mVerticesp[mCount-1];
	mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
	mColorsp[mCount] = mColorsp[mCount-1];
}

void LLRender::texCoord2f(const GLfloat& x, const GLfloat& y)
{ 
	mTexcoordsp[mCount] = LLVector2(x,y);
}

void LLRender::texCoord2i(const GLint& x, const GLint& y)
{ 
	texCoord2f((GLfloat) x, (GLfloat) y);
}

void LLRender::texCoord2fv(const GLfloat* tc)
{ 
	texCoord2f(tc[0], tc[1]);
}

void LLRender::color4ub(const GLubyte& r, const GLubyte& g, const GLubyte& b, const GLubyte& a)
{
	if (!LLGLSLShader::sCurBoundShaderPtr ||
		LLGLSLShader::sCurBoundShaderPtr->mAttributeMask & LLVertexBuffer::MAP_COLOR)
	{
		mColorsp[mCount] = LLColor4U(r,g,b,a);
	}
	else
	{ //not using shaders or shader reads color from a uniform
		diffuseColor4ub(r,g,b,a);
	}
}
void LLRender::color4ubv(const GLubyte* c)
{
	color4ub(c[0], c[1], c[2], c[3]);
}

void LLRender::color4f(const GLfloat& r, const GLfloat& g, const GLfloat& b, const GLfloat& a)
{
	color4ub((GLubyte) (llclamp(r, 0.f, 1.f)*255),
		(GLubyte) (llclamp(g, 0.f, 1.f)*255),
		(GLubyte) (llclamp(b, 0.f, 1.f)*255),
		(GLubyte) (llclamp(a, 0.f, 1.f)*255));
}

void LLRender::color4fv(const GLfloat* c)
{ 
	color4f(c[0],c[1],c[2],c[3]);
}

void LLRender::color3f(const GLfloat& r, const GLfloat& g, const GLfloat& b)
{ 
	color4f(r,g,b,1);
}

void LLRender::color3fv(const GLfloat* c)
{ 
	color4f(c[0],c[1],c[2],1);
}

void LLRender::diffuseColor3f(F32 r, F32 g, F32 b)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r,g,b,1.f);
	}
	else
	{
		glColor3f(r,g,b);
	}
}

void LLRender::diffuseColor3fv(const F32* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, c[0], c[1], c[2], 1.f);
	}
	else
	{
		glColor3fv(c);
	}
}

void LLRender::diffuseColor4f(F32 r, F32 g, F32 b, F32 a)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r,g,b,a);
	}
	else
	{
		glColor4f(r,g,b,a);
	}
}

void LLRender::diffuseColor4fv(const F32* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4fv(LLShaderMgr::DIFFUSE_COLOR, 1, c);
	}
	else
	{
		glColor4fv(c);
	}
}

void LLRender::diffuseColor4ubv(const U8* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, c[0]/255.f, c[1]/255.f, c[2]/255.f, c[3]/255.f);
	}
	else
	{
		glColor4ubv(c);
	}
}

void LLRender::diffuseColor4ub(U8 r, U8 g, U8 b, U8 a)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(!LLGLSLShader::sNoFixedFunction || shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r/255.f, g/255.f, b/255.f, a/255.f);
	}
	else
	{
		glColor4ub(r,g,b,a);
	}
}


void LLRender::debugTexUnits(void)
{
	LL_INFOS("TextureUnit") << "Active TexUnit: " << mCurrTextureUnitIndex << LL_ENDL;
	std::string active_enabled = "false";
	for (U32 i = 0; i < mTexUnits.size(); i++)
	{
		if (getTexUnit(i)->mCurrTexType != LLTexUnit::TT_NONE)
		{
			if (i == mCurrTextureUnitIndex) active_enabled = "true";
			LL_INFOS("TextureUnit") << "TexUnit: " << i << " Enabled" << LL_ENDL;
			LL_INFOS("TextureUnit") << "Enabled As: " ;
			switch (getTexUnit(i)->mCurrTexType)
			{
				case LLTexUnit::TT_TEXTURE:
					LL_CONT << "Texture 2D";
					break;
				case LLTexUnit::TT_RECT_TEXTURE:
					LL_CONT << "Texture Rectangle";
					break;
				case LLTexUnit::TT_CUBE_MAP:
					LL_CONT << "Cube Map";
					break;
				default:
					LL_CONT << "ARGH!!! NONE!";
					break;
			}
			LL_CONT << ", Texture Bound: " << getTexUnit(i)->mCurrTexture << LL_ENDL;
		}
	}
	LL_INFOS("TextureUnit") << "Active TexUnit Enabled : " << active_enabled << LL_ENDL;
}

