/**
 * @file llcurlrequest.cpp
 * @brief Implementation of Request.
 *
 * Copyright (c) 2012, Aleric Inglewood.
 * Copyright (C) 2010, Linden Research, Inc.
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
 *   17/03/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   20/03/2012
 *   Added copyright notice for Linden Lab for those parts that were
 *   copied or derived from llcurl.cpp. The code of those parts are
 *   already in their own llcurl.cpp, so they do not ever need to
 *   even look at this file; the reason I added the copyright notice
 *   is to make clear that I am not the author of 100% of this code
 *   and hence I cannot change the license of it.
 */

#include "linden_common.h"

#include "llsdserialize.h"
#include "llcurlrequest.h"
#include "statemachine/aicurleasyrequeststatemachine.h"

//-----------------------------------------------------------------------------
// class Request
//

namespace AICurlInterface {

bool Request::get(std::string const& url, ResponderPtr responder)
{
  return getByteRange(url, headers_t(), 0, -1, responder);
}

bool Request::getByteRange(std::string const& url, headers_t const& headers, S32 offset, S32 length, ResponderPtr responder)
{
  DoutEntering(dc::curl, "Request::getByteRange(" << url << ", ...)");

  // This might throw AICurlNoEasyHandle.
  AICurlEasyRequestStateMachine* buffered_easy_request = new AICurlEasyRequestStateMachine(true);

  {
    AICurlEasyRequest_wat buffered_easy_request_w(*buffered_easy_request->mCurlEasyRequest);

	AICurlResponderBuffer_wat(*buffered_easy_request->mCurlEasyRequest)->prepRequest(buffered_easy_request_w, headers, responder);

	buffered_easy_request_w->setopt(CURLOPT_HTTPGET, 1);
	if (length > 0)
	{
	  std::string range = llformat("Range: bytes=%d-%d", offset, offset + length - 1);
	  buffered_easy_request_w->addHeader(range.c_str());
	}

	buffered_easy_request_w->finalizeRequest(url);
  }

  buffered_easy_request->run();

  return true;	// We throw in case of problems.
}

bool Request::post(std::string const& url, headers_t const& headers, std::string const& data, ResponderPtr responder, S32 time_out)
{
  DoutEntering(dc::curl, "Request::post(" << url << ", ...)");

  // This might throw AICurlNoEasyHandle.
  AICurlEasyRequestStateMachine* buffered_easy_request = new AICurlEasyRequestStateMachine(true);

  {
    AICurlEasyRequest_wat buffered_easy_request_w(*buffered_easy_request->mCurlEasyRequest);
	AICurlResponderBuffer_wat buffer_w(*buffered_easy_request->mCurlEasyRequest);

	buffer_w->prepRequest(buffered_easy_request_w, headers, responder);

	buffer_w->getInput().write(data.data(), data.size());
	S32 bytes = buffer_w->getInput().str().length();
	buffered_easy_request_w->setPost(NULL, bytes);
	buffered_easy_request_w->addHeader("Content-Type: application/octet-stream");
	buffered_easy_request_w->finalizeRequest(url);

	lldebugs << "POSTING: " << bytes << " bytes." << llendl;
  }

  buffered_easy_request->run();

  return true;	// We throw in case of problems.
}

bool Request::post(std::string const& url, headers_t const& headers, LLSD const& data, ResponderPtr responder, S32 time_out)
{
  DoutEntering(dc::curl, "Request::post(" << url << ", ...)");

  // This might throw AICurlNoEasyHandle.
  AICurlEasyRequestStateMachine* buffered_easy_request = new AICurlEasyRequestStateMachine(true);

  {
    AICurlEasyRequest_wat buffered_easy_request_w(*buffered_easy_request->mCurlEasyRequest);
	AICurlResponderBuffer_wat buffer_w(*buffered_easy_request->mCurlEasyRequest);

	buffer_w->prepRequest(buffered_easy_request_w, headers, responder);

	LLSDSerialize::toXML(data, buffer_w->getInput());
	S32 bytes = buffer_w->getInput().str().length();
	buffered_easy_request_w->setPost(NULL, bytes);
	buffered_easy_request_w->addHeader("Content-Type: application/llsd+xml");
	buffered_easy_request_w->finalizeRequest(url);

	lldebugs << "POSTING: " << bytes << " bytes." << llendl;
  }

  buffered_easy_request->run();

  return true;	// We throw in case of problems.
}

S32 Request::process(void)
{
  //FIXME: needs implementation
  //DoutEntering(dc::warning, "Request::process()");
  return 0;
}

} // namespace AICurlInterface
//==================================================================================

