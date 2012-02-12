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

// An AIFrameTimer object provides a callback API for timer events.
//
// Typical usage:
//
// // Any thread.
// AIFrameTimer timer;
//
// ...
// // Any thread (after successful construction is guaranteed).
// timer.create(5.5, boost::bind(&the_callback, <optional params>));	// Call the_callback(<optional params>) in 5.5 seconds.
//
// The callback function is always called by the main thread and should therefore
// be light weight.
//
// If timer.cancel() is called before the timer expires, then the callback
// function isn't called. If cancel() is called by another thread than the
// main thread, then it is possible that the callback function is called
// even while still inside cancel(), but as soon as cancel() returned it
// is guarenteed that the callback function won't be called anymore.
// Hence, if the callback function is a member of some object then
// cancel() must be called before the destruction of that object (ie from
// it's destructor). Calling cancel() multiple times is not a problem.
// Note: if cancel() is called while the callback function is being
// called then cancel() will block until the callback function returned.
//
// The timer object can be reused (by calling create() again), but
// only after either the callback function was called, or after cancel()
// returned. Most notably, you can call create() again from inside the
// callback function to "restart" the timer.
//

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
// This assures that the object is not yet shared at the moment that it is initialized.

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
	// In order to stop us from returning from cancel() while
	// the callback function is being called (which is done
	// in AIFrameTimer::handleExpiration after obtaining the
	// mHandle.mMutex lock), we start with trying to obtain
	// it here and as such wait till the callback function
	// returned.
    mHandle.mMutex.lock();
	// Next we have to grab this lock in order to stop
	// AIFrameTimer::handleExpiration from even entering
	// in the case we manage to get it first.
    sMutex.lock();
    if (mHandle.mRunningTimer != sTimerList.end())
	{
		sTimerList.erase(mHandle.mRunningTimer);
		mHandle.mRunningTimer = sTimerList.end();
		sNextExpiration = sTimerList.empty() ? NEVER : sTimerList.begin()->expiration();
	}
    sMutex.unlock();
    mHandle.mMutex.unlock();
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
		// for the other thread that tries to delete the callback function,
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
		// Note that if the other thread actually obtained the sMutex then we
		// can't be here: this is still inside the critical area of sMutex.
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

