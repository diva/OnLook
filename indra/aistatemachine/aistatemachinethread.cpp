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

class AIStateMachineThreadBase::Thread : public LLThread {
  private:
	LLPointer<AIThreadImpl> mImpl;
	bool mNeedCleanup;
  public:
	Thread(AIThreadImpl* impl) : LLThread("AIStateMachineThreadBase::Thread"), mImpl(impl) { }
  protected:
	/*virtual*/ void run(void)
	{
	  mNeedCleanup = mImpl->thread_done(mImpl->run());
	}
	/*virtual*/ void terminated(void)
	{
	  if (mNeedCleanup)
	  {
		// The state machine that started us has disappeared! Clean up ourselves.
		mStatus = STOPPED;		// Stop LLThread::shutdown from blocking a whole minute and then calling apr_thread_exit to never return!
		// This is OK now because nobody is watching us and having set the status to STOPPED didn't change anything really.
		// Normally is can cause the LLThread to be deleted directly after by the main thread.
		Thread::completed(this);
	  }
	}
  public:
	// TODO: Implement a thread pool. For now, just create a new one every time.
	static Thread* allocate(AIThreadImpl* impl) { return new Thread(impl); }
	static void completed(Thread* threadp) { delete threadp; }
};

// MAIN THREAD

char const* AIStateMachineThreadBase::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(start_thread);
	AI_CASE_RETURN(wait_stopped);
  }
  return "UNKNOWN STATE";
}

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
	  mThread = Thread::allocate(mImpl);
	  // Set next state.
	  set_state(wait_stopped);
	  idle(wait_stopped);		// Wait till the thread returns.
	  mThread->start();
	  break;
	case wait_stopped:
	  if (!mThread->isStopped())
		break;
	  // We're done!
	  //
	  // We can only get here when AIThreadImpl::done called cont(), (very
	  // shortly after which it will be STOPPED), which means we need to do
	  // the clean up of the thread.
	  // Also, the thread has really stopped now, so it's safe to delete it.
	  //
	  // Clean up the thread.
	  Thread::completed(mThread);
	  mThread = NULL;			// Stop abort_impl() from doing anything.
	  if (mAbort)
		abort();
	  else
		finish();
	  break;
  }
}

void AIStateMachineThreadBase::abort_impl(void)
{
  if (mThread)
  {
	// If this AIStateMachineThreadBase still exists then the first base class of
	// AIStateMachineThread<THREAD_IMPL>, LLPointer<THREAD_IMPL>, also still exists
	// and therefore mImpl is valid.
	bool need_cleanup = mImpl->state_machine_done(mThread);		// Signal the fact that we aborted.
	if (need_cleanup)
	{
	  // This is an unlikely race condition. We have been aborted by our parent,
	  // but at the same time the thread finished!
	  // We can only get here when AIThreadImpl::thread_done already returned
	  // (VERY shortly after the thread will stop (if not already)).
	  // Just go into a tight loop while waiting for that, when it is safe
	  // to clean up the thread.
	  while (!mThread->isStopped())
	  {
		mThread->yield();
	  }
	  Thread::completed(mThread);
	}
	else
	{
	  // The thread is still happily running (and will clean up itself).
	  // Lets make sure we're not flooded with this situation.
	  llwarns << "Thread state machine aborted while the thread is still running. That is a waste of CPU and should be avoided." << llendl;
	}
  }
}

bool AIThreadImpl::state_machine_done(LLThread* threadp)
{
  StateMachineThread_wat state_machine_thread_w(mStateMachineThread);
  AIStateMachineThreadBase* state_machine_thread = *state_machine_thread_w;
  bool need_cleanup = !state_machine_thread;
  if (!need_cleanup)
  {
	// If state_machine_thread is non-NULL, then AIThreadImpl::thread_done wasnt called yet
	// (or at least didn't return yet) which means the thread is still running.
	// Try telling the thread that it can stop.
	threadp->setQuitting();
	// Finally, mark that we are NOT going to do the cleanup by setting mStateMachineThread to NULL.
	*state_machine_thread_w = NULL;
  }
  return need_cleanup;
}

// AIStateMachineThread THREAD

bool AIThreadImpl::thread_done(bool result)
{
  StateMachineThread_wat state_machine_thread_w(mStateMachineThread);
  AIStateMachineThreadBase* state_machine_thread = *state_machine_thread_w;
  bool need_cleanup = !state_machine_thread;
  if (!need_cleanup)
  {
	// If state_machine_thread is non-NULL then AIThreadImpl::abort_impl wasn't called,
	// which means the state machine still exists. In fact, it should be in the waiting() state.
	// It can also happen that the state machine is being aborted right now (but it will still exist).
	// (Note that waiting() and running() aren't strictly thread-safe (we should really lock
	// mSetStateLock here) but by first calling waiting() and then running(), and assuming that
	// changing an int from the value 1 to the value 2 is atomic, this will work since the
	// only possible transition is from waiting to not running).
	llassert(state_machine_thread->waiting() || !state_machine_thread->running());
	state_machine_thread->schedule_abort(!result);
	// Note that if the state machine is not running (being aborted, ie - hanging in abort_impl
	// waiting for the lock on mStateMachineThread) then this is simply ignored.
	state_machine_thread->cont();
	// Finally, mark that we are NOT going to do the cleanup by setting mStateMachineThread to NULL.
	*state_machine_thread_w = NULL;
  }
  return need_cleanup;
}

