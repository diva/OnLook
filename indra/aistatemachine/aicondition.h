/**
 * @file aicondition.h
 * @brief Condition variable for statemachines.
 *
 * Copyright (c) 2013, Aleric Inglewood.
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
 *   14/10/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AICONDITION_H
#define AICONDITION_H

#include <deque>
#include <llpointer.h>
#include "aithreadsafe.h"

class AIStateMachine;
class LLMutex;

// class AICondition
//
// Call AIStateMachine::wait(AICondition&) in the multiplex_impl of a state machine to
// make the state machine go idle until some thread calls AICondition::signal().
//
// If the state machine is no longer running or wasn't waiting anymore because
// something else woke it up, then AICondition::signal() will wake up another
// state machine (if any).
//
// Usage:
//
// struct Foo { bool met(); };	// Returns true when the condition is met.
// AICondition<Foo> Condition_t;
// AIAccess<Foo> Condition_wat;
//
// // Some thread-safe condition variable.
// Condition_t condition;
//
// // Inside the state machine:
// {
//   ...
//   state WAIT_FOR_CONDITION:
//   {
//     // Lock condition and check it. Wait if condition is not met yet.
//     {
//       Condition_wat condition_w(condition);
//       if (!condition_w->met())
//       {
//         wait(condition);
//         break;
//       }
//     }
//     set_state(CONDITION_MET);
//     break;
//   }
//   CONDITION_MET:
//   {
//

class AIConditionBase
{
  public:
	virtual ~AIConditionBase() { }

	void signal(int n = 1);											// Call this when the condition was met to release n state machines.
	void broadcast(void) { signal(mWaitingStateMachines.size()); }	// Release all blocked state machines.

  private:
	// These functions are called by AIStateMachine.
	friend class AIStateMachine;
	void wait(AIStateMachine* state_machine);
	void remove(AIStateMachine* state_machine);

  protected:
	virtual LLMutex& mutex(void)  = 0;

  protected:
	typedef std::deque<LLPointer<AIStateMachine> > queue_t;
	queue_t mWaitingStateMachines;
};

template<typename T>
class AICondition : public AIThreadSafeSimpleDC<T>, public AIConditionBase
{
  protected:
	/*virtual*/ LLMutex& mutex(void) { return this->mMutex; }
};

#endif

