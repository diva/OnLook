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

class RefCountedThreadSafePerServiceRequestQueue;
class ThreadSafeBufferedCurlEasyRequest;

// Forward declaration of BufferedCurlEasyRequestPtr (see aicurlprivate.h).
typedef boost::intrusive_ptr<ThreadSafeBufferedCurlEasyRequest> BufferedCurlEasyRequestPtr;

// AIPerService objects are created by the curl thread and destructed by the main thread.
// We need locking.
typedef AIThreadSafeSimpleDC<AIPerService> threadsafe_PerServiceRequestQueue;
typedef AIAccessConst<AIPerService> PerServiceRequestQueue_crat;
typedef AIAccess<AIPerService> PerServiceRequestQueue_rat;
typedef AIAccess<AIPerService> PerServiceRequestQueue_wat;

} // namespace AICurlPrivate

// We can't put threadsafe_PerServiceRequestQueue in a std::map because you can't copy a mutex.
// Therefore, use an intrusive pointer for the threadsafe type.
typedef boost::intrusive_ptr<AICurlPrivate::RefCountedThreadSafePerServiceRequestQueue> AIPerServicePtr;

//-----------------------------------------------------------------------------
// AIPerService

// This class provides a static interface to create and maintain instances
// of AIPerService objects, so that at any moment there is at most
// one instance per hostname:port. Those instances then are used to queue curl
// requests when the maximum number of connections for that host already
// have been reached.
class AIPerService {
  private:
	typedef std::map<std::string, AIPerServicePtr> instance_map_type;
	typedef AIThreadSafeSimpleDC<instance_map_type> threadsafe_instance_map_type;
	typedef AIAccess<instance_map_type> instance_map_rat;
	typedef AIAccess<instance_map_type> instance_map_wat;

	static threadsafe_instance_map_type sInstanceMap;				// Map of AIPerService instances with the hostname as key.

	friend class AIThreadSafeSimpleDC<AIPerService>;		//threadsafe_PerServiceRequestQueue
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

	static LLAtomicS32 sTotalQueued;			// The sum of mQueuedRequests.size() of all AIPerService objects together.

	bool mQueueEmpty;							// Set to true when the queue becomes precisely empty.
	bool mQueueFull;							// Set to true when the queue is popped and then still isn't empty;
	bool mRequestStarvation;					// Set to true when the queue was about to be popped but was already empty.

	static bool sQueueEmpty;					// Set to true when sTotalQueued becomes precisely zero as the result of popping any queue.
	static bool sQueueFull;						// Set to true when sTotalQueued is still larger than zero after popping any queue.
	static bool sRequestStarvation;				// Set to true when any queue was about to be popped when sTotalQueued was already zero.

	AIAverage mHTTPBandwidth;					// Keeps track on number of bytes received for this service in the past second.
	int mConcurrectConnections;					// The maximum number of allowed concurrent connections to this service.
	int mMaxPipelinedRequests;					// The maximum number of accepted requests that didn't finish yet.

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
	static S32 total_queued_size(void) { return sTotalQueued; }

	AIAverage& bandwidth(void) { return mHTTPBandwidth; }
	AIAverage const& bandwidth(void) const { return mHTTPBandwidth; }

	// Called when CurlConcurrentConnectionsPerService changes.
	static void adjust_concurrent_connections(int increment);

	// Returns true if curl can handle another request for this host.
	// Should return false if the maximum allowed HTTP bandwidth is reached, or when
	// the latency between request and actual delivery becomes too large.
	static bool wantsMoreHTTPRequestsFor(AIPerServicePtr const& per_service, F32 max_kbps, bool no_bandwidth_throttling);

  private:
	// Disallow copying.
	AIPerService(AIPerService const&);
};

namespace AICurlPrivate {

class RefCountedThreadSafePerServiceRequestQueue : public threadsafe_PerServiceRequestQueue {
  public:
	RefCountedThreadSafePerServiceRequestQueue(void) : mReferenceCount(0) { }
	bool exactly_two_left(void) const { return mReferenceCount == 2; }

  private:
	// Used by AIPerServicePtr. Object is deleted when reference count reaches zero.
	LLAtomicU32 mReferenceCount;

	friend void intrusive_ptr_add_ref(RefCountedThreadSafePerServiceRequestQueue* p);
	friend void intrusive_ptr_release(RefCountedThreadSafePerServiceRequestQueue* p);
};

extern U32 CurlConcurrentConnectionsPerService;

} // namespace AICurlPrivate

#endif // AICURLPERSERVICE_H
