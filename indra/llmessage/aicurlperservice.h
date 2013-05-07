/**
 * @file aicurlperservice.h
 * @brief Definition of class AIPerService
 *
 * Copyright (c) 2012, 2013, Aleric Inglewood.
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
 *   04/11/2012
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   06/04/2013
 *   Renamed AIPrivate::PerHostRequestQueue[Ptr] to AIPerHostRequestQueue[Ptr]
 *   to allow public access.
 *
 *   09/04/2013
 *   Renamed everything "host" to "service" and use "hostname:port" as key
 *   instead of just "hostname".
 */

#ifndef AICURLPERSERVICE_H
#define AICURLPERSERVICE_H

#include "llerror.h"		// llassert
#include <string>
#include <deque>
#include <map>
#include <boost/intrusive_ptr.hpp>
#include "aithreadsafe.h"
#include "aiaverage.h"

class AICurlEasyRequest;
class AIPerService;

namespace AICurlPrivate {
namespace curlthread { class MultiHandle; }

class RefCountedThreadSafePerService;
class ThreadSafeBufferedCurlEasyRequest;

// Forward declaration of BufferedCurlEasyRequestPtr (see aicurlprivate.h).
typedef boost::intrusive_ptr<ThreadSafeBufferedCurlEasyRequest> BufferedCurlEasyRequestPtr;

// AIPerService objects are created by the curl thread and destructed by the main thread.
// We need locking.
typedef AIThreadSafeSimpleDC<AIPerService> threadsafe_PerService;
typedef AIAccessConst<AIPerService> PerService_crat;
typedef AIAccess<AIPerService> PerService_rat;
typedef AIAccess<AIPerService> PerService_wat;

} // namespace AICurlPrivate

// We can't put threadsafe_PerService in a std::map because you can't copy a mutex.
// Therefore, use an intrusive pointer for the threadsafe type.
typedef boost::intrusive_ptr<AICurlPrivate::RefCountedThreadSafePerService> AIPerServicePtr;

//-----------------------------------------------------------------------------
// AIPerService

// This class provides a static interface to create and maintain instances of AIPerService objects,
// so that at any moment there is at most one instance per service (hostname:port).
// Those instances then are used to queue curl requests when the maximum number of connections
// for that service already have been reached. And to keep track of the bandwidth usage, and the
// number of queued requests in the pipeline, for this service.
class AIPerService {
  private:
	typedef std::map<std::string, AIPerServicePtr> instance_map_type;
	typedef AIThreadSafeSimpleDC<instance_map_type> threadsafe_instance_map_type;
	typedef AIAccess<instance_map_type> instance_map_rat;
	typedef AIAccess<instance_map_type> instance_map_wat;

	static threadsafe_instance_map_type sInstanceMap;				// Map of AIPerService instances with the hostname as key.

	friend class AIThreadSafeSimpleDC<AIPerService>;	// threadsafe_PerService
	AIPerService(void);

  public:
	~AIPerService();

  public:
	typedef instance_map_type::iterator iterator;
	typedef instance_map_type::const_iterator const_iterator;

	// Utility function; extract canonical (lowercase) hostname and port from url.
	static std::string extract_canonical_servicename(std::string const& url);

	// Return (possibly create) a unique instance for the given hostname.
	static AIPerServicePtr instance(std::string const& servicename);

	// Release instance (object will be deleted if this was the last instance).
	static void release(AIPerServicePtr& instance);

	// Remove everything. Called upon viewer exit.
	static void purge(void);

  private:
	typedef std::deque<AICurlPrivate::BufferedCurlEasyRequestPtr> queued_request_type;

	int mQueuedCommands;						// Number of add commands (minus remove commands) with this host in the command queue.
	int mAdded;									// Number of active easy handles with this host.
	queued_request_type mQueuedRequests;		// Waiting (throttled) requests.

	bool mQueueEmpty;							// Set to true when the queue becomes precisely empty.
	bool mQueueFull;							// Set to true when the queue is popped and then still isn't empty;
	bool mRequestStarvation;					// Set to true when the queue was about to be popped but was already empty.

	AIAverage mHTTPBandwidth;					// Keeps track on number of bytes received for this service in the past second.
	int mConcurrectConnections;					// The maximum number of allowed concurrent connections to this service.
	int mMaxPipelinedRequests;					// The maximum number of accepted requests for this service that didn't finish yet.

	// Global administration of the total number of queued requests of all services combined.
  private:
	struct TotalQueued {
		S32 count;								// The sum of mQueuedRequests.size() of all AIPerService objects together.
		bool empty;								// Set to true when count becomes precisely zero as the result of popping any queue.
		bool full;								// Set to true when count is still larger than zero after popping any queue.
		bool starvation;						// Set to true when any queue was about to be popped when count was already zero.
		TotalQueued(void) : count(0), empty(false), full(false), starvation(false) { }
	};
	static AIThreadSafeSimpleDC<TotalQueued> sTotalQueued;
	typedef AIAccessConst<TotalQueued> TotalQueued_crat;
	typedef AIAccess<TotalQueued> TotalQueued_rat;
	typedef AIAccess<TotalQueued> TotalQueued_wat;
  public:
	static S32 total_queued_size(void) { return TotalQueued_rat(sTotalQueued)->count; }

