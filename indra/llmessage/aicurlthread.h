/**
 * @file aicurlthread.h
 * @brief Thread safe wrapper for libcurl.
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

#ifndef AICURLTHREAD_H
#define AICURLTHREAD_H

#include "aicurl.h"
#include <vector>

#undef AICurlPrivate

namespace AICurlPrivate {
namespace curlthread {

// For ordering a std::set with AICurlEasyRequest objects.
struct AICurlEasyRequestCompare {
  bool operator()(AICurlEasyRequest const& h1, AICurlEasyRequest const& h2) { return h1.get() < h2.get(); }
};

//-----------------------------------------------------------------------------
// PollSet

int const empty = 0x1;
int const complete = 0x2;

enum refresh_t {
  not_complete_not_empty = 0,
  complete_not_empty = complete,
  empty_and_complete = complete|empty
};

class PollSet
{
  public:
	PollSet(void);

	// Add/remove a filedescriptor to/from mFileDescriptors.
	void add(curl_socket_t s);
	void remove(curl_socket_t s);

	// Copy mFileDescriptors to an internal fd_set that is returned by access().
	// Returns if all fds could be copied (complete) and/or if the resulting fd_set is empty.
	refresh_t refresh(void);

	// Return a pointer to the underlaying fd_set.
	fd_set* access(void) { return &mFdSet; }

	// Return the largest fd set in mFdSet by refresh.
	int get_max_fd(void) const { return mMaxFdSet; }

	// Return true if a filedescriptor is set in mFileDescriptors (used for debugging).
	bool contains(curl_socket_t s) const;

	// Return true if a filedescriptor is set in mFdSet.
	bool is_set(curl_socket_t s) const;

	// Clear filedescriptor in mFdSet.
	void clr(curl_socket_t fd);

	// Iterate over all file descriptors that were set by refresh and are still set in mFdSet.
	void reset(void);		// Reset the iterator.
	int get(void) const;	// Return next filedescriptor, or -1 when there are no more.
	                   		// Only valid if reset() was called after the last call to refresh().
	void next(void);		// Advance to next filedescriptor.

  private:
	curl_socket_t* mFileDescriptors;
	size_t mSize;		// Size of mFileDescriptors array.
	int mNrFds;			// The number of filedescriptors in the array.
	int mMaxFd;			// The largest filedescriptor in the array, or -1 when it is empty.
	int mNext;			// The index of the first file descriptor to start copying, the next call to refresh().

	fd_set mFdSet;		// Output variable for select(). (Re)initialized by calling refresh().
	int mMaxFdSet;		// The largest filedescriptor set in mFdSet by refresh(), or -1 when it was empty.

	std::vector<curl_socket_t> mCopiedFileDescriptors;	// File descriptors copied by refresh to mFdSet.
	std::vector<curl_socket_t>::iterator mIter;			// Index into mCopiedFileDescriptors for next(); loop variable.
};

//-----------------------------------------------------------------------------
// MultiHandle

// This class adds member functions that will only be called from the AICurlThread thread.
// This class guarantees that all added easy handles will be removed from the multi handle
// before the multi handle is cleaned up, as is required by libcurl.
class MultiHandle : public CurlMultiHandle
{
  public:
	MultiHandle(void);
	~MultiHandle();

	// Add/remove an easy handle to/from a multi session.
	CURLMcode add_easy_request(AICurlEasyRequest const& easy_request);
	CURLMcode remove_easy_request(AICurlEasyRequest const& easy_request);

	// Reads/writes available data from a particular socket (non-blocking).
	CURLMcode socket_action(curl_socket_t sockfd, int ev_bitmask);

	// Set data to association with an internal socket.
	CURLMcode assign(curl_socket_t sockfd, void* sockptr);

	// Read multi stack informationals.
	CURLMsg const* info_read(int* msgs_in_queue) const;

  private:
	typedef std::set<AICurlEasyRequest, AICurlEasyRequestCompare> addedEasyRequests_type;
	addedEasyRequests_type mAddedEasyRequests;

	bool mHandleAddedOrRemoved;	// Set when an easy handle was added or removed, reset in check_run_count().
	int mPrevRunningHandles;	// The last value of mRunningHandles that check_run_count() was called with.
	int mRunningHandles;		// The last value returned by curl_multi_socket_action.
	long mTimeOut;				// The last time out in ms as set by the call back CURLMOPT_TIMERFUNCTION.

  private:
    static int socket_callback(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);
    static int timer_callback(CURLM* multi, long timeout_ms, void* userp);

  public:
	// Returns the number of active easy handles as reported by the last call to curl_multi_socket_action.
	int getRunningHandles(void) const { return mRunningHandles; }

	// Returns how long to wait for socket action before calling socket_action(CURL_SOCKET_TIMEOUT, 0), in ms.
	int getTimeOut(void) const { return mTimeOut; }

	// This is called before sleeping, after calling (one or more times) socket_action.
	void check_run_count(void);

  public:
	//-----------------------------------------------------------------------------
	// Curl socket administration:

    PollSet mReadPollSet;
	PollSet mWritePollSet;
};

} // namespace curlthread
} // namespace AICurlPrivate

// Thread safe, noncopyable curl multi handle.
// This class wraps MultiHandle for thread-safety.
// AIThreadSafeSingleThreadDC cannot be copied, but that is OK as we don't need that (or want that);
// this class provides a thread-local singleton (exactly one instance per thread), and because it
// can't be copied, that guarantees that the CURLM* handle is never used concurrently, which is
// not allowed by libcurl.
class AICurlMultiHandle : public AIThreadSafeSingleThreadDC<AICurlPrivate::curlthread::MultiHandle>, public LLThreadLocalDataMember {
  public:
	static AICurlMultiHandle& getInstance(void);
  private:
	// Use getInstance().
	AICurlMultiHandle(void) { }
};

typedef AISTAccessConst<AICurlPrivate::curlthread::MultiHandle> AICurlMultiHandle_rat;
typedef AISTAccess<AICurlPrivate::curlthread::MultiHandle> AICurlMultiHandle_wat;

#define AICurlPrivate DONTUSE_AICurlPrivate

#endif
