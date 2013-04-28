/**
 * @file aiperservice.cpp
 * @brief Implementation of AIPerService
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
 *   Renamed AICurlPrivate::PerHostRequestQueue[Ptr] to AIPerHostRequestQueue[Ptr]
 *   to allow public access.
 *
 *   09/04/2013
 *   Renamed everything "host" to "service" and use "hostname:port" as key
 *   instead of just "hostname".
 */

#include "sys.h"
#include "aicurlperservice.h"
#include "aicurlthread.h"
#include "llcontrol.h"

AIPerService::threadsafe_instance_map_type AIPerService::sInstanceMap;
LLAtomicS32 AIPerService::sTotalQueued;
bool AIPerService::sQueueEmpty;
bool AIPerService::sQueueFull;
bool AIPerService::sRequestStarvation;

#undef AICurlPrivate

namespace AICurlPrivate {

// Cached value of CurlConcurrentConnectionsPerService.
U32 CurlConcurrentConnectionsPerService;

// Friend functions of RefCountedThreadSafePerServiceRequestQueue

void intrusive_ptr_add_ref(RefCountedThreadSafePerServiceRequestQueue* per_service)
{
  per_service->mReferenceCount++;
}

void intrusive_ptr_release(RefCountedThreadSafePerServiceRequestQueue* per_service)
{
  if (--per_service->mReferenceCount == 0)
  {
    delete per_service;
  }
}

} // namespace AICurlPrivate

using namespace AICurlPrivate;

AIPerService::AIPerService(void) :
		mQueuedCommands(0), mAdded(0), mQueueEmpty(false),
		mQueueFull(false), mRequestStarvation(false), mHTTPBandwidth(25),	// 25 = 1000 ms / 40 ms.
		mConcurrectConnections(CurlConcurrentConnectionsPerService),
		mMaxPipelinedRequests(CurlConcurrentConnectionsPerService)
{
}

// url must be of the form
// (see http://www.ietf.org/rfc/rfc3986.txt Appendix A for definitions not given here):
//
// url			= sheme ":" hier-part [ "?" query ] [ "#" fragment ]
// hier-part	= "//" authority path-abempty
// authority     = [ userinfo "@" ] host [ ":" port ]
// path-abempty  = *( "/" segment )
//
// That is, a hier-part of the form '/ path-absolute', '/ path-rootless' or
// '/ path-empty' is NOT allowed here. This should be safe because we only
// call this function for curl access, any file access would use APR.
//
// However, as a special exception, this function allows:
//
// url			= authority path-abempty
//
// without the 'sheme ":" "//"' parts.
//
// As follows from the ABNF (see RFC, Appendix A):
// - authority is either terminated by a '/' or by the end of the string because
//   neither userinfo, host nor port may contain a '/'.
// - userinfo does not contain a '@', and if it exists, is always terminated by a '@'.
// - port does not contain a ':', and if it exists is always prepended by a ':'.
//
//static
std::string AIPerService::extract_canonical_servicename(std::string const& url)
{
  char const* p = url.data();
  char const* const end = p + url.size();
  char const* sheme_colon = NULL;
  char const* sheme_slash = NULL;
  char const* first_ampersand = NULL;
  char const* port_colon = NULL;
  std::string servicename;
  char const* hostname = p;                 // Default in the case there is no "sheme://userinfo@".
  while (p < end)
  {
    int c = *p;
    if (c == ':')
    {
      if (!port_colon && LLStringOps::isDigit(p[1]))
      {
        port_colon = p;
      }
      else if (!sheme_colon && !sheme_slash && !first_ampersand && !port_colon)
      {
        // Found a colon before any slash or ampersand: this has to be the colon between the sheme and the hier-part.
        sheme_colon = p;
      }
    }
    else if (c == '/')
    {
      if (!sheme_slash && sheme_colon && sheme_colon == p - 1 && !first_ampersand && p[1] == '/')
      {
        // Found the first '/' in the first occurance of the sequence "://".
        sheme_slash = p;
        hostname = ++p + 1;                 // Point hostname to the start of the authority, the default when there is no "userinfo@" part.
        servicename.clear();                // Remove the sheme.
      }
      else
      {
        // Found slash that is not part of the "sheme://" string. Signals end of authority.
        // We're done.
        break;
      }
    }
    else if (c == '@')
    {
      if (!first_ampersand)
      {
        first_ampersand = p;
        hostname = p + 1;
        servicename.clear();                // Remove the "userinfo@"
      }
    }
    if (p >= hostname)
    {
      // Convert hostname to lowercase in a way that we compare two hostnames equal iff libcurl does.
#if APR_CHARSET_EBCDIC
#error Not implemented
#else
      if (c >= 'A' && c <= 'Z')
        c += ('a' - 'A');
#endif
      servicename += c;
    }
    ++p;
  }
  // Strip of any trailing ":80".
  if (p - 3 == port_colon && p[-1] == '0' && p[-2] == '8')
  {
    return servicename.substr(0, p - hostname - 3);
  }
  return servicename;
}

//static
AIPerServicePtr AIPerService::instance(std::string const& servicename)
{
  llassert(!servicename.empty());
  instance_map_wat instance_map_w(sInstanceMap);
  AIPerService::iterator iter = instance_map_w->find(servicename);
  if (iter == instance_map_w->end())
  {
	iter = instance_map_w->insert(instance_map_type::value_type(servicename, new RefCountedThreadSafePerServiceRequestQueue)).first;
  }
  // Note: the creation of AIPerServicePtr MUST be protected by the lock on sInstanceMap (see release()).
  return iter->second;
}

