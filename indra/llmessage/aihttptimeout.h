/**
 * @file aihttptimeout.h
 * @brief HTTP timeout control class.
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
 *   28/04/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AIHTTPTIMEOUT_H
#define AIHTTPTIMEOUT_H

#include <vector>

#ifndef HTTPTIMEOUT_TESTSUITE

#include "llrefcount.h"
#include "aithreadsafe.h"		// AIAccess

#include <curl/curl.h>      // Needed for files that include this header (also for aicurlprivate.h).
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif

#else

#include <sys/types.h>

class LLRefCount { };
template<typename T> struct AIAccess;
typedef int CURLcode;

#define ASSERT_ONLY_COMMA(...) , __VA_ARGS__

#endif

class AIHTTPTimeoutPolicy;

// Forward declaration (see aicurlprivate.h).
namespace AICurlPrivate {
  class BufferedCurlEasyRequest;
} // namespace AICurlPrivate

typedef AIAccess<AICurlPrivate::BufferedCurlEasyRequest> AICurlEasyRequest_wat;

namespace AICurlPrivate {

class CurlEasyRequest;
class ThreadSafeBufferedCurlEasyRequest;

namespace curlthread {

// A class that keeps track of timeout administration per connection.
class HTTPTimeout : public LLRefCount {
  private:
	AIHTTPTimeoutPolicy const* mPolicy;			// A pointer to the used timeout policy.
	std::vector<U32> mBuckets;					// An array with the number of bytes transfered in each second.
	U16 mBucket;								// The bucket corresponding to mLastSecond.
	bool mNothingReceivedYet;					// Set when created, reset when the HTML reply header from the server is received.
	bool mLowSpeedOn;							// Set while uploading or downloading data.
	bool mUploadFinished;						// Used to keep track of whether upload_finished was called yet.
	S32 mLastSecond;							// The time at which lowspeed() was last called, in seconds since mLowSpeedClock.
	U32 mTotalBytes;							// The sum of all bytes in mBuckets.
	U64 mLowSpeedClock;							// Clock count at which low speed detection (re)started.
	U64 mStalled;								// The clock count at which this transaction is considered to be stalling if nothing is transfered anymore.
  public:
	static F64 const sClockWidth;				// Time between two clock ticks in seconds.
	static U64 sClockCount;						// Clock count used as 'now' during one loop of the main loop.
#if defined(CWDEBUG) || defined(DEBUG_CURLIO)
	ThreadSafeBufferedCurlEasyRequest* mLockObj;
#endif

  public:
	HTTPTimeout(AIHTTPTimeoutPolicy const* policy, ThreadSafeBufferedCurlEasyRequest* lock_obj) :
		mPolicy(policy), mNothingReceivedYet(true), mLowSpeedOn(false), mUploadFinished(false), mStalled((U64)-1)
#if defined(CWDEBUG) || defined(DEBUG_CURLIO)
		, mLockObj(lock_obj)
#endif
		{ }

	// Called when everything we had to send to the server has been sent.
	void upload_finished(void);

	// Called when data is sent. Returns true if transfer timed out.
	bool data_sent(size_t n);

	// Called when data is received. Returns true if transfer timed out.
	bool data_received(size_t n/*,*/ ASSERT_ONLY_COMMA(bool upload_error_status = false));

	// Called immediately before done() after curl finished, with code.
	void done(AICurlEasyRequest_wat const& curlEasyRequest_w, CURLcode code);

	// Accessor.
	bool has_stalled(void) const { return mStalled < sClockCount;  }

	// Called from BufferedCurlEasyRequest::processOutput if a timeout occurred.
	void print_diagnostics(CurlEasyRequest const* curl_easy_request, char const* eff_url);

#if defined(CWDEBUG) || defined(DEBUG_CURLIO)
	void* get_lockobj(void) const { return mLockObj; }
#endif

  private:
	// (Re)start low speed transer rate detection.
	void reset_lowspeed(void);

	// Common low speed detection, Called from data_sent or data_received.
	bool lowspeed(size_t bytes);
};

} // namespace curlthread
} // namespace AICurlPrivate

#endif

