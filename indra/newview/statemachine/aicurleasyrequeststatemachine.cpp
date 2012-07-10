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
#include "llcontrol.h"

enum curleasyrequeststatemachine_state_type {
  AICurlEasyRequestStateMachine_addRequest = AIStateMachine::max_state,
  AICurlEasyRequestStateMachine_waitAdded,
  AICurlEasyRequestStateMachine_waitRemoved,
  AICurlEasyRequestStateMachine_timedOut,		// Only _finished has a higher priority than _timedOut.
  AICurlEasyRequestStateMachine_finished
};

char const* AICurlEasyRequestStateMachine::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_addRequest);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitAdded);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitRemoved);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_timedOut);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_finished);
  }
  return "UNKNOWN STATE";
}

void AICurlEasyRequestStateMachine::initialize_impl(void)
{
  {
	AICurlEasyRequest_wat curlEasyRequest_w(*mCurlEasyRequest);
	llassert(curlEasyRequest_w->is_finalized());	// Call finalizeRequest(url) before calling run().
	curlEasyRequest_w->send_events_to(this);
  }
  set_state(AICurlEasyRequestStateMachine_addRequest);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::added_to_multi_handle(AICurlEasyRequest_wat&)
{
  set_state(AICurlEasyRequestStateMachine_waitRemoved);
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::finished(AICurlEasyRequest_wat&)
{
}

// CURL-THREAD
void AICurlEasyRequestStateMachine::removed_from_multi_handle(AICurlEasyRequest_wat&)
{
  set_state(AICurlEasyRequestStateMachine_finished);
}

void AICurlEasyRequestStateMachine::multiplex_impl(void)
{
  switch (mRunState)
  {
	case AICurlEasyRequestStateMachine_addRequest:
	{
	  set_state(AICurlEasyRequestStateMachine_waitAdded);
	  idle(AICurlEasyRequestStateMachine_waitAdded);			// Wait till AICurlEasyRequestStateMachine::added_to_multi_handle() is called.
	  // Only AFTER going idle, add request to curl thread; this is needed because calls to set_state() are
	  // ignored when the statemachine is not idle, and theoretically the callbacks could be called
	  // immediately after this call.
	  mCurlEasyRequest.addRequest();

	  // Set an inactivity timer.
	  // This shouldn't really be necessary, except in the case of a bug
	  // in libcurl; but lets be sure and set a timer for inactivity.
	  static LLCachedControl<F32> CurlRequestTimeOut("CurlRequestTimeOut", 40.f);
	  mTimer = new AIPersistentTimer;			// Do not delete timer upon expiration.
	  mTimer->setInterval(CurlRequestTimeOut);
	  mTimer->run(this, AICurlEasyRequestStateMachine_timedOut);

	  break;
	}
	case AICurlEasyRequestStateMachine_waitRemoved:
	{
	  idle(AICurlEasyRequestStateMachine_waitRemoved);			// Wait till AICurlEasyRequestStateMachine::removed_from_multi_handle() is called.
	  break;
	}
	case AICurlEasyRequestStateMachine_timedOut:
	{
	  // Libcurl failed to end on error(?)... abort operation in order to free
	  // this curl easy handle and to notify the application that it didn't work.
	  abort();
	  break;
	}
	case AICurlEasyRequestStateMachine_finished:
	{
	  if (mBuffered)
	  {
		AICurlEasyRequest_wat easy_request_w(*mCurlEasyRequest);
		AICurlResponderBuffer_wat buffered_easy_request_w(*mCurlEasyRequest);
		buffered_easy_request_w->processOutput(easy_request_w);
	  }
	  finish();
	  break;
	}
  }
}

void AICurlEasyRequestStateMachine::abort_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::abort_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  // Revert call to addRequest() if that was already called (and the request wasn't removed already again).
  if (AICurlEasyRequestStateMachine_waitAdded <= mRunState && mRunState < AICurlEasyRequestStateMachine_finished)
  {
	// Note that it's safe to call this even if the curl thread already removed it, or will removes it
	// after we called this, before processing the remove command; only the curl thread calls
	// MultiHandle::remove_easy_request, which is a no-op when called twice for the same easy request.
	mCurlEasyRequest.removeRequest();
  }
}

void AICurlEasyRequestStateMachine::finish_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::finish_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  // Revoke callbacks.
  {
	AICurlEasyRequest_wat curl_easy_request_w(*mCurlEasyRequest);
	curl_easy_request_w->send_events_to(NULL);
	curl_easy_request_w->revokeCallbacks();
  }
  // Note that even if the timer expired, it wasn't deleted because we used AIPersistentTimer; so mTimer is still valid.
  // Stop the timer.
  mTimer->abort();
  // And delete it here.
  mTimer->kill();
  // Auto clean up ourselves.
  kill();
}

AICurlEasyRequestStateMachine::AICurlEasyRequestStateMachine(bool buffered) : mBuffered(buffered), mCurlEasyRequest(buffered)
{
  Dout(dc::statemachine, "Calling AICurlEasyRequestStateMachine(" << (buffered ? "true" : "false") << ") [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
}

AICurlEasyRequestStateMachine::~AICurlEasyRequestStateMachine()
{
  Dout(dc::statemachine, "Calling ~AICurlEasyRequestStateMachine() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
}

