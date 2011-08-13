/** 
 * @file llframetimer.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * Copyright (c) 2011, Aleric Inglewood.
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

#include "linden_common.h"

#include "u64.h"

#include "llframetimer.h"
#include "aiframetimer.h"
#include "aiaprpool.h"

// Local constants.
static F64 const USEC_PER_SECOND = 1000000.0;
static F64 const USEC_TO_SEC_F64 = 0.000001;
static F64 const NEVER = 1e16;

// Static members
U64 const LLFrameTimer::sStartTotalTime = totalTime();				// Application start in microseconds since epoch.
U64 LLFrameTimer::sTotalTime = LLFrameTimer::sStartTotalTime;		// Current time in microseconds since epoch, updated at least once per frame.
F64 LLFrameTimer::sTotalSeconds =									// Current time in seconds since epoch, updated together with LLFrameTimer::sTotalTime.
		U64_to_F64(LLFrameTimer::sTotalTime) * USEC_TO_SEC_F64;
F64 LLFrameTimer::sFrameTime = 0.0;									// Current time in seconds since application start, updated together with LLFrameTimer::sTotalTime.
// Updated exactly once per frame:
S32 LLFrameTimer::sFrameCount = 0;									// Current frame number (number of frames since application start).
U64 LLFrameTimer::sPrevTotalTime = LLFrameTimer::sStartTotalTime;	// Previous (frame) time in microseconds since epoch, updated once per frame.
U64 LLFrameTimer::sFrameDeltaTime = 0;								// Microseconds between last two calls to LLFrameTimer::updateFrameTimeAndCount.
// Mutex for the above.
apr_thread_mutex_t* LLFrameTimer::sGlobalMutex;

// static
void LLFrameTimer::global_initialization(void)
{
	apr_thread_mutex_create(&sGlobalMutex, APR_THREAD_MUTEX_UNNESTED, AIAPRRootPool::get()());
	AIFrameTimer::sNextExpiration = NEVER;
}

// static
void LLFrameTimer::updateFrameTime(void)
{
	llassert(is_main_thread());
	sTotalTime = totalTime();
	sTotalSeconds = U64_to_F64(sTotalTime) * USEC_TO_SEC_F64;
	F64 new_frame_time = U64_to_F64(sTotalTime - sStartTotalTime) * USEC_TO_SEC_F64;
	apr_thread_mutex_lock(sGlobalMutex);
	sFrameTime = new_frame_time;
	apr_thread_mutex_unlock(sGlobalMutex);
}

// static
void LLFrameTimer::updateFrameTimeAndCount(void)
{
	updateFrameTime();
	sFrameDeltaTime = sTotalTime - sPrevTotalTime;
	sPrevTotalTime = sTotalTime;
	++sFrameCount;

	// Handle AIFrameTimer expiration and callbacks.
	if (AIFrameTimer::sNextExpiration <= sFrameTime)
	{
		AIFrameTimer::handleExpiration(sFrameTime);
	}
}

// Don't combine pause/unpause with start/stop
// Useage:
//  LLFrameTime foo; // starts automatically
//  foo.unpause(); // noop but safe
//  foo.pause(); // pauses timer
//  foo.unpause() // unpauses
//  F32 elapsed = foo.getElapsedTimeF32() // does not include time between pause() and unpause()
//  Note: elapsed would also be valid with no unpause() call (= time run until pause() called)
void LLFrameTimer::pause(void)
{
	llassert(is_main_thread());
	if (!mPaused)
	{
		// Only the main thread writes to sFrameTime, so there is no need for locking.
		mStartTime = sFrameTime - mStartTime;	// Abuse mStartTime to store the elapsed time so far.
	}
	mPaused = true;
}

void LLFrameTimer::unpause(void)
{
	llassert(is_main_thread());
	if (mPaused)
	{
		// Only the main thread writes to sFrameTime, so there is no need for locking.
		mStartTime = sFrameTime - mStartTime;	// Set mStartTime consistent with the elapsed time so far.
	}
	mPaused = false;
}

void LLFrameTimer::setExpiryAt(F64 seconds_since_epoch)
{
	llassert(is_main_thread());
	llassert(!mPaused);
	// Only the main thread writes to sFrameTime, so there is no need for locking.
	mStartTime = sFrameTime;
	mExpiry = seconds_since_epoch - (USEC_TO_SEC_F64 * sStartTotalTime);
}

F64 LLFrameTimer::expiresAt(void) const
{
	F64 expires_at = U64_to_F64(sStartTotalTime) * USEC_TO_SEC_F64;
	expires_at += mExpiry;
	return expires_at;
}

bool LLFrameTimer::checkExpirationAndReset(F32 expiration)
{
	llassert(!mPaused);
  	F64 frame_time = getElapsedSeconds();
	if (frame_time >= mExpiry)
	{
		mStartTime = frame_time;
		mExpiry = mStartTime + expiration;
		return true;
	}
	return false;
}

F32 LLFrameTimer::getElapsedTimeAndResetF32(void)
{
  llassert(mRunning && !mPaused);
  F64 frame_time = getElapsedSeconds();
  F32 elapsed_time = (F32)(frame_time - mStartTime);
  mExpiry = mStartTime = frame_time;
  return elapsed_time;
}

// static 
// Return number of seconds between the last two frames.
F32 LLFrameTimer::getFrameDeltaTimeF32(void)
{
	llassert(is_main_thread());
	// Only the main thread writes to sFrameDeltaTime, so there is no need for locking.
	return (F32)(U64_to_F64(sFrameDeltaTime) * USEC_TO_SEC_F64); 
}

// static 
// Return seconds since the current frame started
F32 LLFrameTimer::getCurrentFrameTime(void)
{
	llassert(is_main_thread());
	// Only the main thread writes to sTotalTime, so there is no need for locking.
	U64 frame_time = totalTime() - sTotalTime;
	return (F32)(U64_to_F64(frame_time) * USEC_TO_SEC_F64); 
}

// Glue code to avoid full class .h file #includes
F32 getCurrentFrameTime(void)
{
	return (F32)(LLFrameTimer::getCurrentFrameTime());
}
