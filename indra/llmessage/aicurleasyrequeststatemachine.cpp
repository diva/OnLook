/**
 * @file aicurleasyrequeststatemachine.cpp
 * @brief Implementation of AICurlEasyRequestStateMachine
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
 *   06/05/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#include "linden_common.h"
#include "aicurleasyrequeststatemachine.h"
#include "aihttptimeoutpolicy.h"
#include "llcontrol.h"

enum curleasyrequeststatemachine_state_type {
  AICurlEasyRequestStateMachine_addRequest = AIStateMachine::max_state,
  AICurlEasyRequestStateMachine_waitAdded,
  AICurlEasyRequestStateMachine_added,
  AICurlEasyRequestStateMachine_timedOut,	// This must be smaller than the rest, so they always overrule.
  AICurlEasyRequestStateMachine_finished,
  AICurlEasyRequestStateMachine_removed,	// The removed states must be largest two, so they are never ignored.
  AICurlEasyRequestStateMachine_removed_after_finished,
  AICurlEasyRequestStateMachine_bad_file_descriptor
};

char const* AICurlEasyRequestStateMachine::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_addRequest);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitAdded);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_added);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_timedOut);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_finished);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_removed);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_removed_after_finished);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_bad_file_descriptor);
  }
  return "UNKNOWN STATE";
}

void AICurlEasyRequestStateMachine::initialize_impl(void)
{
  {
	AICurlEasyRequest_wat curlEasyRequest_w(*mCurlEasyRequest);
	llassert(curlEasyRequest_w->is_finalized());	// Call finalizeRequest() before calling run().
	curlEasyRequest_w->send_handle_events_to(this);
  }
  mAdded = false;
  mTimedOut = false;
  mFinished = false;
  mHandled = false;
  set_state(AICurlEasyRequestStateMachine_addRequest);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::added_to_multi_handle(AICurlEasyRequest_wat&)
{
  set_state(AICurlEasyRequestStateMachine_added);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::finished(AICurlEasyRequest_wat&)
{
  mFinished = true;
  set_state(AICurlEasyRequestStateMachine_finished);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::removed_from_multi_handle(AICurlEasyRequest_wat&)
{
  llassert(mFinished || mTimedOut);		// If we neither finished nor timed out, then why is this being removed?
  										// Note that allowing this would cause an assertion later on for removing
										// a BufferedCurlEasyRequest with a still active Responder.
  set_state(mFinished ? AICurlEasyRequestStateMachine_removed_after_finished : AICurlEasyRequestStateMachine_removed);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::bad_file_descriptor(AICurlEasyRequest_wat&)
{
  if (!mFinished)
  {
	mFinished = true;
	set_state(AICurlEasyRequestStateMachine_bad_file_descriptor);
  }
}

#ifdef SHOW_ASSERT
// CURL-THREAD
void AICurlEasyRequestStateMachine::queued_for_removal(AICurlEasyRequest_wat&)
{
  llassert(mFinished || mTimedOut);		// See AICurlEasyRequestStateMachine::removed_from_multi_handle
}
#endif

void AICurlEasyRequestStateMachine::multiplex_impl(void)
{
  mSetStateLock.lock();
  state_type current_state = mRunState;
  mSetStateLock.unlock();
  switch (current_state)
  {
	case AICurlEasyRequestStateMachine_addRequest:
	{
	  set_state(AICurlEasyRequestStateMachine_waitAdded);
	  idle(AICurlEasyRequestStateMachine_waitAdded);	// Wait till AICurlEasyRequestStateMachine::added_to_multi_handle() is called.
	  // Only AFTER going idle, add request to curl thread; this is needed because calls to set_state() are
	  // ignored when the statemachine is not idle, and theoretically the callbacks could be called
	  // immediately after this call.
	  mAdded = true;
	  mCurlEasyRequest.addRequest();	// This causes the state to be changed, now or later, to
	  									//   AICurlEasyRequestStateMachine_added, then
										//   AICurlEasyRequestStateMachine_finished and then
										//   AICurlEasyRequestStateMachine_removed_after_finished.

	  // The first two states might be skipped thus, and the state at this point is one of
	  // 1) AICurlEasyRequestStateMachine_waitAdded (idle)
	  // 2) AICurlEasyRequestStateMachine_added (running)
	  // 3) AICurlEasyRequestStateMachine_finished (running)
	  // 4) AICurlEasyRequestStateMachine_removed_after_finished (running)

	  if (mTotalDelayTimeout > 0.f)
	  {
		// Set an inactivity timer.
		// This shouldn't really be necessary, except in the case of a bug
		// in libcurl; but lets be sure and set a timer for inactivity.
		mTimer = new AIPersistentTimer;			// Do not delete timer upon expiration.
		mTimer->setInterval(mTotalDelayTimeout);
		mTimer->run(this, AICurlEasyRequestStateMachine_timedOut, false, false);
	  }
	  break;
	}
	case AICurlEasyRequestStateMachine_added:
	{
	  // The request was added to the multi handle. This is a no-op, which is good cause
	  // this state might be skipped anyway ;).
	  idle(current_state);				// Wait for the next event.

	  // The state at this point is one of
	  // 1) AICurlEasyRequestStateMachine_added (idle)
	  // 2) AICurlEasyRequestStateMachine_finished (running)
	  // 3) AICurlEasyRequestStateMachine_removed_after_finished (running)
	  break;
	}
	case AICurlEasyRequestStateMachine_timedOut:
	{
	  // It is possible that exactly at this point the state changes into
	  // AICurlEasyRequestStateMachine_finished, with as result that mTimedOut
	  // is set while we will continue with that state. Hence that mTimedOut
	  // is explicitly reset in that state.

	  // Libcurl failed to deliver within a reasonable time... Abort operation in order
	  // to free this curl easy handle and to notify the application that it didn't work.
	  mTimedOut = true;
	  llassert(mAdded);
	  mAdded = false;
	  mCurlEasyRequest.removeRequest();
	  idle(current_state);				// Wait till AICurlEasyRequestStateMachine::removed_from_multi_handle() is called.
	  break;
	}
	case AICurlEasyRequestStateMachine_finished:
	case AICurlEasyRequestStateMachine_removed_after_finished:
	{
	  if (!mHandled)
	  {
		// Only do this once.
		mHandled = true;

		if (mTimer)
		{
		  // Stop the timer. Note that it's the main thread that generates timer events,
		  // so we're certain that there will be no time out anymore if we reach this point.
		  mTimer->abort();
		}

		// The request finished and either data or an error code is available.
		AICurlEasyRequest_wat easy_request_w(*mCurlEasyRequest);
		easy_request_w->processOutput();
	  }

	  if (current_state == AICurlEasyRequestStateMachine_finished)
	  {
	    idle(current_state);				// Wait till AICurlEasyRequestStateMachine::removed_from_multi_handle() is called.
	    break;
	  }

	  // See above.
	  mTimedOut = false;
	  /* Fall-Through */
	}
	case AICurlEasyRequestStateMachine_removed:
	{
	  // The request was removed from the multi handle.
	  if (mTimedOut)
	  {
		AICurlEasyRequest_wat easy_request_w(*mCurlEasyRequest);
		easy_request_w->timed_out();
	  }

	  // We're done. If we timed out, abort -- or else the application will
	  // think that getResult() will return a valid error code from libcurl.
	  if (mTimedOut)
		abort();
	  else
		finish();

	  break;
	}
	case AICurlEasyRequestStateMachine_bad_file_descriptor:
	{
	  AICurlEasyRequest_wat(*mCurlEasyRequest)->bad_socket();
	  abort();
	}
  }
}

