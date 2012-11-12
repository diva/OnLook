/**
 * @file aicurlperhost.h
 * @brief Definition of class PerHostRequestQueue
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
 *   04/11/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AICURLPERHOST_H
#define AICURLPERHOST_H

#include "llerror.h"		// llassert
#include <string>
#include <deque>
#include <map>
#include <boost/intrusive_ptr.hpp>
#include "aithreadsafe.h"

class AICurlEasyRequest;

namespace AICurlPrivate {
namespace curlthread { class MultiHandle; }

class PerHostRequestQueue;
class RefCountedThreadSafePerHostRequestQueue;
class ThreadSafeBufferedCurlEasyRequest;

// Forward declaration of BufferedCurlEasyRequestPtr (see aicurlprivate.h).
typedef boost::intrusive_ptr<ThreadSafeBufferedCurlEasyRequest> BufferedCurlEasyRequestPtr;

// PerHostRequestQueue objects are created by the curl thread and destructed by the main thread.
// We need locking.
typedef AIThreadSafeSimpleDC<PerHostRequestQueue> threadsafe_PerHostRequestQueue;
typedef AIAccessConst<PerHostRequestQueue> PerHostRequestQueue_crat;
typedef AIAccess<PerHostRequestQueue> PerHostRequestQueue_rat;
typedef AIAccess<PerHostRequestQueue> PerHostRequestQueue_wat;

// We can't put threadsafe_PerHostRequestQueue in a std::map because you can't copy a mutex.
// Therefore, use an intrusive pointer for the threadsafe type.
typedef boost::intrusive_ptr<RefCountedThreadSafePerHostRequestQueue> PerHostRequestQueuePtr;

//-----------------------------------------------------------------------------
// PerHostRequestQueue

// This class provides a static interface to create and maintain instances
// of PerHostRequestQueue objects, so that at any moment there is at most
// one instance per hostname. Those instances then are used to queue curl
// requests when the maximum number of connections for that host already
// have been reached.
class PerHostRequestQueue {
  private:
	typedef std::map<std::string, PerHostRequestQueuePtr> instance_map_type;
	typedef AIThreadSafeSimpleDC<instance_map_type> threadsafe_instance_map_type;
	typedef AIAccess<instance_map_type> instance_map_rat;
	typedef AIAccess<instance_map_type> instance_map_wat;

	static threadsafe_instance_map_type sInstanceMap;				// Map of PerHostRequestQueue instances with the hostname as key.

	friend class AIThreadSafeSimpleDC<PerHostRequestQueue>;			//threadsafe_PerHostRequestQueue
	PerHostRequestQueue(void) : mAdded(0) { }

  public:
	typedef instance_map_type::iterator iterator;
	typedef instance_map_type::const_iterator const_iterator;

	// Return (possibly create) a unique instance for the given hostname.
	static PerHostRequestQueuePtr instance(std::string const& hostname);

	// Release instance (object will be deleted if this was the last instance).
	static void release(PerHostRequestQueuePtr& instance);

	// Remove everything. Called upon viewer exit.
	static void purge(void);

  private:
	typedef std::deque<BufferedCurlEasyRequestPtr> queued_request_type;

	int mAdded;									// Number of active easy handles with this host.
	queued_request_type mQueuedRequests;		// Waiting (throttled) requests.

  public:
	void added_to_multi_handle(void);					// Called when an easy handle for this host has been added to the multi handle.
	void removed_from_multi_handle(void);				// Called when an easy handle for this host is removed again from the multi handle.
	bool throttled(void) const;							// Returns true if the maximum number of allowed requests for this host have been added to the multi handle.

	void queue(AICurlEasyRequest const& easy_request);	// Add easy_request to the queue.
	bool cancel(AICurlEasyRequest const& easy_request);	// Remove easy_request from the queue (if it's there).

    void add_queued_to(curlthread::MultiHandle* mh);	// Add queued easy handle (if any) to the multi handle. The request is removed from the queue,
														// followed by either a call to added_to_multi_handle() or to queue() to add it back.
  private:
	// Disallow copying.
	PerHostRequestQueue(PerHostRequestQueue const&) { }
};

class RefCountedThreadSafePerHostRequestQueue : public threadsafe_PerHostRequestQueue {
  public:
	RefCountedThreadSafePerHostRequestQueue(void) : mReferenceCount(0) { }
	bool lastone(void) const { llassert(mReferenceCount >= 2); return mReferenceCount == 2; }

  private:
	// Used by PerHostRequestQueuePtr. Object is deleted when reference count reaches zero.
	LLAtomicU32 mReferenceCount;

	friend void intrusive_ptr_add_ref(RefCountedThreadSafePerHostRequestQueue* p);
	friend void intrusive_ptr_release(RefCountedThreadSafePerHostRequestQueue* p);
};

extern U32 curl_concurrent_connections_per_host;

} // namespace AICurlPrivate

#endif // AICURLPERHOST_H
