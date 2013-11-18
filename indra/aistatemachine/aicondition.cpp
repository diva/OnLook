/**
 * @file aicondition.cpp
 * @brief Implementation of AICondition
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

#include "sys.h"
#include "aicondition.h"
#include "aistatemachine.h"

void AIConditionBase::wait(AIStateMachine* state_machine)
{
  // The condition must be locked before calling AIStateMachine::wait().
  llassert(mutex().isSelfLocked());
  // Add the new state machine at the end.
  mWaitingStateMachines.push_back(state_machine);
}

void AIConditionBase::remove(AIStateMachine* state_machine)
{
  mutex().lock();
  // Remove all occurances of state_machine from the queue.
  queue_t::iterator const end = mWaitingStateMachines.end();
  queue_t::iterator last = end;
  for (queue_t::iterator iter = mWaitingStateMachines.begin(); iter != last; ++iter)
  {
	if (iter->get() == state_machine)
	{
	  if (--last == iter)
	  {
		break;
	  }
	  queue_t::value_type::swap(*iter, *last);
	}
  }
  // This invalidates all iterators involved, including end, but not any iterators to the remaining elements.
  mWaitingStateMachines.erase(last, end);
  mutex().unlock();
}

void AIConditionBase::signal(int n)
{
  // The condition must be locked before calling AICondition::signal or AICondition::broadcast.
  llassert(mutex().isSelfLocked());
  // Signal n state machines.
  while (n > 0 && !mWaitingStateMachines.empty())
  {
	LLPointer<AIStateMachine> state_machine = mWaitingStateMachines.front();
	bool success = state_machine->signalled();
	// Only state machines that are actually still blocked should be in the queue:
	// they are removed from the queue by calling AICondition::remove whenever
	// they are unblocked for whatever reason...
	llassert(success);
	if (success)
	{
	  ++n;
	}
	else
	{
	  // We never get here...
	  remove(state_machine.get());
	}
  }
}

