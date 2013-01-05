/** 
 * @file llwearable.h
 * @brief LLWearable class header file
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

#ifndef LL_LLWEARABLE_H
#define LL_LLWEARABLE_H

#include "llavatarappearancedefines.h"
#include "llextendedstatus.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llwearabletype.h"
#include "lllocaltextureobject.h"

class LLMD5;
class LLVisualParam;
class LLTexGlobalColorInfo;
class LLTexGlobalColor;
class LLAvatarAppearance;

// Abstract class.
class LLWearable
{
	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
public:
	virtual ~LLWearable() {};

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	virtual LLWearableType::EType		getType() const = 0;
	virtual void	writeToAvatar(LLAvatarAppearance* avatarp) = 0;
	virtual LLLocalTextureObject* getLocalTextureObject(S32 index) = 0;
	virtual const LLLocalTextureObject* getLocalTextureObject(S32 index) const = 0;
	virtual void 						setVisualParamWeight(S32 index, F32 value, BOOL upload_bake) = 0;
	virtual F32							getVisualParamWeight(S32 index) const = 0;
	virtual LLVisualParam*				getVisualParam(S32 index) const = 0;

	// Something happened that requires the wearable to be updated (e.g. worn/unworn).
	virtual void		setUpdated() const = 0;

	// Update the baked texture hash.
	virtual void		addToBakedTextureHash(LLMD5& hash) const = 0;
};

#endif  // LL_LLWEARABLE_H
