/**
 * @file aistatemachinethread.cpp
 * @brief Implementation of AIStateMachineThread
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
 *   23/01/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"
#include "aistatemachinethread.h"

class StateMachineThread : public LLThread {
  private:
	LLPointer<AIThreadImpl> mImpl;
	bool mNeedCleanup;
  public:
	StateMachineThread(AIThreadImpl* impl) : LLThread("StateMachineThread"), mImpl(impl) { }
  protected:
	/*virtual*/ void run(void)
	{
	  mNeedCleanup = !mImpl->done(mImpl->run());
	}
	/*virtual*/ void terminated(void)
	{
	  if (mNeedCleanup)
	  {
		// The state machine that started us has disappeared! Clean up ourselves.
		mStatus = STOPPED;		// Stop LLThread::shutdown from blocking a whole minute and then calling apr_thread_exit to never return!
		// This is OK now because nobody is watching us and having set the status to STOPPED didn't change anything really.
		// Normally is can cause the LLThread to be deleted directly after by the main thread.
		delete this;
	  }
	}
};

void AIStateMachineThreadBase::initialize_impl(void)
{
  mThread = NULL;
  mAbort = false;
  set_state(start_thread);
}

void AIStateMachineThreadBase::multiplex_impl(void)
{
  switch(mRunState)
  {
	case start_thread:
	  // TODO: Implement thread pools. For now just start a new thread.
	  mThread = new StateMachineThread(mImpl);
	  // Set next state.
	  set_state(wait_stopped);
	  idle(wait_stopped);		// Wait till the thread returns.
	  mThread->start();
	  break;
	case wait_stopped:
	  if (!mThread->isStopped())
		break;
	  // We're done!
	  mImpl = NULL;
	  delete mThread;
	  mThread = NULL;
	  if (mAbort)
		abort();
	  else
		finish();
	  break;
  }
}

void AIStateMachineThreadBase::abort_impl(void)
{
  // If this AIStateMachineThreadBase still exists then the first base class of
  // AIStateMachineThread<THREAD_IMPL>, LLPointer<THREAD_IMPL>, also still exists
  // and therefore mImpl is valid (unless we set it NULL above).
  if (mImpl)
  {
	// We can only get here if the statemachine was aborted by it's parent,
	// in that case we'll never reach the "We're done!" above, so this
	// call also signals that the thread has to clean up itself.
	mImpl->abort();
  }
}

void AIStateMachineThreadBase::finish_impl(void)
{
  if (mThread)
  {
	if (!mThread->isStopped())
	{
	  // Lets make sure we're not flooded with this.
	  llwarns << "Thread state machine aborted while the thread is still running. That is a waste of CPU and should be avoided." << llendl;
	  // In fact, this should only ever happen when the state machine was aborted!
	  llassert(aborted());
	  if (!aborted())
	  {
		// Avoid a call back to us.
		mImpl->abort();
	  }
	  mThread->setQuitting();		// Try to signal the thread that is can abort.
	}
	mThread = NULL;
  }
}

char const* AIStateMachineThreadBase::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(start_thread);
	AI_CASE_RETURN(wait_stopped);
  }
  return "UNKNOWN STATE";
}

// AIStateMachineThread THREAD

bool AIThreadImpl::done(bool result)
{
  StateMachineThread_wat state_machine_thread_w(mStateMachineThread);
  AIStateMachineThreadBase* state_machine_thread = *state_machine_thread_w;
  bool state_machine_running = state_machine_thread;
  if (state_machine_running)
  {
	// If state_machine_thread is non-NULL, then AIStateMachineThreadBase::abort_impl was never called,
	// which means the state machine still exists. In fact, it should be in the waiting() state.
	// The only other way it could possibly have been destructed is when finished() was called
	// while the thread was still running, which would be quite illegal and not possible when
	// the statemachine is actually waiting as it should.
	llassert(state_machine_thread->waiting());
	state_machine_thread->schedule_abort(!result);
	state_machine_thread->cont();
  }
  return state_machine_running;
}

