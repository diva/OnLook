/**
 * @file aistatemachinethread.h
 * @brief Run code in a thread.
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

#ifndef AISTATEMACHINETHREAD_H
#define AISTATEMACHINETHREAD_H

#include "aistatemachine.h"
#include "llthread.h"
#include "aithreadsafe.h"

#ifdef EXAMPLE_CODE	// undefined

class HelloWorldThread : public AIThreadImpl {
  private:
	bool mStdErr;		// input
	bool mSuccess;		// output

  public:
	// Constructor.
	HelloWorldThread(AIStateMachineThreadBase* state_machine_thread) :
	    AIThreadImpl(state_machine_thread), mStdErr(false), mSuccess(false) { }		// MAIN THREAD

	// Some initialization function (if needed).
	void init(bool err)	{ mStdErr = err; }											// MAIN THREAD
	// Read back output.
	bool successful(void) const { return mSuccess; }

	// Mandatory signature.
	/*virtual*/ bool run(void)														// NEW THREAD
	{
	  if (mStdErr)
		std::cerr << "Hello world" << std::endl;
	  else
		std::cout << "Hello world" << std::endl;
	  mSuccess = true;
	  return true;			// true = finish, false = abort.
	}
};

// The states of this state machine.
enum hello_world_state_type {
  HelloWorld_start = AIStateMachine::max_state,
  HelloWorld_done
};

// The statemachine class (this is almost a template).
class HelloWorld : public AIStateMachine {
  private:
	AIStateMachineThread<HelloWorldThread> mHelloWorld;
	bool mErr;

  public:
	HelloWorld() : mErr(false) { }

	// Print to stderr or stdout?
	void init(bool err) { mErr = err; }

  protected:
	// Call finish() (or abort()), not delete.
	/*virtual*/ ~HelloWorld() { }

	// Handle initializing the object.
	/*virtual*/ void initialize_impl(void);

	// Handle mRunState.
	/*virtual*/ void multiplex_impl(void);

	// Handle aborting from current bs_run state.
	/*virtual*/ void abort_impl(void) { }

	// Handle cleaning up from initialization (or post abort) state.
	/*virtual*/ void finish_impl(void)
	{
	  // Kill object by default.
	  // This can be overridden by calling run() from the callback function.
	  kill();
	}

	// Implemenation of state_str for run states.
	/*virtual*/ char const* state_str_impl(state_type run_state) const
	{
	  switch(run_state)
	  {
		AI_CASE_RETURN(HelloWorld_start);
		AI_CASE_RETURN(HelloWorld_done);
	  }
	  return "UNKNOWN STATE";
	}
};

// The actual implementation of this statemachine starts here!

void HelloWorld::initialize_impl(void)
{
  mHelloWorld->init(mErr);							// Initialize the thread object.
  set_state(HelloWorld_start);
}

void HelloWorld::multiplex_impl(void)
{
  switch (mRunState)
  {
	case HelloWorld_start:
	{
	  mHelloWorld.run(this, HelloWorld_done);		// Run HelloWorldThread and set the state of 'this' to HelloWorld_done when finished.
	  // This statemachine is already idle() here (set to idle by the above call).
	  break;
	}
	case HelloWorld_done:
	{
	  // We're done. Lets also abort when the thread reported no success.
	  if (mHelloWorld->successful())				// Read output/result of thread object.
		finish();
	  else
		abort();
	  break;
	}
  }
}

#endif	// EXAMPLE CODE

class AIStateMachineThreadBase;

// Derive from this to implement the code that must run in another thread.
class AIThreadImpl : public LLThreadSafeRefCount {
  private:
	typedef AIAccess<AIStateMachineThreadBase*> StateMachineThread_wat;
	AIThreadSafeSimpleDC<AIStateMachineThreadBase*> mStateMachineThread;

  protected:
	AIThreadImpl(AIStateMachineThreadBase* state_machine_thread) : mStateMachineThread(state_machine_thread) { }

  public:
	virtual bool run(void) = 0;
	bool done(bool result);
	void abort(void) { *StateMachineThread_wat(mStateMachineThread) = NULL; }
};

// The base class for statemachine threads.
class AIStateMachineThreadBase : public AIStateMachine {
  private:
	// The states of this state machine.
	enum thread_state_type {
	  start_thread = AIStateMachine::max_state,		// Start the thread (if necessary create it first).
	  wait_stopped									// Wait till the thread is stopped.
	};

  protected:
	AIStateMachineThreadBase(AIThreadImpl* impl) : mImpl(impl) { }

  private:
	// Handle initializing the object.
	/*virtual*/ void initialize_impl(void);

	// Handle mRunState.
	/*virtual*/ void multiplex_impl(void);

	// Handle aborting from current bs_run state.
	/*virtual*/ void abort_impl(void);

	// Handle cleaning up from initialization (or post abort) state.
	/*virtual*/ void finish_impl(void);

	// Implemenation of state_str for run states.
	/*virtual*/ char const* state_str_impl(state_type run_state) const;

  private:
	LLThread* mThread;		// The thread that the code is run in.
	AIThreadImpl* mImpl;	// Pointer to the implementation code that needs to be run in the thread.
	bool mAbort;			// (Inverse of) return value of AIThreadImpl::run(). Only valid in state wait_stopped.

  public:
	void schedule_abort(bool do_abort) { mAbort = do_abort; }

};

// The state machine that runs T::run() in a thread.
// THREAD_IMPL Must be derived from AIThreadImpl.
template<typename THREAD_IMPL>
struct AIStateMachineThread : public LLPointer<THREAD_IMPL>, public AIStateMachineThreadBase {
  // Constructor.
  AIStateMachineThread(void) :
	  LLPointer<THREAD_IMPL>(new THREAD_IMPL(this)),
	  AIStateMachineThreadBase(LLPointer<THREAD_IMPL>::get())
  { }
};

#endif

