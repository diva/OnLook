/** 
 * @File llavatarappearance.cpp
 * @brief Implementation of LLAvatarAppearance class
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

#if LL_MSVC
// disable warning about boost::lexical_cast returning uninitialized data
// when it fails to parse the string
#pragma warning (disable:4701)
#endif

#include "linden_common.h"

#include "llavatarappearance.h"
#include "llavatarappearancedefines.h"
#include "llavatarjointmesh.h"
#include "imageids.h"
#include "lldir.h"
#include "lldeleteutils.h"
#include "llpolymorph.h"
#include "llpolymesh.h"
#include "llpolyskeletaldistortion.h"
#include "llstl.h"
#include "lltexglobalcolor.h"
#include "llwearabledata.h"


#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

#include <boost/lexical_cast.hpp>

using namespace LLAvatarAppearanceDefines;
const LLColor4 DUMMY_COLOR = LLColor4(0.5,0.5,0.5,1.0);
//static
BOOL LLAvatarAppearance::teToColorParams( ETextureIndex te, U32 *param_name )
{
	switch( te )
	{
		case TEX_UPPER_SHIRT:
			param_name[0] = 803; //"shirt_red";
			param_name[1] = 804; //"shirt_green";
			param_name[2] = 805; //"shirt_blue";
			break;

		case TEX_LOWER_PANTS:
			param_name[0] = 806; //"pants_red";
			param_name[1] = 807; //"pants_green";
			param_name[2] = 808; //"pants_blue";
			break;

		case TEX_LOWER_SHOES:
			param_name[0] = 812; //"shoes_red";
			param_name[1] = 813; //"shoes_green";
			param_name[2] = 817; //"shoes_blue";
			break;

		case TEX_LOWER_SOCKS:
			param_name[0] = 818; //"socks_red";
			param_name[1] = 819; //"socks_green";
			param_name[2] = 820; //"socks_blue";
			break;

		case TEX_UPPER_JACKET:
		case TEX_LOWER_JACKET:
			param_name[0] = 834; //"jacket_red";
			param_name[1] = 835; //"jacket_green";
			param_name[2] = 836; //"jacket_blue";
			break;

		case TEX_UPPER_GLOVES:
			param_name[0] = 827; //"gloves_red";
			param_name[1] = 829; //"gloves_green";
			param_name[2] = 830; //"gloves_blue";
			break;

		case TEX_UPPER_UNDERSHIRT:
			param_name[0] = 821; //"undershirt_red";
			param_name[1] = 822; //"undershirt_green";
			param_name[2] = 823; //"undershirt_blue";
			break;
	
		case TEX_LOWER_UNDERPANTS:
			param_name[0] = 824; //"underpants_red";
			param_name[1] = 825; //"underpants_green";
			param_name[2] = 826; //"underpants_blue";
			break;

		case TEX_SKIRT:
			param_name[0] = 921; //"skirt_red";
			param_name[1] = 922; //"skirt_green";
			param_name[2] = 923; //"skirt_blue";
			break;

		case TEX_HEAD_TATTOO:
		case TEX_LOWER_TATTOO:
		case TEX_UPPER_TATTOO:
			param_name[0] = 1071; //"tattoo_red";
			param_name[1] = 1072; //"tattoo_green";
			param_name[2] = 1073; //"tattoo_blue";
			break;	

		default:
			llassert(0);
			return FALSE;
	}

	return TRUE;
}
// static
LLColor4 LLAvatarAppearance::getDummyColor()
{
	return DUMMY_COLOR;
}