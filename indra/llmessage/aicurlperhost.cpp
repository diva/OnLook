/**
 * @file aiperhost.cpp
 * @brief Implementation of AIPerHostRequestQueue
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
 */

#include "sys.h"
#include "aicurlperhost.h"
#include "aicurlthread.h"

AIPerHostRequestQueue::threadsafe_instance_map_type AIPerHostRequestQueue::sInstanceMap;
LLAtomicS32 AIPerHostRequestQueue::sTotalQueued;

#undef AICurlPrivate

namespace AICurlPrivate {

U32 curl_concurrent_connections_per_host;

// Friend functions of RefCountedThreadSafePerHostRequestQueue

void intrusive_ptr_add_ref(RefCountedThreadSafePerHostRequestQueue* per_host)
{
  per_host->mReferenceCount++;
}

void intrusive_ptr_release(RefCountedThreadSafePerHostRequestQueue* per_host)
{
  if (--per_host->mReferenceCount == 0)
  {
    delete per_host;
  }
}

} // namespace AICurlPrivate

using namespace AICurlPrivate;

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
std::string AIPerHostRequestQueue::extract_canonical_hostname(std::string const& url)
{
  std::string::size_type pos;
  std::string::size_type authority = 0;														// Default if there is no sheme.
  if ((pos = url.find("://")) != url.npos && pos < url.find('/')) authority = pos + 3;		// Skip the "sheme://" if any, the second find is to avoid finding a "://" as part of path-abempty.
  std::string::size_type host = authority;													// Default if there is no userinfo.
  if ((pos = url.find('@', authority)) != url.npos) host = pos + 1;							// Skip the "userinfo@" if any.
  authority = url.length() - 1;																// Default last character of host if there is no path-abempty.
  if ((pos = url.find('/', host)) != url.npos) authority = pos - 1;							// Point to last character of host.
  std::string::size_type len = url.find_last_not_of(":0123456789", authority) - host + 1;	// Skip trailing ":port", if any.
  std::string hostname(url, host, len);
#if APR_CHARSET_EBCDIC
#error Not implemented
#else
  // Convert hostname to lowercase in a way that we compare two hostnames equal iff libcurl does.
  for (std::string::iterator iter = hostname.begin(); iter != hostname.end(); ++iter)
  {
	int c = *iter;
	if (c >= 'A' && c <= 'Z')
	  *iter = c + ('a' - 'A');
  }
#endif
  return hostname;
}

//static
AIPerHostRequestQueuePtr AIPerHostRequestQueue::instance(std::string const& hostname)
{
  llassert(!hostname.empty());
  instance_map_wat instance_map_w(sInstanceMap);
  AIPerHostRequestQueue::iterator iter = instance_map_w->find(hostname);
  if (iter == instance_map_w->end())
  {
	iter = instance_map_w->insert(instance_map_type::value_type(hostname, new RefCountedThreadSafePerHostRequestQueue)).first;
  }
  // Note: the creation of AIPerHostRequestQueuePtr MUST be protected by the lock on sInstanceMap (see release()).
  return iter->second;
}

//static
void AIPerHostRequestQueue::release(AIPerHostRequestQueuePtr& instance)
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
	llassert(PerHostRequestQueue_rat(*instance)->mQueuedRequests.empty());
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

bool AIPerHostRequestQueue::throttled() const
{
  llassert(mAdded <= int(curl_concurrent_connections_per_host));
  return mAdded == int(curl_concurrent_connections_per_host);
}

void AIPerHostRequestQueue::added_to_multi_handle(void)
{
  llassert(mAdded < int(curl_concurrent_connections_per_host));
  ++mAdded;
}

void AIPerHostRequestQueue::removed_from_multi_handle(void)
{
  --mAdded;
  llassert(mAdded >= 0);
}

void AIPerHostRequestQueue::queue(AICurlEasyRequest const& easy_request)
{
  mQueuedRequests.push_back(easy_request.get_ptr());
  sTotalQueued++;
}

bool AIPerHostRequestQueue::cancel(AICurlEasyRequest const& easy_request)
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
  // and the deque is protected by the lock on the AIPerHostRequestQueue object.
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

void AIPerHostRequestQueue::add_queued_to(curlthread::MultiHandle* multi_handle)
{
  if (!mQueuedRequests.empty())
  {
	multi_handle->add_easy_request(mQueuedRequests.front());
	mQueuedRequests.pop_front();
	--sTotalQueued;
	llassert(sTotalQueued >= 0);
  }
}

//static
void AIPerHostRequestQueue::purge(void)
{
  instance_map_wat instance_map_w(sInstanceMap);
  for (iterator host = instance_map_w->begin(); host != instance_map_w->end(); ++host)
  {
	Dout(dc::curl, "Purging queue of host \"" << host->first << "\".");
	PerHostRequestQueue_wat per_host_w(*host->second);
	size_t s = per_host_w->mQueuedRequests.size();
	per_host_w->mQueuedRequests.clear();
	sTotalQueued -= s;
	llassert(sTotalQueued >= 0);
  }
}