void AICurlEasyRequestStateMachine::abort_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::abort_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  // Revert call to addRequest() if that was already called (and the request wasn't removed again already).
  if (mAdded)
  {
	// Note that it's safe to call this even if the curl thread already removed it, or will removes it
	// after we called this, before processing the remove command; only the curl thread calls
	// MultiHandle::remove_easy_request, which is a no-op when called twice for the same easy request.
	mAdded = false;
	mCurlEasyRequest.removeRequest();
  }
}

void AICurlEasyRequestStateMachine::finish_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::finish_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  // Revoke callbacks.
  {
	AICurlEasyRequest_wat curl_easy_request_w(*mCurlEasyRequest);
	curl_easy_request_w->send_buffer_events_to(NULL);
	curl_easy_request_w->send_handle_events_to(NULL);
	curl_easy_request_w->revokeCallbacks();
  }
  if (mTimer)
  {
	// Note that even if the timer expired, it wasn't deleted because we used AIPersistentTimer; so mTimer is still valid.
	// Stop the timer, if it's still running.
	if (!mHandled)
	  mTimer->abort();
  }
  // Auto clean up ourselves.
  kill();
}

AICurlEasyRequestStateMachine::AICurlEasyRequestStateMachine(void) :
    mTimer(NULL), mTotalDelayTimeout(AIHTTPTimeoutPolicy::getDebugSettingsCurlTimeout().getTotalDelay())
{
  Dout(dc::statemachine, "Calling AICurlEasyRequestStateMachine(void) [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  AICurlInterface::Stats::AICurlEasyRequestStateMachine_count++;
}

void AICurlEasyRequestStateMachine::setTotalDelayTimeout(F32 totalDelayTimeout)
{
  mTotalDelayTimeout = totalDelayTimeout;
}

AICurlEasyRequestStateMachine::~AICurlEasyRequestStateMachine()
{
  Dout(dc::statemachine, "Calling ~AICurlEasyRequestStateMachine() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  --AICurlInterface::Stats::AICurlEasyRequestStateMachine_count;
}