//static
void AIPerService::release(AIPerServicePtr& instance)
{
  if (instance->exactly_two_left())		// Being 'instance' and the one in sInstanceMap.
  {
	// The viewer can be have left main() we can't access the global sInstanceMap anymore.
	if (LLApp::isStopped())
	{
	  return;
	}
	instance_map_wat instance_map_w(sInstanceMap);
	// It is possible that 'exactly_two_left' is not up to date anymore.
	// Therefore, recheck the condition now that we have locked sInstanceMap.
	if (!instance->exactly_two_left())
	{
	  // Some other thread added this host in the meantime.
	  return;
	}
	// The reference in the map is the last one; that means there can't be any curl easy requests queued for this host.
	llassert(PerServiceRequestQueue_rat(*instance)->mQueuedRequests.empty());
	// Find the host and erase it from the map.
	iterator const end = instance_map_w->end();
	for(iterator iter = instance_map_w->begin(); iter != end; ++iter)
	{
	  if (instance == iter->second)
	  {
		instance_map_w->erase(iter);
		instance.reset();
		return;
	  }
	}
	// We should always find the host.
	llassert(false);
  }
  instance.reset();
}

bool AIPerService::throttled() const
{
  return mAdded >= mConcurrectConnections;
}

void AIPerService::added_to_multi_handle(void)
{
  ++mAdded;
}

void AIPerService::removed_from_multi_handle(void)
{
  --mAdded;
  llassert(mAdded >= 0);
}

void AIPerService::queue(AICurlEasyRequest const& easy_request)
{
  mQueuedRequests.push_back(easy_request.get_ptr());
  sTotalQueued++;
}

bool AIPerService::cancel(AICurlEasyRequest const& easy_request)
{
  queued_request_type::iterator const end = mQueuedRequests.end();
  queued_request_type::iterator cur = std::find(mQueuedRequests.begin(), end, easy_request.get_ptr());

  if (cur == end)
	return false;		// Not found.

  // We can't use erase because that uses assignment to move elements,
  // because it isn't thread-safe. Therefore, move the element that we found to 
  // the back with swap (could just swap with the end immediately, but I don't
  // want to break the order in which requests where added). Swap is also not
  // thread-safe, but OK here because it only touches the objects in the deque,
  // and the deque is protected by the lock on the AIPerService object.
  queued_request_type::iterator prev = cur;
  while (++cur != end)
  {
	prev->swap(*cur);				// This is safe,
	prev = cur;
  }
  mQueuedRequests.pop_back();		// if this is safe.
  --sTotalQueued;
  llassert(sTotalQueued >= 0);
  return true;
}

void AIPerService::add_queued_to(curlthread::MultiHandle* multi_handle)
{
  if (!mQueuedRequests.empty())
  {
	multi_handle->add_easy_request(mQueuedRequests.front());
	mQueuedRequests.pop_front();
	llassert(sTotalQueued > 0);
	if (!--sTotalQueued)
	{
	  // We obtained a request from the queue, and after that there we no more request in any queue.
	  sQueueEmpty = true;
	}
	else
	{
	  // We obtained a request from the queue, and even after that there was at least one more request in some queue.
	  sQueueFull = true;
	}
	if (mQueuedRequests.empty())
	{
	  // We obtained a request from the queue, and after that there we no more request in the queue of this host.
	  mQueueEmpty = true;
	}
	else
	{
	  // We obtained a request from the queue, and even after that there was at least one more request in the queue of this host.
	  mQueueFull = true;
	}
  }
  else
  {
	// We can add a new request, but there is none in the queue!
	mRequestStarvation = true;
	if (sTotalQueued == 0)
	{
	  // The queue of every host is empty!
	  sRequestStarvation = true;
	}
  }
}

//static
void AIPerService::purge(void)
{
  instance_map_wat instance_map_w(sInstanceMap);
  for (iterator host = instance_map_w->begin(); host != instance_map_w->end(); ++host)
  {
	Dout(dc::curl, "Purging queue of host \"" << host->first << "\".");
	PerServiceRequestQueue_wat per_service_w(*host->second);
	size_t s = per_service_w->mQueuedRequests.size();
	per_service_w->mQueuedRequests.clear();
	sTotalQueued -= s;
	llassert(sTotalQueued >= 0);
  }
}

//static
void AIPerService::adjust_concurrent_connections(int increment)
{
  instance_map_wat instance_map_w(sInstanceMap);
  for (AIPerService::iterator iter = instance_map_w->begin(); iter != instance_map_w->end(); ++iter)
  {
	PerServiceRequestQueue_wat per_service_w(*iter->second);
	U32 old_concurrent_connections = per_service_w->mConcurrectConnections;
	per_service_w->mConcurrectConnections = llclamp(old_concurrent_connections + increment, (U32)1, CurlConcurrentConnectionsPerService);
	increment = per_service_w->mConcurrectConnections - old_concurrent_connections;
	per_service_w->mMaxPipelinedRequests = llmax(per_service_w->mMaxPipelinedRequests + increment, 0);
  }
}

