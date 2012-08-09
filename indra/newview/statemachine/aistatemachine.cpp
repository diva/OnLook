/**
 * @file aistatemachine.cpp
 * @brief Implementation of AIStateMachine
 *
 * Copyright (c) 2010, Aleric Inglewood.
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
 *   01/03/2010
 *   Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"

#include <algorithm>

#include "llcallbacklist.h"
#include "llcontrol.h"
#include "llfasttimer.h"
#include "aithreadsafe.h"
#include "aistatemachine.h"

extern F64 calc_clock_frequency(void);

extern LLControlGroup gSavedSettings;

// Local variables.
namespace {
  struct QueueElementComp;

  class QueueElement {
	private:
	  AIStateMachine* mStateMachine;
	  U64 mRuntime;

	public:
	  QueueElement(AIStateMachine* statemachine) : mStateMachine(statemachine), mRuntime(0) { }
	  friend bool operator==(QueueElement const& e1, QueueElement const& e2) { return e1.mStateMachine == e2.mStateMachine; }
	  friend struct QueueElementComp;

	  AIStateMachine& statemachine(void) const { return *mStateMachine; }
	  void add(U64 count) { mRuntime += count; }
  };

  struct QueueElementComp {
	bool operator()(QueueElement const& e1, QueueElement const& e2) const { return e1.mRuntime < e2.mRuntime; }
  };

  typedef std::vector<QueueElement> active_statemachines_type;
  active_statemachines_type active_statemachines;
  typedef std::vector<AIStateMachine*> continued_statemachines_type;
  struct cscm_type
  {
	continued_statemachines_type continued_statemachines;
	bool calling_mainloop;
  };
  AIThreadSafeDC<cscm_type> continued_statemachines_and_calling_mainloop;
}

// static
AIThreadSafeSimpleDC<U64> AIStateMachine::sMaxCount;

void AIStateMachine::updateSettings(void)
{
  static const LLCachedControl<U32> StateMachineMaxTime("StateMachineMaxTime", 20);
  static U32 last_StateMachineMaxTime = 0;
  if (last_StateMachineMaxTime != StateMachineMaxTime)
  {
    Dout(dc::statemachine, "Initializing AIStateMachine::sMaxCount");
    *AIAccess<U64>(sMaxCount) = calc_clock_frequency() * StateMachineMaxTime / 1000;
	last_StateMachineMaxTime = StateMachineMaxTime;
  }
}

//----------------------------------------------------------------------------
//
// Public methods
//

void AIStateMachine::run(AIStateMachine* parent, state_type new_parent_state, bool abort_parent, bool on_abort_signal_parent)
{
  DoutEntering(dc::statemachine, "AIStateMachine::run(" << (void*)parent << ", " << (parent ? parent->state_str(new_parent_state) : "NA") << ", " << abort_parent << ") [" << (void*)this << "]");
  // Must be the first time we're being run, or we must be called from a callback function.
  llassert(!mParent || mState == bs_callback);
  llassert(!mCallback || mState == bs_callback);
  // Can only be run when in this state.
  llassert(mState == bs_initialize || mState == bs_callback);

  // Allow NULL to be passed as parent to signal that we want to reuse the old one.
  if (parent)
  {
	mParent = parent;
	// In that case remove any old callback!
	if (mCallback)
	{
	  delete mCallback;
	  mCallback = NULL;
	}

	mNewParentState = new_parent_state;
	mAbortParent = abort_parent;
	mOnAbortSignalParent = on_abort_signal_parent;
  }

  // If abort_parent is requested then a parent must be provided.
  llassert(!abort_parent || mParent);
  // If a parent is provided, it must be running.
  llassert(!mParent || mParent->mState == bs_run);

  // Mark that run() has been called, in case we're being called from a callback function.
  mState = bs_initialize;

  // Set mIdle to false and add statemachine to continued_statemachines.
  mSetStateLock.lock();
  locked_cont();
}

void AIStateMachine::run(callback_type::signal_type::slot_type const& slot)
{
  DoutEntering(dc::statemachine, "AIStateMachine::run(<slot>) [" << (void*)this << "]");
  // Must be the first time we're being run, or we must be called from a callback function.
  llassert(!mParent || mState == bs_callback);
  llassert(!mCallback || mState == bs_callback);
  // Can only be run when in this state.
  llassert(mState == bs_initialize || mState == bs_callback);

  // Clean up any old callbacks.
  mParent = NULL;
  if (mCallback)
  {
	delete mCallback;
	mCallback = NULL;
  }

  mCallback = new callback_type(slot);

  // Mark that run() has been called, in case we're being called from a callback function.
  mState = bs_initialize;

  // Set mIdle to false and add statemachine to continued_statemachines.
  mSetStateLock.lock();
  locked_cont();
}

void AIStateMachine::idle(void)
{
  DoutEntering(dc::statemachine, "AIStateMachine::idle() [" << (void*)this << "]");
  llassert(is_main_thread());
  llassert(!mIdle);
  mIdle = true;
  mSleep = 0;
#ifdef SHOW_ASSERT
  mCalledThreadUnsafeIdle = true;
#endif
}

void AIStateMachine::idle(state_type current_run_state)
{
  DoutEntering(dc::statemachine, "AIStateMachine::idle(" << state_str(current_run_state) << ") [" << (void*)this << "]");
  llassert(is_main_thread());
  llassert(!mIdle);
  mSetStateLock.lock();
  // Only go idle if the run state is (still) what we expect it to be.
  // Otherwise assume that another thread called set_state() and continue running.
  if (current_run_state == mRunState)
  {
	mIdle = true;
	mSleep = 0;
  }
  mSetStateLock.unlock();
}

// About thread safeness:
//
// The main thread initializes a statemachine and calls run, so a statemachine
// runs in the main thread. However, it is allowed that a state calls idle()
// and then allows one or more other threads to call cont() upon some
// event (only once, of course, as idle() has to be called before cont()
// can be called again-- and a non-main thread is not allowed to call idle()).
// Instead of cont() one may also call set_state().
// Of course, this may give rise to a race condition; if that happens then
// the thread that calls cont() (set_state()) first is serviced, and the other
// thread(s) are ignored, as if they never called cont().
void AIStateMachine::locked_cont(void)
{
  DoutEntering(dc::statemachine, "AIStateMachine::locked_cont() [" << (void*)this << "]");
  llassert(mIdle);
  // Atomic test mActive and change mIdle.
  mIdleActive.lock();
#ifdef SHOW_ASSERT
  mContThread.reset();
#endif
  mIdle = false;
  bool not_active = mActive == as_idle;
  mIdleActive.unlock();
  // mActive is only changed in AIStateMachine::mainloop, by the main-thread, and
  // here, possibly by any thread. However, after setting mIdle to false above, it
  // is impossible for any thread to come here, until after the main-thread called
  // idle(). So, if this is the main thread then that certainly isn't going to
  // happen until we left this function, while if this is another thread  and the
  // state machine is already running in the main thread then not_active is false
  // and we're already at the end of this function.
  // If not_active is true then main-thread is not running this statemachine.
  // It might call cont() (or set_state()) but never locked_cont(), and will never
  // start actually running until we are done here and release the lock on
  // continued_statemachines_and_calling_mainloop again. It is therefore safe
  // to release mSetStateLock here, with as advantage that if we're not the main-
  // thread and not_active is true, then the main-thread won't block when it has
  // a timer running that times out and calls set_state().
  mSetStateLock.unlock();
  if (not_active)
  {
	AIWriteAccess<cscm_type> cscm_w(continued_statemachines_and_calling_mainloop);
	// See above: it is not possible that mActive was changed since not_active
	// was set to true above.
	llassert_always(mActive == as_idle);
	Dout(dc::statemachine, "Adding " << (void*)this << " to continued_statemachines");
	cscm_w->continued_statemachines.push_back(this);
	if (!cscm_w->calling_mainloop)
	{
	  Dout(dc::statemachine, "Adding AIStateMachine::mainloop to gIdleCallbacks");
	  cscm_w->calling_mainloop = true;
	  gIdleCallbacks.addFunction(&AIStateMachine::mainloop);
	}
	mActive = as_queued;
	llassert_always(!mIdle);	// It should never happen that the main thread calls idle(), while another thread calls cont() concurrently.
  }
}

void AIStateMachine::set_state(state_type state)
{
  DoutEntering(dc::statemachine, "AIStateMachine::set_state(" << state_str(state) << ") [" << (void*)this << "]");

  // Stop race condition of multiple threads calling cont() or set_state() here.
  mSetStateLock.lock();

  // Do not call set_state() unless running.
  llassert(mState == bs_run || !is_main_thread());

  // If this function is called from another thread than the main thread, then we have to ignore
  // it if we're not idle and the state is less than the current state. The main thread must
  // be able to change the state to anything (also smaller values). Note that that only can work
  // if the main thread itself at all times cancels thread callbacks that call set_state()
  // before calling idle() again!
  //
  // Thus: main thead calls idle(), and tells one or more threads to do callbacks on events,
  // which (might) call set_state(). If the main thread calls set_state first (currently only
  // possible as a result of the use of a timer) it will set mIdle to false (here) then cancel
  // the call backs from the other threads and only then call idle() again.
  // Thus if you want other threads get here while mIdle is false to be ignored then the
  // main thread should use a large value for the new run state.
  //
  // If a non-main thread calls set_state first, then the state is changed but the main thread
  // can still override it if it calls set_state before handling the new state; in the latter
  // case it would still be as if the call from the non-main thread was ignored.
  //
  // Concurrent calls from non-main threads however, always result in the largest state
  // to prevail.

  // If the state machine is already running, and we are not the main-thread and the new
  // state is less than the current state, ignore it.
  // Also, if abort() or finish() was called, then we should just ignore it.
  if (mState != bs_run ||
	  (!mIdle && state <= mRunState && !AIThreadID::in_main_thread()))
  {
#ifdef SHOW_ASSERT
	// It's a bit weird if the same thread does two calls on a row where the second call
	// has a smaller value: warn about that.
	if (mState == bs_run && mContThread.equals_current_thread())
	{
	  llwarns << "Ignoring call to set_state(" << state_str(state) <<
		  ") by non-main thread before main-thread could react on previous call, "
		  "because new state is smaller than old state (" << state_str(mRunState) << ")." << llendl;
	}
#endif
	mSetStateLock.unlock();
	return;		// Ignore.
  }

  // Do not call idle() when set_state is called from another thread; use idle(state_type) instead.
  llassert(!mCalledThreadUnsafeIdle || is_main_thread());

  // Change mRunState to the requested value.
  if (mRunState != state)
  {
	mRunState = state;
	Dout(dc::statemachine, "mRunState set to " << state_str(mRunState));
  }

  // Continue the state machine if appropriate.
  if (mIdle)
	locked_cont();				// This unlocks mSetStateLock.
  else
	mSetStateLock.unlock();

  // If we get here then mIdle is false, so only mRunState can still be changed but we won't
  // call locked_cont() anymore. When the main thread finally picks up on the state change,
  // it will cancel any possible callbacks from other threads and process the largest state
  // that this function was called with in the meantime.
}

void AIStateMachine::abort(void)
{
  DoutEntering(dc::statemachine, "AIStateMachine::abort() [" << (void*)this << "]");
  // It's possible that abort() is called before calling AIStateMachine::multiplex.
  // In that case the statemachine wasn't initialized yet and we should just kill() it.
  if (LL_UNLIKELY(mState == bs_initialize))
  {
	// It's ok to use the thread-unsafe idle() here, because if the statemachine
	// wasn't started yet, then other threads won't call set_state() on it.
	if (!mIdle)
	  idle();
	// run() calls locked_cont() after which the top of the mainloop adds this
	// state machine to active_statemachines. Therefore, if the following fails
	// then either the same statemachine called run() immediately followed by abort(),
	// which is not allowed; or there were two active statemachines running,
	// the first created a new statemachine and called run() on it, and then
	// the other (before reaching the top of the mainloop) called abort() on
	// that freshly created statemachine. Obviously, this is highly unlikely,
	// but if that is the case then here we bump the statemachine into
	// continued_statemachines to prevent kill() to delete this statemachine:
	// the caller of abort() does not expect that.
	if (LL_UNLIKELY(mActive == as_idle))
	{
	  mSetStateLock.lock();
	  locked_cont();
	  idle();
	}
	kill();
  }
  else
  {
	llassert(mState == bs_run);
	mSetStateLock.lock();
	mState = bs_abort;		// Causes additional calls to set_state to be ignored.
	mSetStateLock.unlock();
	abort_impl();
	mAborted = true;
	finish();
  }
}

void AIStateMachine::finish(void)
{
  DoutEntering(dc::statemachine, "AIStateMachine::finish() [" << (void*)this << "]");
  mSetStateLock.lock();
  llassert(mState == bs_run || mState == bs_abort);
  // It is possible that mIdle is true when abort or finish was called from
  // outside multiplex_impl. However, that only may be done by the main thread.
  llassert(!mIdle || is_main_thread());
  if (!mIdle)
	idle();					// After calling this, we don't want other threads to call set_state() anymore.
  mState = bs_finish;		// Causes additional calls to set_state to be ignored.
  mSetStateLock.unlock();
  finish_impl();
  // Did finish_impl call kill()? Then that is only the default. Remember it.
  bool default_delete = (mState == bs_killed);
  mState = bs_finish;
  if (mParent)
  {
	// It is possible that the parent is not running when the parent is in fact aborting and called
	// abort on this object from it's abort_impl function. It that case we don't want to recursively
	// call abort again (or change it's state).
	if (mParent->running())
	{
	  if (mAborted && mAbortParent)
	  {
		mParent->abort();
		mParent = NULL;
	  }
	  else if (!mAborted || mOnAbortSignalParent)
	  {
		mParent->set_state(mNewParentState);
	  }
	}
  }
  // After this (bool)*this evaluates to true and we can call the callback, which then is allowed to call run().
  mState = bs_callback;
  if (mCallback)
  {
	// This can/may call kill() that sets mState to bs_kill and in which case the whole AIStateMachine
	// will be deleted from the mainloop, or it may call run() that sets mState is set to bs_initialize
	// and might change or reuse mCallback or mParent.
	mCallback->callback(!mAborted);
	if (mState != bs_initialize)
	{
	  delete mCallback;
	  mCallback = NULL;
	  mParent = NULL;
	}
  }
  else
  {
	// Not restarted by callback. Allow run() to be called later on.
	mParent = NULL;
  }
  // Fix the final state.
  if (mState == bs_callback)
	mState = default_delete ? bs_killed : bs_initialize;
  if (mState == bs_killed && mActive == as_idle)
  {
	// Bump the statemachine onto the active statemachine list, or else it won't be deleted.
	mSetStateLock.lock();
	locked_cont();
	idle();
  }
}

void AIStateMachine::kill(void)
{
  DoutEntering(dc::statemachine, "AIStateMachine::kill() [" << (void*)this << "]");
  // Should only be called from finish() (or when not running (bs_initialize)).
  // However, also allow multiple calls to kill() on a row (bs_killed) (which effectively don't do anything).
  llassert(mIdle && (mState == bs_callback || mState == bs_finish || mState == bs_initialize || mState == bs_killed));
  base_state_type prev_state = mState;
  mState = bs_killed;
  if (prev_state == bs_initialize && mActive == as_idle)
  {
	// We're not running (ie being deleted by a parent statemachine), delete it immediately.
	delete this;
  }
}

// Return stringified 'state'.
char const* AIStateMachine::state_str(state_type state)
{
  if (state >= min_state && state < max_state)
  {
	switch (state)
	{
	  AI_CASE_RETURN(bs_initialize);
	  AI_CASE_RETURN(bs_run);
	  AI_CASE_RETURN(bs_abort);
	  AI_CASE_RETURN(bs_finish);
	  AI_CASE_RETURN(bs_callback);
	  AI_CASE_RETURN(bs_killed);
	}
  }
  return state_str_impl(state);
}

//----------------------------------------------------------------------------
//
// Private methods
//

void AIStateMachine::multiplex(U64 current_time)
{
  // Return immediately when this state machine is sleeping.
  // A negative value of mSleep means we're counting frames,
  // a positive value means we're waiting till a certain
  // amount of time has passed.
  if (mSleep != 0)
  {
	if (mSleep < 0)
	{
	  if (++mSleep)
		return;
	}
	else
	{
	  if (current_time < (U64)mSleep)
		return;
	  mSleep = 0;
	}
  }

  DoutEntering(dc::statemachine, "AIStateMachine::multiplex() [" << (void*)this << "] [with state: " << state_str(mState == bs_run ? mRunState : mState) << "]");
  llassert(mState == bs_initialize || mState == bs_run);

  // Real state machine starts here.
  if (mState == bs_initialize)
  {
	mAborted = false;
	mState = bs_run;
	initialize_impl();
	if (mAborted || mState != bs_run)
	  return;
  }
  multiplex_impl();
}

//static
void AIStateMachine::add_continued_statemachines(void)
{
  AIReadAccess<cscm_type> cscm_r(continued_statemachines_and_calling_mainloop);
  bool nonempty = false;
  for (continued_statemachines_type::const_iterator iter = cscm_r->continued_statemachines.begin(); iter != cscm_r->continued_statemachines.end(); ++iter)
  {
	nonempty = true;
	active_statemachines.push_back(QueueElement(*iter));
	Dout(dc::statemachine, "Adding " << (void*)*iter << " to active_statemachines");
	(*iter)->mActive = as_active;
  }
  if (nonempty)
	AIWriteAccess<cscm_type>(cscm_r)->continued_statemachines.clear();
}

static LLFastTimer::DeclareTimer FTM_STATEMACHINE("State Machine");
// static
void AIStateMachine::mainloop(void*)
{
  LLFastTimer t(FTM_STATEMACHINE);
  add_continued_statemachines();
  llassert(!active_statemachines.empty());
  // Run one or more state machines.
  U64 total_clocks = 0;
  U64 max_count = *AIAccess<U64>(sMaxCount);
  for (active_statemachines_type::iterator iter = active_statemachines.begin(); iter != active_statemachines.end(); ++iter)
  {
	AIStateMachine& statemachine(iter->statemachine());
	if (!statemachine.mIdle)
	{
	  U64 start = LLFastTimer::getCPUClockCount64();
	  // This might call idle() and then pass the statemachine to another thread who then may call cont().
	  // Hence, after this isn't not sure what mIdle is, and it can change from true to false at any moment,
	  // if it is true after this function returns.
	  statemachine.multiplex(start);
	  U64 delta = LLFastTimer::getCPUClockCount64() - start;
	  iter->add(delta);
	  total_clocks += delta;
	  if (total_clocks >= max_count)
	  {
#ifndef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "AIStateMachine::mainloop did run for " << (total_clocks * 1000 / calc_clock_frequency()) << " ms." << llendl;
#endif
		std::sort(active_statemachines.begin(), active_statemachines.end(), QueueElementComp());
		break;
	  }
	}
  }
  // Remove idle state machines from the loop.
  active_statemachines_type::iterator iter = active_statemachines.begin();
  while (iter != active_statemachines.end())
  {
	AIStateMachine& statemachine(iter->statemachine());
	// Atomic test mIdle and change mActive.
	bool locked = statemachine.mIdleActive.tryLock();
	// If the lock failed, then another thread is in the middle of calling cont(),
	// thus mIdle will end up false. So, there is no reason to block here; just
	// treat mIdle as false already.
	if (locked && statemachine.mIdle)
	{
	  // Without the lock, it would be possible that another thread called cont() right here,
	  // changing mIdle to false again but NOT adding the statemachine to continued_statemachines,
	  // thinking it is in active_statemachines (and it is), while immediately below it is
	  // erased from active_statemachines.
	  statemachine.mActive = as_idle;
	  // Now, calling cont() is ok -- as that will cause the statemachine to be added to
	  // continued_statemachines, so it's fine in that case-- even necessary-- to remove it from
	  // active_statemachines regardless, and we can release the lock here.
	  statemachine.mIdleActive.unlock();
	  Dout(dc::statemachine, "Erasing " << (void*)&statemachine << " from active_statemachines");
	  iter = active_statemachines.erase(iter);
	  if (statemachine.mState == bs_killed)
	  {
	  	Dout(dc::statemachine, "Deleting " << (void*)&statemachine);
		delete &statemachine;
	  }
	}
	else
	{
	  if (locked)
	  {
		statemachine.mIdleActive.unlock();
	  }
	  llassert(statemachine.mActive == as_active);	// It should not be possible that another thread called cont() and changed this when we are we are not idle.
	  llassert(statemachine.mState == bs_run || statemachine.mState == bs_initialize);
	  ++iter;
	}
  }
  if (active_statemachines.empty())
  {
	// If this was the last state machine, remove mainloop from the IdleCallbacks.
	AIReadAccess<cscm_type> cscm_r(continued_statemachines_and_calling_mainloop);
	if (cscm_r->continued_statemachines.empty() && cscm_r->calling_mainloop)
	{
	  Dout(dc::statemachine, "Removing AIStateMachine::mainloop from gIdleCallbacks");
	  AIWriteAccess<cscm_type>(cscm_r)->calling_mainloop = false;
	  gIdleCallbacks.deleteFunction(&AIStateMachine::mainloop);
	}
  }
}

// static
void AIStateMachine::flush(void)
{
  DoutEntering(dc::curl, "AIStateMachine::flush(void)");
  add_continued_statemachines();
  // Abort all state machines.
  for (active_statemachines_type::iterator iter = active_statemachines.begin(); iter != active_statemachines.end(); ++iter)
  {
	AIStateMachine& statemachine(iter->statemachine());
	if (statemachine.abortable())
	{
	  // We can't safely call abort() here for non-running (run() was called, but they we're initialized yet) statemachines,
	  // because that might call kill() which in some cases is undesirable (ie, when it is owned by a partent that will
	  // also call abort() on it when it is aborted itself).
	  if (statemachine.running())
		statemachine.abort();
	  else
		statemachine.idle();		// Stop the statemachine from starting, in the next loop with batch == 0.
	}
  }
  for (int batch = 0;; ++batch)
  {
	// Run mainloop until all state machines are idle (batch == 0) or deleted (batch == 1).
	for(;;)
	{
	  {
		AIReadAccess<cscm_type> cscm_r(continued_statemachines_and_calling_mainloop);
		if (!cscm_r->calling_mainloop)
		  break;
	  }
	  mainloop(NULL);
	}
	if (batch == 1)
	  break;
	add_continued_statemachines();
	// Kill all state machines.
	for (active_statemachines_type::iterator iter = active_statemachines.begin(); iter != active_statemachines.end(); ++iter)
	{
	  AIStateMachine& statemachine(iter->statemachine());
	  if (statemachine.running())
		statemachine.kill();
	}
  }
}
