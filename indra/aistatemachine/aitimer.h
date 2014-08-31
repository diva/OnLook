/**
 * @file aitimer.h
 * @brief Generate a timer event
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

#ifndef AITIMER_H
#define AITIMER_H

#include "aistatemachine.h"
#include "aiframetimer.h"

// A timer state machine.
//
// Before calling run(), call setInterval() to pass needed parameters.
//
// When the state machine finishes it calls the callback, use parameter _1,
// (success) to check whether or not the statemachine actually timed out or
// was cancelled. The boolean is true when it expired and false if the
// state machine was aborted.
//
// Objects of this type can be reused multiple times, see
// also the documentation of AIStateMachine.
//
// Typical usage:
//
// AITimer* timer = new AITimer;
//
// timer->setInterval(5.5);		// 5.5 seconds time out interval.
// timer->run(...);				// Start timer and pass callback; see AIStateMachine.
//
// The default behavior is to call the callback and then delete the AITimer object.
// One can call run() again from the callback function to get a repeating expiration.
// You can call run(...) with parameters too, but using run() without parameters will
// just reuse the old ones (call the same callback).
//
class AITimer : public AIStateMachine {
  protected:
	// The base class of this state machine.
	typedef AIStateMachine direct_base_type;

	// The different states of the state machine.
	enum timer_state_type {
	  AITimer_start = direct_base_type::max_state,
	  AITimer_expired
	};
  public:
	static state_type const max_state = AITimer_expired + 1;

  private:
	AIFrameTimer mFrameTimer;		//!< The actual timer that this object wraps.
	F64 mInterval;					//!< Input variable: interval after which the event will be generated, in seconds.

  public:
	AITimer(CWD_ONLY(bool debug = false)) :
#ifdef CWDEBUG
		AIStateMachine(debug),
#endif
		mInterval(0) { DoutEntering(dc::statemachine(mSMDebug), "AITimer(void) [" << (void*)this << "]"); }

	/**
	 * @brief Set the interval after which the timer should expire.
	 *
	 * @param interval Amount of time in seconds before the timer will expire.
	 *
	 * Call abort() at any time to stop the timer (and delete the AITimer object).
	 */
	void setInterval(F64 interval) { mInterval = interval; }

	/**
	 * @brief Get the expiration interval.
	 *
	 * @returns expiration interval in seconds.
	 */
	F64 getInterval(void) const { return mInterval; }

	/*virtual*/ const char* getName() const { return "AITimer"; }

  protected:
	// Call finish() (or abort()), not delete.
	/*virtual*/ ~AITimer() { DoutEntering(dc::statemachine(mSMDebug), "~AITimer() [" << (void*)this << "]"); mFrameTimer.cancel(); }

	// Handle initializing the object.
	/*virtual*/ void initialize_impl(void);

	// Handle mRunState.
	/*virtual*/ void multiplex_impl(state_type run_state);

	// Handle aborting from current bs_run state.
	/*virtual*/ void abort_impl(void);

	// Implemenation of state_str for run states.
	/*virtual*/ char const* state_str_impl(state_type run_state) const;

  private:
	// This is the callback for mFrameTimer.
	void expired(void);
};

#endif