	// Global administration of the maximum number of pipelined requests of all services combined.
  private:
	struct MaxPipelinedRequests {
		S32 count;								// The maximum total number of accepted requests that didn't finish yet.
		U64 last_increment;						// Last time that sMaxPipelinedRequests was incremented.
		U64 last_decrement;						// Last time that sMaxPipelinedRequests was decremented.
		MaxPipelinedRequests(void) : count(32), last_increment(0), last_decrement(0) { }
	};
	static AIThreadSafeSimpleDC<MaxPipelinedRequests> sMaxPipelinedRequests;
	typedef AIAccessConst<MaxPipelinedRequests> MaxPipelinedRequests_crat;
	typedef AIAccess<MaxPipelinedRequests> MaxPipelinedRequests_rat;
	typedef AIAccess<MaxPipelinedRequests> MaxPipelinedRequests_wat;
  public:
	static void setMaxPipelinedRequests(S32 count) { MaxPipelinedRequests_wat(sMaxPipelinedRequests)->count = count; }
	static void incrementMaxPipelinedRequests(S32 increment) { MaxPipelinedRequests_wat(sMaxPipelinedRequests)->count += increment; }

	// Global administration of throttle fraction (which is the same for all services).
  private:
	struct ThrottleFraction {
	  U32 fraction;								// A value between 0 and 1024: each service is throttled when it uses more than max_bandwidth * (sThrottleFraction/1024) bandwidth.
	  AIAverage average;						// Average of fraction over 25 * 40ms = 1 second.
	  U64 last_add;								// Last time that faction was added to average.
	  ThrottleFraction(void) : fraction(1024), average(25), last_add(0) { }
	};
	static AIThreadSafeSimpleDC<ThrottleFraction> sThrottleFraction;
	typedef AIAccessConst<ThrottleFraction> ThrottleFraction_crat;
	typedef AIAccess<ThrottleFraction> ThrottleFraction_rat;
	typedef AIAccess<ThrottleFraction> ThrottleFraction_wat;

	static LLAtomicU32 sHTTPThrottleBandwidth125;			// HTTPThrottleBandwidth times 125 (in bytes/s).
	static bool sNoHTTPBandwidthThrottling;					// Global override to disable bandwidth throttling.

  public:
	void added_to_command_queue(void) { ++mQueuedCommands; }
	void removed_from_command_queue(void) { --mQueuedCommands; llassert(mQueuedCommands >= 0); }
	void added_to_multi_handle(void);					// Called when an easy handle for this host has been added to the multi handle.
	void removed_from_multi_handle(void);				// Called when an easy handle for this host is removed again from the multi handle.
	bool throttled(void) const;							// Returns true if the maximum number of allowed requests for this host have been added to the multi handle.

	void queue(AICurlEasyRequest const& easy_request);	// Add easy_request to the queue.
	bool cancel(AICurlEasyRequest const& easy_request);	// Remove easy_request from the queue (if it's there).

    void add_queued_to(AICurlPrivate::curlthread::MultiHandle* mh);
														// Add queued easy handle (if any) to the multi handle. The request is removed from the queue,
														// followed by either a call to added_to_multi_handle() or to queue() to add it back.

	S32 pipelined_requests(void) const { return mQueuedCommands + mQueuedRequests.size() + mAdded; }

	AIAverage& bandwidth(void) { return mHTTPBandwidth; }
	AIAverage const& bandwidth(void) const { return mHTTPBandwidth; }

	static void setNoHTTPBandwidthThrottling(bool nb) { sNoHTTPBandwidthThrottling = nb; }
	static void setHTTPThrottleBandwidth(F32 max_kbps) { sHTTPThrottleBandwidth125 = 125.f * max_kbps; }
	static size_t getHTTPThrottleBandwidth125(void) { return sHTTPThrottleBandwidth125; }

	// Called when CurlConcurrentConnectionsPerService changes.
	static void adjust_concurrent_connections(int increment);

	// The two following functions are static and have the AIPerService object passed
	// as first argument as an AIPerServicePtr because that avoids the need of having
	// the AIPerService object locked for the whole duration of the call.
	// The functions only lock it when access is required.

	// Returns true if curl can handle another request for this host.
	// Should return false if the maximum allowed HTTP bandwidth is reached, or when
	// the latency between request and actual delivery becomes too large.
	static bool wantsMoreHTTPRequestsFor(AIPerServicePtr const& per_service);
	// Return true if too much bandwidth is being used.
	static bool checkBandwidthUsage(AIPerServicePtr const& per_service, U64 sTime_40ms);

  private:
	// Disallow copying.
	AIPerService(AIPerService const&);
};

namespace AICurlPrivate {

class RefCountedThreadSafePerService : public threadsafe_PerService {
  public:
	RefCountedThreadSafePerService(void) : mReferenceCount(0) { }
	bool exactly_two_left(void) const { return mReferenceCount == 2; }

  private:
	// Used by AIPerServicePtr. Object is deleted when reference count reaches zero.
	LLAtomicU32 mReferenceCount;

	friend void intrusive_ptr_add_ref(RefCountedThreadSafePerService* p);
	friend void intrusive_ptr_release(RefCountedThreadSafePerService* p);
};

extern U32 CurlConcurrentConnectionsPerService;

} // namespace AICurlPrivate

#endif // AICURLPERSERVICE_H
