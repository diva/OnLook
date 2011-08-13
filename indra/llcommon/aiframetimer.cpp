/**
 * @file aiframetimer.cpp
 *
 * Copyright (c) 2011, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   06/08/2011
 *   - Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"

#include "aiframetimer.h"

static F64 const NEVER = 1e16;				// 317 million years.

F64 AIFrameTimer::sNextExpiration;
AIFrameTimer::timer_list_type AIFrameTimer::sTimerList;
LLMutex AIFrameTimer::sMutex;

// Notes on thread-safety of AIRunningFrameTimer (continued from aiframetimer.h)
//
// Most notably, the constructor and init() should be called as follows:
// 1) The object is constructed (AIRunningFrameTimer::AIRunningFrameTimer).
// 2) The lock is obtained.
// 3) The object is inserted in the list (operator<(AIRunningFrameTimer const&, AIRunningFrameTimer const&)).
// 4) The object is initialized (AIRunningFrameTimer::init).
// 5) The lock is released.
// This assures that the object is not yet shared at the moment
// that it is initialized (assignment to mConnection is not thread-safe).

void AIFrameTimer::create(F64 expiration, signal_type::slot_type const& slot)
{
	AIRunningFrameTimer new_timer(expiration, this);
	sMutex.lock();
	llassert(mHandle.mRunningTimer == sTimerList.end());	// Create may only be called when the timer isn't already running.
	mHandle.init(sTimerList.insert(new_timer), slot);
	sNextExpiration = sTimerList.begin()->expiration();
	sMutex.unlock();
}

void AIFrameTimer::cancel(void)
{
    mHandle.mMutex.lock();
    sMutex.lock();
    mHandle.mMutex.unlock();
    if (mHandle.mRunningTimer != sTimerList.end())
	{
		timer_list_type::iterator running_timer = mHandle.mRunningTimer;
		mHandle.mRunningTimer = sTimerList.end();
		sTimerList.erase(running_timer);
		sNextExpiration = sTimerList.empty() ? NEVER : sTimerList.begin()->expiration();
	}
    sMutex.unlock();
}

void AIFrameTimer::handleExpiration(F64 current_frame_time)
{
    sMutex.lock();
	for(;;)
	{
		if (sTimerList.empty())
		{
			// No running timers left.
			sNextExpiration = NEVER;
			break;
		}
		timer_list_type::iterator running_timer = sTimerList.begin();
		sNextExpiration = running_timer->expiration();
		if (sNextExpiration > current_frame_time)
		{
			// Didn't expire yet.
			break;
		}

		// Obtain handle of running timer through the associated AIFrameTimer object.
		// Note that if the AIFrameTimer object was destructed (when running_timer->getTimer()
		// would return an invalid pointer) then it called cancel(), so we can't be here.
		Handle& handle(running_timer->getTimer()->mHandle);
		llassert_always(running_timer == handle.mRunningTimer);

		// We're going to erase this timer, so stop cancel() from doing the same.
		handle.mRunningTimer = sTimerList.end();

		// We keep handle.mMutex during the callback to prevent the thread that
		// owns the AIFrameTimer from deleting the callback function while we
		// call it: in order to do so it first has to call cancel(), which will
		// block until we release this mutex again, or we won't call the callback
		// function here because the trylock fails.
		//
		// Assuming the main thread arrived here, there are two possible states
		// for the other thread that tries to delete the call back function,
		// right after calling the cancel() function too:
		//
		// 1. It hasn't obtained the first lock yet, we obtain the handle.mMutex
		// lock and the other thread will stall on the first line of cancel().
		// After do_callback returns, the other thread will do nothing because
		// handle.mRunningTimer equals sTimerList.end(), exit the function and
		// (possibly) delete the callback object, but that is ok as we already
		// returned from the callback function.
		//
		// 2. It already called cancel() and hangs on the second line trying to
		// obtain sMutex.lock(). The trylock below fails and we never call the
		// callback function. We erase the running timer here and release sMutex
		// at the end, after which the other thread does nothing because
		// handle.mRunningTimer equals sTimerList.end(), exits the function and
		// (possibly) deletes the callback object.
		//
		// Note that if the other thread actually obtained the sMutex and
		// released handle.mMutex again, then we can't be here: this is still
		// inside the critical area of sMutex.
		if (handle.mMutex.tryLock())			// If this fails then another thread is in the process of cancelling this timer, so do nothing.
		{
			sMutex.unlock();
			running_timer->do_callback();		// May not throw exceptions.
			sMutex.lock();
			handle.mMutex.unlock();				// Allow other thread to return from cancel() and possibly delete the callback object.
		}

		// Erase the timer from the running list.
		sTimerList.erase(running_timer);
	}
    sMutex.unlock();
}

