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

enum curleasyrequeststatemachine_state_type {
  AICurlEasyRequestStateMachine_addRequest = AIStateMachine::max_state,
  AICurlEasyRequestStateMachine_waitAdded,
  AICurlEasyRequestStateMachine_waitFinished,
  AICurlEasyRequestStateMachine_finished
};

char const* AICurlEasyRequestStateMachine::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_addRequest);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitAdded);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitFinished);
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
  set_state(AICurlEasyRequestStateMachine_waitFinished);
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
	  mCurlEasyRequest.addRequest();
	  set_state(AICurlEasyRequestStateMachine_waitAdded);
	}
	case AICurlEasyRequestStateMachine_waitAdded:
	{
	  idle();			// Wait till AICurlEasyRequestStateMachine::added_to_multi_handle() is called.
	  break;
	}
	case AICurlEasyRequestStateMachine_waitFinished:
	{
	  idle();			// Wait till AICurlEasyRequestStateMachine::finished() is called.
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
  Dout(dc::curl, "AICurlEasyRequestStateMachine::abort_impl called for = " << (void*)mCurlEasyRequest.get());
  // We must first revoke the events, or the curl thread might change mRunState still.
  {
    AICurlEasyRequest_wat curl_easy_request_w(*mCurlEasyRequest);
	curl_easy_request_w->send_events_to(NULL);
	curl_easy_request_w->revokeCallbacks();
  }
  if (mRunState >= AICurlEasyRequestStateMachine_waitAdded && mRunState < AICurlEasyRequestStateMachine_finished)
  {
	// Revert call to addRequest().
	// Note that it's safe to call this even if the curl thread already removed it, or will removes it
	// after we called this, before processing the remove command; only the curl thread calls
	// MultiHandle::remove_easy_request, which is a no-op when called twice for the same easy request.
	mCurlEasyRequest.removeRequest();
  }
}

void AICurlEasyRequestStateMachine::finish_impl(void)
{
  Dout(dc::curl, "AICurlEasyRequestStateMachine::finish_impl called for = " << (void*)mCurlEasyRequest.get());
  if (!aborted())
  {
    AICurlEasyRequest_wat curl_easy_request_w(*mCurlEasyRequest);
	curl_easy_request_w->send_events_to(NULL);
	curl_easy_request_w->revokeCallbacks();
  }
}

AICurlEasyRequestStateMachine::AICurlEasyRequestStateMachine(bool buffered) : mBuffered(buffered), mCurlEasyRequest(buffered)
{
  Dout(dc::statemachine, "Calling AICurlEasyRequestStateMachine(" << (buffered ? "true" : "false") << ") [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
}

AICurlEasyRequestStateMachine::~AICurlEasyRequestStateMachine()
{
  Dout(dc::statemachine, "Calling ~AICurlEasyRequestStateMachine() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
}

