/** 
 * @file lltexlayer.cpp
 * @brief A texture layer. Used for avatars.
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

#include "linden_common.h"

#include "lltexlayer.h"

#include "llavatarappearance.h"
#include "llcrc.h"
#include "imageids.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "lldir.h"
#include "llvfile.h"
#include "llvfs.h"
#include "lltexlayerparams.h"
#include "lltexturemanagerbridge.h"
//#include "llrender2dutils.h"
//#include "llwearable.h"
//#include "llwearabledata.h"
#include "llvertexbuffer.h"
#include "llviewervisualparam.h"

//#include "../tools/imdebug/imdebug.h"

using namespace LLAvatarAppearanceDefines;


//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::sHasCaches = FALSE;

LLTexLayerSet::LLTexLayerSet(LLAvatarAppearance* const appearance) :
	mAvatarAppearance( appearance )
{
}
//-----------------------------------------------------------------------------
// LLTexLayerStaticImageList
//-----------------------------------------------------------------------------

LLTexLayerStaticImageList::LLTexLayerStaticImageList() :
	mGLBytes(0),
	mTGABytes(0),
	mImageNames(16384)
{
}

LLTexLayerStaticImageList::~LLTexLayerStaticImageList()
{
	deleteCachedImages();
}

void LLTexLayerStaticImageList::dumpByteCount() const
{
	llinfos << "Avatar Static Textures " <<
		"KB GL:" << (mGLBytes / 1024) <<
		"KB TGA:" << (mTGABytes / 1024) << "KB" << llendl;
}

void LLTexLayerStaticImageList::deleteCachedImages()
{
	if( mGLBytes || mTGABytes )
	{
		llinfos << "Clearing Static Textures " <<
			"KB GL:" << (mGLBytes / 1024) <<
			"KB TGA:" << (mTGABytes / 1024) << "KB" << llendl;

		//mStaticImageLists uses LLPointers, clear() will cause deletion
		
		mStaticImageListTGA.clear();
		mStaticImageList.clear();
		
		mGLBytes = 0;
		mTGABytes = 0;
	}
}

// Note: in general, for a given image image we'll call either getImageTga() or getTexture().
// We call getImageTga() if the image is used as an alpha gradient.
// Otherwise, we call getTexture()

// Returns an LLImageTGA that contains the encoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
static LLFastTimer::DeclareTimer FTM_LOAD_STATIC_TGA("getImageTGA");
LLImageTGA* LLTexLayerStaticImageList::getImageTGA(const std::string& file_name)
{
	LLFastTimer t(FTM_LOAD_STATIC_TGA);
	const char *namekey = mImageNames.addString(file_name);
	image_tga_map_t::const_iterator iter = mStaticImageListTGA.find(namekey);
	if( iter != mStaticImageListTGA.end() )
	{
		return iter->second;
	}
	else
	{
		std::string path;
		path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
		LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
		if( image_tga->getDataSize() > 0 )
		{
			mStaticImageListTGA[ namekey ] = image_tga;
			mTGABytes += image_tga->getDataSize();
			return image_tga;
		}
		else
		{
			return NULL;
		}
	}
}

// Returns a GL Image (without a backing ImageRaw) that contains the decoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
static LLFastTimer::DeclareTimer FTM_LOAD_STATIC_TEXTURE("getTexture");
LLGLTexture* LLTexLayerStaticImageList::getTexture(const std::string& file_name, BOOL is_mask)
{
	LLFastTimer t(FTM_LOAD_STATIC_TEXTURE);
	LLPointer<LLGLTexture> tex;
	const char *namekey = mImageNames.addString(file_name);

	texture_map_t::const_iterator iter = mStaticImageList.find(namekey);
	if( iter != mStaticImageList.end() )
	{
		tex = iter->second;
	}
	else
	{
		llassert(gTextureManagerBridgep);
		tex = gTextureManagerBridgep->getLocalTexture( FALSE );
		LLPointer<LLImageRaw> image_raw = new LLImageRaw;
		if( loadImageRaw( file_name, image_raw ) )
		{
			if( (image_raw->getComponents() == 1) && is_mask )
			{
				// Note: these are static, unchanging images so it's ok to assume
				// that once an image is a mask it's always a mask.
				tex->setExplicitFormat( GL_ALPHA8, GL_ALPHA );
			}
			tex->createGLTexture(0, image_raw, 0, TRUE, LLGLTexture::LOCAL);

			gGL.getTexUnit(0)->bind(tex);
			tex->setAddressMode(LLTexUnit::TAM_CLAMP);

			mStaticImageList [ namekey ] = tex;
			mGLBytes += (S32)tex->getWidth() * tex->getHeight() * tex->getComponents();
		}
		else
		{
			tex = NULL;
		}
	}

	return tex;
}

// Reads a .tga file, decodes it, and puts the decoded data in image_raw.
// Returns TRUE if successful.
static LLFastTimer::DeclareTimer FTM_LOAD_IMAGE_RAW("loadImageRaw");
BOOL LLTexLayerStaticImageList::loadImageRaw(const std::string& file_name, LLImageRaw* image_raw)
{
	LLFastTimer t(FTM_LOAD_IMAGE_RAW);
	BOOL success = FALSE;
	std::string path;
	path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
	LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
	if( image_tga->getDataSize() > 0 )
	{
		// Copy data from tga to raw.
		success = image_tga->decode( image_raw );
	}

	return success;
}

