/**
 * @file aitimer.cpp
 * @brief Implementation of AITimer
 *
 * Copyright (c) 2012, Aleric Inglewood.
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
 *   07/02/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"
#include "aitimer.h"

enum timer_state_type {
  AITimer_start = AIStateMachine::max_state,
  AITimer_expired
};

char const* AITimer::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AITimer_start);
	AI_CASE_RETURN(AITimer_expired);
  }
  return "UNKNOWN STATE";
}

void AITimer::initialize_impl(void)
{
  llassert(!mFrameTimer.isRunning());
  set_state(AITimer_start);
}

void AITimer::expired(void)
{
  set_state(AITimer_expired);
}

void AITimer::multiplex_impl(void)
{
  switch (mRunState)
  {
	case AITimer_start:
	{
	  mFrameTimer.create(mInterval, boost::bind(&AITimer::expired, this));
	  idle();
	  break;
	}
	case AITimer_expired:
	{
	  finish();
	  break;
	}
  }
}

void AITimer::abort_impl(void)
{
  mFrameTimer.cancel();
}

void AITimer::finish_impl(void)
{
  // Kill object by default.
  // This can be overridden by calling run() from the callback function.
  kill();
}

void AIPersistentTimer::finish_impl(void)
{
  // Don't kill object by default.
  if (aborted())
	kill();
  // Callback function should always call kill() or run().
}
