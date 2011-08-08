/**
 * @file aiframetimer.h
 * @brief Implementation of AIFrameTimer.
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
 *   05/08/2011
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AIFRAMETIMER_H
#define AIFRAMETIMER_H

#include "llframetimer.h"

class LL_COMMON_API AIFrameTimer
{
  private:
	F64 mExpire;								// Time at which the timer expires, in seconds since application start (compared to LLFrameTimer::sFrameTime).
	static std::set<AIFrameTimer> sTimerList;	// List with all running timers.

	friend class LLFrameTimer;
	static F64 sNextExpiration;					// Cache of smallest value in sTimerList.

  public:
	AIFrameTimer(F64 expiration) : mExpire(LLFrameTimer::getElapsedSeconds() + expiration) { }

	static void handleExpiration(F64 current_frame_time);

	friend bool operator<(AIFrameTimer const& ft1, AIFrameTimer const& ft2) { return ft1.mExpire < ft2.mExpire; }
};


#endif
