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

char const* AITimer::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AITimer_start);
	AI_CASE_RETURN(AITimer_expired);
  }
  llassert(false);
  return "UNKNOWN STATE";
}

void AITimer::initialize_impl(void)
{
  llassert(!mFrameTimer.isRunning());
  set_state(AITimer_start);
}

void AITimer::expired(void)
{
  advance_state(AITimer_expired);
}

void AITimer::multiplex_impl(state_type run_state)
{
  switch (run_state)
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
