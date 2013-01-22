/** 
 * @file llerrorlegacy.h
 * @date   January 2007
 * @brief old things from the older error system
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLERRORLEGACY_H
#define LL_LLERRORLEGACY_H

#include "llpreprocessor.h"

/*
	LEGACY -- DO NOT USE THIS STUFF ANYMORE
*/

// Specific error codes
const int LL_ERR_NOERR = 0;
const int LL_ERR_ASSET_REQUEST_FAILED = -1;
//const int LL_ERR_ASSET_REQUEST_INVALID = -2;
const int LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE = -3;
const int LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE = -4;
const int LL_ERR_INSUFFICIENT_PERMISSIONS = -5;
const int LL_ERR_EOF = -39;
const int LL_ERR_CANNOT_OPEN_FILE = -42;
const int LL_ERR_FILE_NOT_FOUND = -43;
const int LL_ERR_FILE_EMPTY     = -44;
const int LL_ERR_TCP_TIMEOUT    = -23016;
const int LL_ERR_CIRCUIT_GONE   = -23017;
const int LL_ERR_PRICE_MISMATCH = -23018;



// Define one of these for different error levels in release...
// #define RELEASE_SHOW_DEBUG // Define this if you want your release builds to show lldebug output.
#define RELEASE_SHOW_INFO // Define this if you want your release builds to show llinfo output
#define RELEASE_SHOW_WARN // Define this if you want your release builds to show llwarn output.


//////////////////////////////////////////
//
//  Implementation - ignore
//
//
#ifdef _DEBUG
#define SHOW_DEBUG
#define SHOW_WARN
#define SHOW_INFO
#define SHOW_ASSERT
#else // _DEBUG

#ifdef LL_RELEASE_WITH_DEBUG_INFO
#define SHOW_ASSERT
#endif // LL_RELEASE_WITH_DEBUG_INFO

#ifdef RELEASE_SHOW_DEBUG
#define SHOW_DEBUG
#endif

#ifdef RELEASE_SHOW_WARN
#define SHOW_WARN
#endif

#ifdef RELEASE_SHOW_INFO
#define SHOW_INFO
#endif

#ifdef RELEASE_SHOW_ASSERT
#define SHOW_ASSERT
#endif

#endif // _DEBUG



#define lldebugst(type)			lldebugs
#define llendflush				llendl


#define llerror(msg, num)		llerrs << "Error # " << num << ": " << msg << llendl;

#define llwarning(msg, num)		llwarns << "Warning # " << num << ": " << msg << llendl;

#define liru_slashpos			std::string(__FILE__).find_last_of("/\\")
#define liru_slashpos2			std::string(__FILE__).substr(0,liru_slashpos).find_last_of("/\\")
#define liru_assert_strip		/*strip path down to lastlevel directory and filename for assert.*/\
	(liru_slashpos == std::string::npos ? std::string(__FILE__)/*just filename, print as is*/\
		: liru_slashpos2 == std::string::npos ? std::string(__FILE__)/*Apparently, we're in / or perhaps the top of the drive, print as is*/\
			: std::string(__FILE__).substr(1+liru_slashpos2))/*print foo/bar.cpp or perhaps foo\bar.cpp*/

#define llassert_always(func)	do { if (LL_UNLIKELY(!(func))) llerrs << "\nASSERT(" #func ")\nfile:" << liru_assert_strip << " line:" << std::dec << __LINE__ << llendl; } while(0)

#ifdef SHOW_ASSERT
#define llassert(func)			llassert_always(func)
#define llverify(func)			llassert_always(func)
#else
#define llassert(func)
#define llverify(func)			do {if (func) {}} while(0)
#endif

// This can be used for function parameters that are only used by llassert.
// The ellipsis is needed in case the parameter contains comma's (ie, as part of the type,
// or trailing comma). The first version can be used as first (or only) parameter of a function,
// or as parameter in the middle when adding a trailing comma, while the second version can be
// used as last parameter.
//
// Example usage:
//
// void foo(ASSERT_ONLY(int x));
// void foo(x, ASSERT_ONLY(int y,) int z);
// void foo(x/*,*/ ASSERT_ONLY_COMMA(int y));	// The optional /*,*/ makes it just a bit better readable.
#ifdef SHOW_ASSERT
#define ASSERT_ONLY(...)		__VA_ARGS__
#define ASSERT_ONLY_COMMA(...)	, __VA_ARGS__
#else
#define ASSERT_ONLY(...)
#define ASSERT_ONLY_COMMA(...)
#endif

// handy compile-time assert - enforce those template parameters! 
#define cassert(expn) typedef char __C_ASSERT__[(expn)?1:-1]   /* Flawfinder: ignore */
	//XXX: used in two places in llcommon/llskipmap.h

#endif // LL_LLERRORLEGACY_H
