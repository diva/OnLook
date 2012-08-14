/**
 * @file aicurlthread.cpp
 * @brief Implementation of AICurl, curl thread functions.
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

#include "linden_common.h"
#include "aicurlthread.h"
#include "lltimer.h"		// ms_sleep
#include <sys/types.h>
#if !LL_WINDOWS
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <deque>

// On linux, add -DDEBUG_WINDOWS_CODE_ON_LINUX to test the windows code used in this file.
#if !defined(DEBUG_WINDOWS_CODE_ON_LINUX) || !defined(LL_LINUX) || defined(LL_RELEASE)
#undef DEBUG_WINDOWS_CODE_ON_LINUX
#define DEBUG_WINDOWS_CODE_ON_LINUX 0
#endif

#if DEBUG_WINDOWS_CODE_ON_LINUX

struct windows_fd_set {
  unsigned int fd_count;
  curl_socket_t fd_array[64];
};

unsigned int const not_found  = (unsigned int)-1;

static inline unsigned int find_fd(curl_socket_t s, windows_fd_set const* fsp)
{
  for (unsigned int i = 0; i < fsp->fd_count; ++i)
  {
	if (fsp->fd_array[i] == s)
	  return i;
  }
  return not_found;
}

static int windows_select(int, windows_fd_set* readfds, windows_fd_set* writefds, windows_fd_set*, timeval* val)
{
  fd_set r;
  fd_set w;
  FD_ZERO(&r);
  FD_ZERO(&w);
  int mfd = -1;
  if (readfds)
  {
	for (int i  = 0; i < readfds->fd_count; ++i)
	{
	  int fd = readfds->fd_array[i];
	  FD_SET(fd, &r);
	  mfd = llmax(mfd, fd);
	}
  }
  if (writefds)
  {
	for (int i  = 0; i < writefds->fd_count; ++i)
	{
	  int fd = writefds->fd_array[i];
	  FD_SET(fd, &w);
	  mfd = llmax(mfd, fd);
	}
  }
  int nfds = select(mfd + 1, readfds ? &r : NULL, writefds ? &w : NULL, NULL, val);
  if (readfds)
  {
	unsigned int fd_count = 0;
	for (int i  = 0; i < readfds->fd_count; ++i)
	{
	  if (FD_ISSET(readfds->fd_array[i], &r))
		readfds->fd_array[fd_count++] = readfds->fd_array[i];
	}
	readfds->fd_count = fd_count;
  }
  if (writefds)
  {
	unsigned int fd_count = 0;
	for (int i  = 0; i < writefds->fd_count; ++i)
	{
	  if (FD_ISSET(writefds->fd_array[i], &w))
		writefds->fd_array[fd_count++] = writefds->fd_array[i];
	}
	writefds->fd_count = fd_count;
  }
  return nfds;
}

#undef FD_SETSIZE
#undef FD_ZERO
#undef FD_ISSET
#undef FD_SET
#undef FD_CLR

int const FD_SETSIZE  = sizeof(windows_fd_set::fd_array) / sizeof(curl_socket_t);

static void FD_ZERO(windows_fd_set* fsp)
{
  fsp->fd_count = 0;
}

static bool FD_ISSET(curl_socket_t s, windows_fd_set const* fsp)
{
  return find_fd(s, fsp) != not_found;
}

static void FD_SET(curl_socket_t s, windows_fd_set* fsp)
{
  llassert(!FD_ISSET(s, fsp));
  fsp->fd_array[fsp->fd_count++] = s;
}

static void FD_CLR(curl_socket_t s, windows_fd_set* fsp)
{
  unsigned int i = find_fd(s, fsp);
  llassert(i != not_found);
  fsp->fd_array[i] = fsp->fd_array[--(fsp->fd_count)];
}

#define fd_set windows_fd_set
#define select windows_select

int WSAGetLastError(void)
{
  return errno;
}

typedef char* LPTSTR;

bool FormatMessage(int, void*, int e, int, LPTSTR error_str_ptr, int, void*)
{
  char* error_str = *(LPTSTR*)error_str_ptr;
  error_str = strerror(e);
  return true;
}

void LocalFree(LPTSTR)
{
}

int const FORMAT_MESSAGE_ALLOCATE_BUFFER = 0;
int const FORMAT_MESSAGE_FROM_SYSTEM = 0;
int const FORMAT_MESSAGE_IGNORE_INSERTS = 0;
int const INVALID_SOCKET = -1;
int const SOCKET_ERROR = -1;
int const WSAEWOULDBLOCK = EWOULDBLOCK;

int closesocket(curl_socket_t fd)
{
  return close(fd);
}

int const FIONBIO = 0;

int ioctlsocket(int fd, int, unsigned long* nonblocking_enable)
{
  int res = fcntl(fd, F_GETFL, 0);
  llassert_always(res != -1);
  if (*nonblocking_enable)
	res |= O_NONBLOCK;
  else
	res &= ~O_NONBLOCK;
  return fcntl(fd, F_SETFD, res);
}

#endif // DEBUG_WINDOWS_CODE_ON_LINUX

#define WINDOWS_CODE (LL_WINDOWS || DEBUG_WINDOWS_CODE_ON_LINUX)

#undef AICurlPrivate

namespace AICurlPrivate {

enum command_st {
  cmd_none,
  cmd_add,
  cmd_boost,
  cmd_remove
};

class Command {
  public:
	Command(void) : mCommand(cmd_none) { }
	Command(AICurlEasyRequest const& easy_request, command_st command) : mCurlEasyRequest(easy_request.get_ptr()), mCommand(command) { }

	command_st command(void) const { return mCommand; }
	CurlEasyRequestPtr const& easy_request(void) const { return mCurlEasyRequest; }

	bool operator==(AICurlEasyRequest const& easy_request) const { return mCurlEasyRequest == easy_request.get_ptr(); }

	void reset(void);

  private:
	CurlEasyRequestPtr mCurlEasyRequest;
	command_st mCommand;
};

void Command::reset(void)
{
  mCurlEasyRequest.reset();
  mCommand = cmd_none;
}

// The following two globals have separate locks for speed considerations (in order not
// to block the main thread unnecessarily) but have the following correlation:
//
// MAIN-THREAD (AICurlEasyRequest::addRequest)
// * command_queue locked
//   - A non-active (mActiveMultiHandle is NULL) ThreadSafeCurlEasyRequest (by means of an AICurlEasyRequest pointing to it) is added to command_queue with as command cmd_add.
// * command_queue unlocked
//
// If at this point addRequest is called again, then it is detected that the last command added to the queue
// for this ThreadSafeCurlEasyRequest is cmd_add.
//
// CURL-THREAD (AICurlThread::wakeup):
// * command_queue locked
//   * command_being_processed is write-locked
//     - command_being_processed is assigned the value of the command in the queue.
//   * command_being_processed is unlocked
//   - The command is removed from command_queue
// * command_queue unlocked
//
// If at this point addRequest is called again, then it is detected that command_being_processed adds the same ThreadSafeCurlEasyRequest.
//
// * command_being_processed is read-locked
//   - mActiveMultiHandle is set to point to the curl multi handle
//   - The easy handle is added to the multi handle
// * command_being_processed is write-locked
//   - command_being_processed is reset
// * command_being_processed is unlocked
//
// If at this point addRequest is called again, then it is detected that the ThreadSafeCurlEasyRequest is active.

// Multi-threaded queue for passing Command objects from the main-thread to the curl-thread.
AIThreadSafeSimpleDC<std::deque<Command> > command_queue;
typedef AIAccess<std::deque<Command> > command_queue_wat;

AIThreadSafeDC<Command> command_being_processed;
typedef AIWriteAccess<Command> command_being_processed_wat;
typedef AIReadAccess<Command> command_being_processed_rat;

namespace curlthread {
// All functions in this namespace are only run by the curl thread, unless they are marked with MAIN-THREAD.

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

#if !WINDOWS_CODE
	// Return the largest fd set in mFdSet by refresh.
	curl_socket_t get_max_fd(void) const { return mMaxFdSet; }
#endif

	// Return true if a filedescriptor is set in mFileDescriptors (used for debugging).
	bool contains(curl_socket_t s) const;

	// Return true if a filedescriptor is set in mFdSet.
	bool is_set(curl_socket_t s) const;

	// Clear filedescriptor in mFdSet.
	void clr(curl_socket_t fd);

	// Iterate over all file descriptors that were set by refresh and are still set in mFdSet.
	void reset(void);				// Reset the iterator.
	curl_socket_t get(void) const;	// Return next filedescriptor, or CURL_SOCKET_BAD when there are no more.
	                   				// Only valid if reset() was called after the last call to refresh().
	void next(void);				// Advance to next filedescriptor.

  private:
	curl_socket_t* mFileDescriptors;
	int mNrFds;						// The number of filedescriptors in the array.
	int mNext;						// The index of the first file descriptor to start copying, the next call to refresh().

	fd_set mFdSet;					// Output variable for select(). (Re)initialized by calling refresh().

#if !WINDOWS_CODE
	curl_socket_t mMaxFd;			// The largest filedescriptor in the array, or CURL_SOCKET_BAD when it is empty.
	curl_socket_t mMaxFdSet;		// The largest filedescriptor set in mFdSet by refresh(), or CURL_SOCKET_BAD when it was empty.
	std::vector<curl_socket_t> mCopiedFileDescriptors;	// Filedescriptors copied by refresh to mFdSet.
	std::vector<curl_socket_t>::iterator mIter;			// Index into mCopiedFileDescriptors for next(); loop variable.
#else
	unsigned int mIter;				// Index into fd_set::fd_array.
#endif
};

// A PollSet can store at least 1024 filedescriptors, or FD_SETSIZE if that is larger than 1024 [MAXSIZE].
// The number of stored filedescriptors is mNrFds [0 <= mNrFds <= MAXSIZE].
// The largest filedescriptor is stored is mMaxFd, which is -1 iff mNrFds == 0.
// The file descriptors are stored contiguous in mFileDescriptors[i], with 0 <= i < mNrFds.
// File descriptors with the highest priority should be stored first (low index).
//
// mNext is an index into mFileDescriptors that is copied first, the next call to refresh().
// It is set to 0 when mNrFds < FD_SETSIZE, even if mNrFds == 0.
//
// After a call to refresh():
//
// mFdSet has bits set for at most FD_SETSIZE - 1 filedescriptors, copied from mFileDescriptors starting
// at index mNext (wrapping around to 0). If mNrFds < FD_SETSIZE then mNext is reset to 0 before copying starts.
// If mNrFds >= FD_SETSIZE then mNext is set to the next filedescriptor that was not copied (otherwise it is left at 0).
//
// mMaxFdSet is the largest filedescriptor in mFdSet or -1 if it is empty.

static size_t const MAXSIZE = llmax(1024, FD_SETSIZE);

// Create an empty PollSet.
PollSet::PollSet(void) : mFileDescriptors(new curl_socket_t [MAXSIZE]),
                         mNrFds(0), mNext(0)
#if !WINDOWS_CODE
						 , mMaxFd(-1), mMaxFdSet(-1)
#endif
{
  FD_ZERO(&mFdSet);
}

// Add filedescriptor s to the PollSet.
void PollSet::add(curl_socket_t s)
{
  llassert_always(mNrFds < (int)MAXSIZE);
  mFileDescriptors[mNrFds++] = s;
#if !WINDOWS_CODE
  mMaxFd = llmax(mMaxFd, s);
#endif
}

// Remove filedescriptor s from the PollSet.
void PollSet::remove(curl_socket_t s)
{
  // The number of open filedescriptors is relatively small,
  // and on top of that we rather do something CPU intensive
  // than bandwidth intensive (lookup table). Hence that this
  // is a linear search in an array containing just the open
  // filedescriptors. Then, since we are reading this memory
  // page anyway, we might as well write to it without losing
  // much clock cycles. Therefore, shift the whole vector
  // back, keeping it compact and keeping the filedescriptors
  // in the same order (which is supposedly their priority).
  //
  // The general case is where mFileDescriptors contains s at an index
  // between 0 and mNrFds:
  //                              mNrFds = 6
  //                                v
  // index: 0   1   2   3   4   5
  //        a   b   c   s   d   e

  // This function should never be called unless s is actually in mFileDescriptors,
  // as a result of a previous call to PollSet::add().
  llassert(mNrFds > 0);

  // Correct mNrFds for when the descriptor is removed.
  // Make i 'point' to the last entry.
  int i = --mNrFds;
  //                       i = NrFds = 5
  //                            v
  // index: 0   1   2   3   4   5
  //        a   b   c   s   d   e
  curl_socket_t cur = mFileDescriptors[i];		// cur = 'e'
#if !WINDOWS_CODE
  curl_socket_t max = -1;
#endif
  while (cur != s)
  {
	llassert(i > 0);
	curl_socket_t next = mFileDescriptors[--i];	// next = 'd'
	mFileDescriptors[i] = cur;					// Overwrite 'd' with 'e'.
#if !WINDOWS_CODE
	max = llmax(max, cur);					// max is the maximum value in 'i' or higher.
#endif
	cur = next;									// cur = 'd'
	//                        i  NrFds = 5
	//                        v   v
	// index: 0   1   2   3   4
	//        a   b   c   s   e					// cur = 'd'
	//
	// Next loop iteration: next = 's', overwrite 's' with 'd', cur = 's'; loop terminates.
	//                    i      NrFds = 5
	//                    v       v
	// index: 0   1   2   3   4
	//        a   b   c   d   e					// cur = 's'
  }
  llassert(cur == s);
  // At this point i was decremented once more and points to the element before the old s.
  //                i          NrFds = 5
  //                v           v
  // index: 0   1   2   3   4
  //        a   b   c   d   e					// max = llmax('d', 'e')

  // If mNext pointed to an element before s, it should be left alone. Otherwise, if mNext pointed
  // to s it must now point to 'd', or if it pointed beyond 's' it must be decremented by 1.
  if (mNext > i)								// i is where s was.
	--mNext;

#if !WINDOWS_CODE
  // If s was the largest file descriptor, we have to update mMaxFd.
  if (s == mMaxFd)
  {
	while (i > 0)
	{
	  curl_socket_t next = mFileDescriptors[--i];
	  max = llmax(max, next);
	}
	mMaxFd = max;
	llassert(mMaxFd < s);
	llassert((mMaxFd == -1) == (mNrFds == 0));
  }
#endif

  // ALSO make sure that s is no longer set in mFdSet, or we might confuse libcurl by
  // calling curl_multi_socket_action for a socket that it told us to remove.
#if !WINDOWS_CODE
  clr(s);
#else
  // We have to use a custom implementation here, because we don't want to invalidate mIter.
  // This is the same algorithm as above, but with mFdSet.fd_count instead of mNrFds,
  // mFdSet.fd_array instead of mFileDescriptors and mIter instead of mNext.
  if (FD_ISSET(s, &mFdSet))
  {
	llassert(mFdSet.fd_count > 0);
	unsigned int i = --mFdSet.fd_count;
	curl_socket_t cur = mFdSet.fd_array[i];
	while (cur != s)
	{
	  llassert(i > 0);
	  curl_socket_t next = mFdSet.fd_array[--i];
	  mFdSet.fd_array[i] = cur;
	  cur = next;
	}
	if (mIter > i)
	  --mIter;
	llassert(mIter <= mFdSet.fd_count);
  }
#endif
}

bool PollSet::contains(curl_socket_t fd) const
{
  for (int i = 0; i < mNrFds; ++i)
	if (mFileDescriptors[i] == fd)
	  return true;
  return false;
}

inline bool PollSet::is_set(curl_socket_t fd) const
{
  return FD_ISSET(fd, &mFdSet);
}

inline void PollSet::clr(curl_socket_t fd)
{
  FD_CLR(fd, &mFdSet);
}

// This function fills mFdSet with at most FD_SETSIZE - 1 filedescriptors,
// starting at index mNext (updating mNext when not all could be added),
// and updates mMaxFdSet to be the largest fd added to mFdSet, or -1 if it's empty.
refresh_t PollSet::refresh(void)
{
  FD_ZERO(&mFdSet);
#if !WINDOWS_CODE
  mCopiedFileDescriptors.clear();
#endif

  if (mNrFds == 0)
  {
#if !WINDOWS_CODE
	mMaxFdSet = -1;
#endif
	return empty_and_complete;
  }

  llassert_always(mNext < mNrFds);

  // Test if mNrFds is larger than or equal to FD_SETSIZE; equal, because we reserve one
  // filedescriptor for the wakeup fd: we copy maximal FD_SETSIZE - 1 filedescriptors.
  // If not then we're going to copy everything so that we can save on CPU cycles
  // by not calculating mMaxFdSet here.
  if (mNrFds >= FD_SETSIZE)
  {
	llwarns << "PollSet::reset: More than FD_SETSIZE (" << FD_SETSIZE << ") file descriptors active!" << llendl;
#if !WINDOWS_CODE
	// Calculate mMaxFdSet.
	// Run over FD_SETSIZE - 1 elements, starting at mNext, wrapping to 0 when we reach the end.
	int max = -1, i = mNext, count = 0;
	while (++count < FD_SETSIZE) { max = llmax(max, mFileDescriptors[i]); if (++i == mNrFds) i = 0; }
	mMaxFdSet = max;
#endif
  }
  else
  {
	mNext = 0;				// Start at the beginning if we copy everything anyway.
#if !WINDOWS_CODE
	mMaxFdSet = mMaxFd;
#endif
  }
  int count = 0;
  int i = mNext;
  for(;;)
  {
	if (++count == FD_SETSIZE)
	{
	  mNext = i;
	  return not_complete_not_empty;
	}
	FD_SET(mFileDescriptors[i], &mFdSet);
#if !WINDOWS_CODE
	mCopiedFileDescriptors.push_back(mFileDescriptors[i]);
#endif
	if (++i == mNrFds)
	{
	  // If we reached the end and start at the beginning, then we copied everything.
	  if (mNext == 0)
		break;
	  // When can only come here if mNrFds >= FD_SETSIZE, hence we can just
	  // wrap around and terminate on count reaching FD_SETSIZE.
	  i = 0;
	}
  }
  return complete_not_empty;
}

// The API reset(), get() and next() allows one to run over all filedescriptors
// in mFdSet that are set. This works by running only over the filedescriptors
// that were set initially (by the call to refresh()) and then checking if that
// filedescriptor is (still) set in mFdSet.
//
// A call to reset() resets mIter to the beginning, so that get() returns
// the first filedescriptor that is still set. A call to next() progresses
// the iterator to the next set filedescriptor. If get() return -1, then there
// were no more filedescriptors set.
//
// Note that one should never call next() unless get() didn't return -1, so
// the call sequence is:
// refresh();
// /* reset some or all bits in mFdSet */
// reset();
// while (get() != CURL_SOCKET_BAD) // next();
//
// Note also that this API is only used by MergeIterator, which wraps it
// and provides a different API to use.

void PollSet::reset(void)
{
#if WINDOWS_CODE
  mIter = 0;
#else
  if (mCopiedFileDescriptors.empty())
	mIter = mCopiedFileDescriptors.end();
  else
  {
	mIter = mCopiedFileDescriptors.begin();
	if (!FD_ISSET(*mIter, &mFdSet))
	  next();
  }
#endif
}

inline curl_socket_t PollSet::get(void) const
{
#if WINDOWS_CODE
  return (mIter >= mFdSet.fd_count) ? CURL_SOCKET_BAD : mFdSet.fd_array[mIter];
#else
  return (mIter == mCopiedFileDescriptors.end()) ? CURL_SOCKET_BAD : *mIter;
#endif
}

void PollSet::next(void)
{
#if WINDOWS_CODE
  llassert(mIter < mFdSet.fd_count);
  ++mIter;
#else
  llassert(mIter != mCopiedFileDescriptors.end());	// Only call next() if the last call to get() didn't return -1.
  while (++mIter != mCopiedFileDescriptors.end() && !FD_ISSET(*mIter, &mFdSet));
#endif
}

//-----------------------------------------------------------------------------
// MergeIterator
//
// This class takes two PollSet's and allows one to run over all filedescriptors
// that are set in one or both poll sets, returning each filedescriptor only
// once, by calling next() until it returns false.

class MergeIterator
{
  public:
	MergeIterator(PollSet* readPollSet, PollSet* writePollSet);

	bool next(curl_socket_t& fd_out, int& ev_bitmask_out);

  private:
	PollSet* mReadPollSet;
	PollSet* mWritePollSet;
	int readIndx;
	int writeIndx;
};

MergeIterator::MergeIterator(PollSet* readPollSet, PollSet* writePollSet) :
    mReadPollSet(readPollSet), mWritePollSet(writePollSet), readIndx(0), writeIndx(0)
{
  mReadPollSet->reset();
  mWritePollSet->reset();
}

bool MergeIterator::next(curl_socket_t& fd_out, int& ev_bitmask_out)
{
  curl_socket_t rfd = mReadPollSet->get();
  curl_socket_t wfd = mWritePollSet->get();

  if (rfd == CURL_SOCKET_BAD && wfd == CURL_SOCKET_BAD)
	return false;

  if (rfd == wfd)
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN | CURL_CSELECT_OUT;
	mReadPollSet->next();
  }
  else if (wfd == CURL_SOCKET_BAD || (rfd != CURL_SOCKET_BAD && rfd < wfd))	// Use and increment smaller one, unless it's CURL_SOCKET_BAD.
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN;
	mReadPollSet->next();
	if (wfd != CURL_SOCKET_BAD && mWritePollSet->is_set(rfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_OUT;
	  mWritePollSet->clr(rfd);
	}
  }
  else
  {
	fd_out = wfd;
	ev_bitmask_out = CURL_CSELECT_OUT;
	mWritePollSet->next();
	if (rfd != CURL_SOCKET_BAD && mReadPollSet->is_set(wfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_IN;
	  mReadPollSet->clr(wfd);
	}
  }

  return true;
}

//-----------------------------------------------------------------------------
// CurlSocketInfo

// A class with info for each socket that is in use by curl.
class CurlSocketInfo
{
  public:
	CurlSocketInfo(MultiHandle& multi_handle, CURL* easy, curl_socket_t s, int action);
	~CurlSocketInfo();

	void set_action(int action);

  private:
	MultiHandle& mMultiHandle;
	CURL const* mEasy;
	curl_socket_t mSocketFd;
	int mAction;
};

CurlSocketInfo::CurlSocketInfo(MultiHandle& multi_handle, CURL* easy, curl_socket_t s, int action) :
    mMultiHandle(multi_handle), mEasy(easy), mSocketFd(s), mAction(CURL_POLL_NONE)
{
  mMultiHandle.assign(s, this);
  llassert(!mMultiHandle.mReadPollSet->contains(s));
  llassert(!mMultiHandle.mWritePollSet->contains(s));
  set_action(action);
}

CurlSocketInfo::~CurlSocketInfo()
{
  set_action(CURL_POLL_NONE);
}

void CurlSocketInfo::set_action(int action)
{
  int toggle_action = mAction ^ action; 
  mAction = action;
  if ((toggle_action & CURL_POLL_IN))
  {
	if ((action & CURL_POLL_IN))
	  mMultiHandle.mReadPollSet->add(mSocketFd);
	else
	  mMultiHandle.mReadPollSet->remove(mSocketFd);
  }
  if ((toggle_action & CURL_POLL_OUT))
  {
	if ((action & CURL_POLL_OUT))
	  mMultiHandle.mWritePollSet->add(mSocketFd);
	else
	  mMultiHandle.mWritePollSet->remove(mSocketFd);
  }
}

//-----------------------------------------------------------------------------
// AICurlThread

class AICurlThread : public LLThread
{
  public:
	static AICurlThread* sInstance;
	LLMutex mWakeUpMutex;
	bool mWakeUpFlag;			// Protected by mWakeUpMutex.

  public:
	// MAIN-THREAD
	AICurlThread(void);
	virtual ~AICurlThread();

	// MAIN-THREAD
	void wakeup_thread(void);

	// MAIN-THREAD
	void stop_thread(void) { mRunning = false; wakeup_thread(); }

  protected:
	virtual void run(void);
	void wakeup(AICurlMultiHandle_wat const& multi_handle_w);
	void process_commands(AICurlMultiHandle_wat const& multi_handle_w);

  private:
	// MAIN-THREAD
	void create_wakeup_fds(void);
	void cleanup_wakeup_fds(void);

	curl_socket_t mWakeUpFd_in;
	curl_socket_t mWakeUpFd;

	int mZeroTimeOut;

	volatile bool mRunning;
};

// Only the main thread is accessing this.
AICurlThread* AICurlThread::sInstance = NULL;

// MAIN-THREAD
AICurlThread::AICurlThread(void) : LLThread("AICurlThread"),
    mWakeUpFd_in(CURL_SOCKET_BAD),
	mWakeUpFd(CURL_SOCKET_BAD),
	mZeroTimeOut(0), mRunning(true), mWakeUpFlag(false)
{
  create_wakeup_fds();
  sInstance = this;
}

// MAIN-THREAD
AICurlThread::~AICurlThread()
{
  sInstance = NULL;
  cleanup_wakeup_fds();
}

#if LL_WINDOWS
static std::string formatWSAError()
{
	std::ostringstream r;
	int e = WSAGetLastError();
	LPTSTR error_str = 0;
	r << e;
	if(FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, e, 0, (LPTSTR)&error_str, 0, NULL))
	{
		r << " " << utf16str_to_utf8str(error_str);
		LocalFree(error_str);
	}
	else
	{
		r << " Unknown WinSock error";
	}
	return r.str();
}
#elif WINDOWS_CODE
static std::string formatWSAError()
{
	return strerror(errno);
}
#endif

#if LL_WINDOWS
 /* Copyright 2007, 2010 by Nathan C. Myers <ncm@cantrip.org>
 * This code is Free Software.  It may be copied freely, in original or 
 * modified form, subject only to the restrictions that (1) the author is
 * relieved from all responsibilities for any use for any purpose, and (2)
 * this copyright notice must be retained, unchanged, in its entirety.  If
 * for any reason the author might be held responsible for any consequences
 * of copying or use, license is withheld.  
 */
static int dumb_socketpair(SOCKET socks[2], bool make_overlapped)
{
    union {
       struct sockaddr_in inaddr;
       struct sockaddr addr;
    } a;
    SOCKET listener;
    int e;
    socklen_t addrlen = sizeof(a.inaddr);
    DWORD flags = (make_overlapped ? WSA_FLAG_OVERLAPPED : 0);
    int reuse = 1;

    if (socks == 0) {
      WSASetLastError(WSAEINVAL);
      return SOCKET_ERROR;
    }

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) 
        return SOCKET_ERROR;

    memset(&a, 0, sizeof(a));
    a.inaddr.sin_family = AF_INET;
    a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.inaddr.sin_port = 0; 

    socks[0] = socks[1] = INVALID_SOCKET;
    do {
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, 
               (char*) &reuse, (socklen_t) sizeof(reuse)) == -1)
            break;
        if  (bind(listener, &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
            break;

        memset(&a, 0, sizeof(a));
        if  (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
            break;
        // win32 getsockname may only set the port number, p=0.0005.
        // ( http://msdn.microsoft.com/library/ms738543.aspx ):
        a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.inaddr.sin_family = AF_INET;

        if (listen(listener, 1) == SOCKET_ERROR)
            break;

        socks[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, flags);
        if (socks[0] == INVALID_SOCKET)
            break;
        if (connect(socks[0], &a.addr, sizeof(a.inaddr)) == SOCKET_ERROR)
            break;

        socks[1] = accept(listener, NULL, NULL);
        if (socks[1] == INVALID_SOCKET)
            break;

        closesocket(listener);
        return 0;

    } while (0);

    e = WSAGetLastError();
    closesocket(listener);
    closesocket(socks[0]);
    closesocket(socks[1]);
    WSASetLastError(e);
    return SOCKET_ERROR;
}
#elif WINDOWS_CODE
int dumb_socketpair(int socks[2], int dummy)
{
    (void) dummy;
    return socketpair(AF_LOCAL, SOCK_STREAM, 0, socks);
}
#endif

// MAIN-THREAD
void AICurlThread::create_wakeup_fds(void)
{
#if WINDOWS_CODE
	//SGTODO
	curl_socket_t socks[2];
	if (dumb_socketpair(socks, false) == SOCKET_ERROR)
	{
		llerrs << "Failed to generate wake-up socket pair" << formatWSAError() << llendl;
		return;
	}
	u_long nonblocking_enable = TRUE;
	int error = ioctlsocket(socks[0], FIONBIO, &nonblocking_enable);
	if(error)
	{
		llerrs << "Failed to set wake-up socket nonblocking: " << formatWSAError() << llendl;
	}
	llassert(nonblocking_enable);
	error = ioctlsocket(socks[1], FIONBIO, &nonblocking_enable);
	if(error)
	{
		llerrs << "Failed to set wake-up input socket nonblocking: " << formatWSAError() << llendl;
	}
	mWakeUpFd = socks[0];
	mWakeUpFd_in = socks[1];
#else
  int pipefd[2];
  if (pipe(pipefd))
  {
	llerrs << "Failed to create wakeup pipe: " << strerror(errno) << llendl;
  }
  int const flags = O_NONBLOCK;
  for (int i = 0; i < 2; ++i)
  {
	if (fcntl(pipefd[i], F_SETFL, flags))
	{
	  llerrs << "Failed to set pipe to non-blocking: " << strerror(errno) << llendl;
	}
  }
  mWakeUpFd = pipefd[0];		// Read-end of the pipe.
  mWakeUpFd_in = pipefd[1];		// Write-end of the pipe.
#endif
}

// MAIN-THREAD
void AICurlThread::cleanup_wakeup_fds(void)
{
#if WINDOWS_CODE
	//SGTODO
	if (mWakeUpFd != CURL_SOCKET_BAD)
	{
		int error = closesocket(mWakeUpFd);
		if (error)
		{
			llwarns << "Error closing wake-up socket" << formatWSAError() << llendl;
		}
	}
	if (mWakeUpFd_in != CURL_SOCKET_BAD)
	{
		int error = closesocket(mWakeUpFd_in);
		if (error)
		{
			llwarns << "Error closing wake-up input socket" << formatWSAError() << llendl;
		}
	}
#else
  if (mWakeUpFd_in != CURL_SOCKET_BAD)
	close(mWakeUpFd_in);
  if (mWakeUpFd != CURL_SOCKET_BAD)
	close(mWakeUpFd);
#endif
}

// MAIN-THREAD
void AICurlThread::wakeup_thread(void)
{
  DoutEntering(dc::curl, "AICurlThread::wakeup_thread");
  llassert(is_main_thread());

  // Try if curl thread is still awake and if so, pass the new commands directly.
  if (mWakeUpMutex.tryLock())
  {
	mWakeUpFlag = true;
	mWakeUpMutex.unlock();
	return;
  }

#if WINDOWS_CODE
  //SGTODO
  int len = send(mWakeUpFd_in, "!", 1, 0);
  if (len == SOCKET_ERROR)
  {
	  llerrs << "Send to wake-up socket failed: " << formatWSAError() << llendl;
  }
  llassert_always(len == 1);
  //SGTODO: handle EAGAIN if needed
#else
  // If write() is interrupted by a signal before it writes any data, it shall return -1 with errno set to [EINTR].
  // If write() is interrupted by a signal after it successfully writes some data, it shall return the number of bytes written.
  // Write requests to a pipe or FIFO shall be handled in the same way as a regular file with the following exceptions:
  // If the O_NONBLOCK flag is set, write() requests shall be handled differently, in the following ways:
  //   A write request for {PIPE_BUF} or fewer bytes shall have the following effect:
  //     if there is sufficient space available in the pipe, write() shall transfer all the data and return the number
  //     of bytes requested. Otherwise, write() shall transfer no data and return -1 with errno set to [EAGAIN].
  ssize_t len;
  do
  {
    len = write(mWakeUpFd_in, "!", 1);
    if (len == -1 && errno == EAGAIN)
	  return;		// Unread characters are still in the pipe, so no need to add more.
  }
  while(len == -1 && errno == EINTR);
  if (len == -1)
  {
	llerrs << "write(3) to mWakeUpFd_in: " << strerror(errno) << llendl;
  }
  llassert_always(len == 1);
#endif
}

void AICurlThread::wakeup(AICurlMultiHandle_wat const& multi_handle_w)
{
  DoutEntering(dc::curl, "AICurlThread::wakeup");

#if WINDOWS_CODE
  //SGTODO
  char buf[256];
  bool got_data = false;
  for(;;)
  {
	int len = recv(mWakeUpFd, buf, sizeof(buf), 0);
	if (len > 0)
	{
	  // Data was read from the pipe.
	  got_data = true;
	  if (len < sizeof(buf))
		break;
	}
	else if (len == SOCKET_ERROR)
	{
	  // An error occurred.
	  if (errno == EWOULDBLOCK)
	  {
		if (got_data)
		  break;
		// There was no data, even though select() said so. If this ever happens at all(?), lets just return and enter select() again.
		return;
	  }
	  else if (errno == EINTR)
	  {
		continue;
	  }
	  else
	  {
		llerrs << "read(3) from mWakeUpFd: " << formatWSAError() << llendl;
		return;
	  }
	}
	else
	{
	  // pipe(2) returned 0.
	  llwarns << "read(3) from mWakeUpFd returned 0, indicating that the pipe on the other end was closed! Shutting down curl thread." << llendl;
	  closesocket(mWakeUpFd);
	  mWakeUpFd = CURL_SOCKET_BAD;
	  mRunning = false;
	  return;
	}
  }
#else
  // If a read() is interrupted by a signal before it reads any data, it shall return -1 with errno set to [EINTR].
  // If a read() is interrupted by a signal after it has successfully read some data, it shall return the number of bytes read.
  // When attempting to read from an empty pipe or FIFO:
  // If no process has the pipe open for writing, read() shall return 0 to indicate end-of-file.
  // If some process has the pipe open for writing and O_NONBLOCK is set, read() shall return -1 and set errno to [EAGAIN].
  char buf[256];
  bool got_data = false;
  for(;;)
  {
	ssize_t len = read(mWakeUpFd, buf, sizeof(buf));
	if (len > 0)
	{
	  // Data was read from the pipe.
	  got_data = true;
	  if (len < sizeof(buf))
		break;
	}
	else if (len == -1)
	{
	  // An error occurred.
	  if (errno == EAGAIN)
	  {
		if (got_data)
		  break;
		// There was no data, even though select() said so. If this ever happens at all(?), lets just return and enter select() again.
		return;
	  }
	  else if (errno == EINTR)
	  {
		continue;
	  }
	  else
	  {
		llerrs << "read(3) from mWakeUpFd: " << strerror(errno) << llendl;
		return;
	  }
	}
	else
	{
	  // pipe(2) returned 0.
	  llwarns << "read(3) from mWakeUpFd returned 0, indicating that the pipe on the other end was closed! Shutting down curl thread." << llendl;
	  close(mWakeUpFd);
	  mWakeUpFd = CURL_SOCKET_BAD;
	  mRunning = false;
	  return;
	}
  }
#endif
  // Data was received on mWakeUpFd. This means that the main-thread added one
  // or more commands to the command queue and called wakeUpCurlThread().
  process_commands(multi_handle_w);
}

void AICurlThread::process_commands(AICurlMultiHandle_wat const& multi_handle_w)
{
  DoutEntering(dc::curl, "AICurlThread::process_commands(void)");

  // If we get here then the main thread called wakeup_thread() recently.
  for(;;)
  {
	// Access command_queue, and move command to command_being_processed.
	{
	  command_queue_wat command_queue_w(command_queue);
	  if (command_queue_w->empty())
	  {
		mWakeUpMutex.lock();
		mWakeUpFlag = false;
		mWakeUpMutex.unlock();
		break;
	  }
	  // Move the next command from the queue into command_being_processed.
	  *command_being_processed_wat(command_being_processed) = command_queue_w->front();
	  command_queue_w->pop_front();
	}
	// Access command_being_processed only.
	{
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  switch(command_being_processed_r->command())
	  {
		case cmd_none:
		case cmd_boost:	// FIXME: future stuff
		  break;
		case cmd_add:
		  multi_handle_w->add_easy_request(AICurlEasyRequest(command_being_processed_r->easy_request()));
		  break;
		case cmd_remove:
		  multi_handle_w->remove_easy_request(AICurlEasyRequest(command_being_processed_r->easy_request()), true);
		  break;
	  }
	  // Done processing.
	  command_being_processed_wat command_being_processed_w(command_being_processed_r);
	  command_being_processed_w->reset();		// This destroys the CurlEasyRequest in case of a cmd_remove.
	}
  }
}

// The main loop of the curl thread.
void AICurlThread::run(void)
{
  DoutEntering(dc::curl, "AICurlThread::run()");

  {
	AICurlMultiHandle_wat multi_handle_w(AICurlMultiHandle::getInstance());
	while(mRunning)
	{
	  // If mRunning is true then we can only get here if mWakeUpFd != CURL_SOCKET_BAD.
	  llassert(mWakeUpFd != CURL_SOCKET_BAD);
	  // Copy the next batch of file descriptors from the PollSets mFiledescriptors into their mFdSet.
	  multi_handle_w->mReadPollSet->refresh();
	  refresh_t wres = multi_handle_w->mWritePollSet->refresh();
	  // Add wake up fd if any, and pass NULL to select() if a set is empty.
	  fd_set* read_fd_set = multi_handle_w->mReadPollSet->access();
	  FD_SET(mWakeUpFd, read_fd_set);
	  fd_set* write_fd_set = ((wres & empty)) ? NULL : multi_handle_w->mWritePollSet->access();
	  // Calculate nfds (ignored on windows).
#if !WINDOWS_CODE
	  curl_socket_t const max_rfd = llmax(multi_handle_w->mReadPollSet->get_max_fd(), mWakeUpFd);
	  curl_socket_t const max_wfd = multi_handle_w->mWritePollSet->get_max_fd();
	  int nfds = llmax(max_rfd, max_wfd) + 1;
	  llassert(0 <= nfds && nfds <= FD_SETSIZE);
	  llassert((max_rfd == -1) == (read_fd_set == NULL) &&
			   (max_wfd == -1) == (write_fd_set == NULL));	// Needed on Windows.
	  llassert((max_rfd == -1 || multi_handle_w->mReadPollSet->is_set(max_rfd)) &&
			   (max_wfd == -1 || multi_handle_w->mWritePollSet->is_set(max_wfd)));
#else
	  int nfds = 64;
#endif
	  int ready = 0;
	  // Process every command in command_queue before entering select().
	  for(;;)
	  {
		mWakeUpMutex.lock();
		if (mWakeUpFlag)
		{
		  mWakeUpMutex.unlock();
		  process_commands(multi_handle_w);
		  continue;
		}
		break;
	  }
	  // wakeup_thread() is also called after setting mRunning to false.
	  if (!mRunning)
	  {
		mWakeUpMutex.unlock();
		break;
	  }
	  // If we get here then mWakeUpFlag has been false since we grabbed the lock.
	  // We're now entering select(), during which the main thread will write to the pipe/socket
	  // to wake us up, because it can't get the lock.
	  struct timeval timeout;
	  long timeout_ms = multi_handle_w->getTimeOut();
	  // If no timeout is set, sleep 1 second.
	  if (LL_UNLIKELY(timeout_ms < 0))
		timeout_ms = 1000;
	  if (LL_UNLIKELY(timeout_ms == 0))
	  {
		if (mZeroTimeOut >= 10000)
		{
		  if (mZeroTimeOut == 10000)
			llwarns << "Detected more than 10000 zero-timeout calls of select() by curl thread (more than 101 seconds)!" << llendl;
		}
		else if (mZeroTimeOut >= 1000)
		  timeout_ms = 10;
		else if (mZeroTimeOut >= 100)
		  timeout_ms = 1;
	  }
	  else
	  {
		if (LL_UNLIKELY(mZeroTimeOut >= 10000))
		  llinfos << "Timeout of select() call by curl thread reset (to " << timeout_ms << " ms)." << llendl;
		mZeroTimeOut = 0;
	  }
	  timeout.tv_sec = timeout_ms / 1000;
	  timeout.tv_usec = (timeout_ms % 1000) * 1000;
#ifdef CWDEBUG
	  static int last_nfds = -1;
	  static long last_timeout_ms = -1;
	  static int same_count = 0;
	  bool same = (nfds == last_nfds && timeout_ms == last_timeout_ms);
	  if (!same)
	  {
		if (same_count > 1)
		  Dout(dc::curl, "Last select() call repeated " << same_count << " times.");
		Dout(dc::curl|flush_cf|continued_cf, "select(" << nfds << ", ..., timeout = " << timeout_ms << " ms) = ");
		same_count = 1;
	  }
	  else
	  {
		++same_count;
	  }
#endif
	  ready = select(nfds, read_fd_set, write_fd_set, NULL, &timeout);
	  mWakeUpMutex.unlock();
#ifdef CWDEBUG
	  static int last_ready = -2;
	  static int last_errno = 0;
	  if (!same)
		Dout(dc::finish|cond_error_cf(ready == -1), ready);
	  else if (ready != last_ready || (ready == -1 && errno != last_errno))
	  {
		if (same_count > 1)
		  Dout(dc::curl, "Last select() call repeated " << same_count << " times.");
		Dout(dc::curl|cond_error_cf(ready == -1), "select(" << last_nfds << ", ..., timeout = " << last_timeout_ms << " ms) = " << ready);
		same_count = 1;
	  }
	  last_nfds = nfds;
	  last_timeout_ms = timeout_ms;
	  last_ready = ready;
	  if (ready == -1)
		last_errno = errno;
#endif
	  // Select returns the total number of bits set in each of the fd_set's (upon return),
	  // or -1 when an error occurred. A value of 0 means that a timeout occurred.
	  if (ready == -1)
	  {
		llwarns << "select() failed: " << errno << ", " << strerror(errno) << llendl;
		continue;
	  }
	  else if (ready == 0)
	  {
		multi_handle_w->socket_action(CURL_SOCKET_TIMEOUT, 0);
	  }
	  else
	  {
		if (multi_handle_w->mReadPollSet->is_set(mWakeUpFd))
		{
		  // Process commands from main-thread. This can add or remove filedescriptors from the poll sets.
		  wakeup(multi_handle_w);
		  --ready;
		}
		// Handle all active filedescriptors.
		MergeIterator iter(multi_handle_w->mReadPollSet, multi_handle_w->mWritePollSet);
		curl_socket_t fd;
		int ev_bitmask;
		while (ready > 0 && iter.next(fd, ev_bitmask))
		{
		  ready -= (ev_bitmask == (CURL_CSELECT_IN|CURL_CSELECT_OUT)) ? 2 : 1;
		  // This can cause libcurl to do callbacks and remove filedescriptors, causing us to reset their bits in the poll sets.
		  multi_handle_w->socket_action(fd, ev_bitmask);
		  llassert(ready >= 0);
		}
		// Note that ready is not necessarily 0 here, because it's possible
		// that libcurl removed file descriptors which we subsequently
		// didn't handle.
	  }
	  multi_handle_w->check_run_count();
	}
  }
  AICurlMultiHandle::destroyInstance();
}

//-----------------------------------------------------------------------------
// MultiHandle

MultiHandle::MultiHandle(void) : mHandleAddedOrRemoved(false), mPrevRunningHandles(0), mRunningHandles(0), mTimeOut(-1), mReadPollSet(NULL), mWritePollSet(NULL)
{
  mReadPollSet = new PollSet;
  mWritePollSet = new PollSet;
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETFUNCTION, &MultiHandle::socket_callback));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETDATA, this));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERFUNCTION, &MultiHandle::timer_callback));
  check_multi_code(curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERDATA, this));
}

MultiHandle::~MultiHandle()
{
  llinfos << "Destructing MultiHandle with " << mAddedEasyRequests.size() << " active curl easy handles." << llendl;

  // This thread was terminated.
  // Curl demands that all handles are removed from the multi session handle before calling curl_multi_cleanup.
  for(addedEasyRequests_type::iterator iter = mAddedEasyRequests.begin(); iter != mAddedEasyRequests.end(); iter = mAddedEasyRequests.begin())
  {
	remove_easy_request(*iter);
  }
  delete mWritePollSet;
  delete mReadPollSet;
}

#if defined(CWDEBUG) || defined(DEBUG_CURLIO)
#undef AI_CASE_RETURN
#define AI_CASE_RETURN(x) do { case x: return #x; } while(0)
char const* action_str(int action)
{
  switch(action)
  {
	AI_CASE_RETURN(CURL_POLL_NONE);
	AI_CASE_RETURN(CURL_POLL_IN);
	AI_CASE_RETURN(CURL_POLL_OUT);
	AI_CASE_RETURN(CURL_POLL_INOUT);
	AI_CASE_RETURN(CURL_POLL_REMOVE);
  }
  return "<unknown action>";
}
#endif

//static
int MultiHandle::socket_callback(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp)
{
  DoutEntering(dc::curl, "MultiHandle::socket_callback(" << (void*)easy << ", " << s << ", " << action_str(action) << ", " << (void*)userp << ", " << (void*)socketp << ")");
  MultiHandle& self = *static_cast<MultiHandle*>(userp);
  CurlSocketInfo* sock_info = static_cast<CurlSocketInfo*>(socketp);
  if (action == CURL_POLL_REMOVE)
  {
	delete sock_info;
  }
  else
  {
	if (!sock_info)
	{
	  sock_info = new CurlSocketInfo(self, easy, s, action);
	}
	else
	{
	  sock_info->set_action(action);
	}
  }
  return 0;
}

//static
int MultiHandle::timer_callback(CURLM* multi, long timeout_ms, void* userp)
{
  MultiHandle& self = *static_cast<MultiHandle*>(userp);
  llassert(multi == self.mMultiHandle);
  self.mTimeOut = timeout_ms;
  Dout(dc::curl, "MultiHandle::timer_callback(): timeout set to " << timeout_ms << " ms.");
  return 0;
}

CURLMcode MultiHandle::socket_action(curl_socket_t sockfd, int ev_bitmask)
{
  CURLMcode res;
  do
  {
    res = check_multi_code(curl_multi_socket_action(mMultiHandle, sockfd, ev_bitmask, &mRunningHandles));
  }
  while(res == CURLM_CALL_MULTI_PERFORM);
  return res;
}

CURLMcode MultiHandle::assign(curl_socket_t sockfd, void* sockptr)
{
  return check_multi_code(curl_multi_assign(mMultiHandle, sockfd, sockptr));
}

CURLMsg const* MultiHandle::info_read(int* msgs_in_queue) const
{
  CURLMsg const* ret = curl_multi_info_read(mMultiHandle, msgs_in_queue);
  // NULL could be an error, but normally it isn't, so don't print anything and
  // never increment Stats::multi_errors. However, lets just increment multi_calls
  // when it certainly wasn't an error...
  if (ret)
	Stats::multi_calls++;
  return ret;
}

CURLMcode MultiHandle::add_easy_request(AICurlEasyRequest const& easy_request)
{
  std::pair<addedEasyRequests_type::iterator, bool> res = mAddedEasyRequests.insert(easy_request);
  llassert(res.second);							// May not have been added before.
  CURLMcode ret;
  {
	AICurlEasyRequest_wat curl_easy_request_w(*easy_request);
	ret = curl_easy_request_w->add_handle_to_multi(curl_easy_request_w, mMultiHandle);
  }
  mHandleAddedOrRemoved = true;
  Dout(dc::curl, "MultiHandle::add_easy_request: Added AICurlEasyRequest " << (void*)easy_request.get() << "; now processing " << mAddedEasyRequests.size() << " easy handles.");
  return ret;
}

CURLMcode MultiHandle::remove_easy_request(AICurlEasyRequest const& easy_request, bool as_per_command)
{
  addedEasyRequests_type::iterator iter = mAddedEasyRequests.find(easy_request);
  if (iter == mAddedEasyRequests.end())
	return (CURLMcode)-2;				// Was already removed before.
  CURLMcode res;
  {
	AICurlEasyRequest_wat curl_easy_request_w(**iter);
	res = curl_easy_request_w->remove_handle_from_multi(curl_easy_request_w, mMultiHandle);
#ifdef SHOW_ASSERT
	curl_easy_request_w->mRemovedPerCommand = as_per_command;
#endif
  }
  mAddedEasyRequests.erase(iter);
  mHandleAddedOrRemoved = true;
  Dout(dc::curl, "MultiHandle::remove_easy_request: Removed AICurlEasyRequest " << (void*)easy_request.get() << "; now processing " << mAddedEasyRequests.size() << " easy handles.");
  return res;
}

void MultiHandle::check_run_count(void)
{
  if (mHandleAddedOrRemoved || mRunningHandles < mPrevRunningHandles)
  {
	CURLMsg const* msg;
	int msgs_left;
	while ((msg = info_read(&msgs_left)))
	{
	  if (msg->msg == CURLMSG_DONE)
	  {
		CURL* easy = msg->easy_handle;
		ThreadSafeCurlEasyRequest* ptr;
		CURLcode rese = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
		llassert_always(rese == CURLE_OK);
		AICurlEasyRequest easy_request(ptr);
		llassert(*AICurlEasyRequest_wat(*easy_request) == easy);
		// Store the result and transfer info in the easy handle.
		{
		  AICurlEasyRequest_wat curl_easy_request_w(*easy_request);
		  curl_easy_request_w->store_result(msg->data.result);
#ifdef CWDEBUG
		  char* eff_url;
		  curl_easy_request_w->getinfo(CURLINFO_EFFECTIVE_URL, &eff_url);
		  Dout(dc::curl, "Finished: " << eff_url << " (" << msg->data.result << ")");
#endif
		  // Signal that this easy handle finished.
		  curl_easy_request_w->done(curl_easy_request_w);
		}
		// This invalidates msg, but not easy_request.
		CURLMcode res = remove_easy_request(easy_request);
		// This should hold, I think, because the handles are obviously ok and
		// the only error we could get is when remove_easy_request() was already
		// called before (by this thread); but if that was the case then the easy
		// handle should not have been be returned by info_read()...
		llassert(res == CURLM_OK);
		// Nevertheless, if it was already removed then just ignore it.
		if (res == CURLM_OK)
		{
		}
		else if (res == -2)
		{
		  llwarns << "Curl easy handle returned by curl_multi_info_read() that is not (anymore) in MultiHandle::mAddedEasyRequests!?!" << llendl;
		}
		// Destruction of easy_request at this point, causes the CurlEasyRequest to be deleted.
	  }
	}
	mHandleAddedOrRemoved = false;
  }
  mPrevRunningHandles = mRunningHandles;
}

} // namespace curlthread
} // namespace AICurlPrivate

//static
void AICurlMultiHandle::destroyInstance(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  Dout(dc::curl, "Destroying AICurlMultiHandle [" << (void*)tldata.mCurlMultiHandle << "] for thread \"" << tldata.mName << "\".");
  delete tldata.mCurlMultiHandle;
  tldata.mCurlMultiHandle = NULL;
}

//=============================================================================
// MAIN-THREAD (needing to access the above declarations).

//static
AICurlMultiHandle& AICurlMultiHandle::getInstance(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  if (!tldata.mCurlMultiHandle)
  {
	tldata.mCurlMultiHandle = new AICurlMultiHandle;
	Dout(dc::curl, "Created AICurlMultiHandle [" << (void*)tldata.mCurlMultiHandle << "] for thread \"" << tldata.mName << "\".");
  }
  return *static_cast<AICurlMultiHandle*>(tldata.mCurlMultiHandle);
}

namespace AICurlPrivate {

bool curlThreadIsRunning(void)
{
  using curlthread::AICurlThread;
  return AICurlThread::sInstance && !AICurlThread::sInstance->isStopped();
}

void wakeUpCurlThread(void)
{
  using curlthread::AICurlThread;
  if (AICurlThread::sInstance)
	AICurlThread::sInstance->wakeup_thread();
}

void stopCurlThread(void)
{
  using curlthread::AICurlThread;
  if (AICurlThread::sInstance)
  {
	AICurlThread::sInstance->stop_thread();
	int count = 101;
	while(--count && !AICurlThread::sInstance->isStopped())
	{
	  ms_sleep(10);
	}
	Dout(dc::curl, "Curl thread" << (curlThreadIsRunning() ? " not" : "") << " stopped after " << ((100 - count) * 10) << "ms.");
	// Clear the command queue, for a cleaner cleanup.
	command_queue_wat command_queue_w(command_queue);
	command_queue_w->clear();
  }
}

} // namespace AICurlPrivate

//-----------------------------------------------------------------------------
// AICurlEasyRequest

void AICurlEasyRequest::addRequest(void)
{
  using namespace AICurlPrivate;

  {
	// Write-lock the command queue.
	command_queue_wat command_queue_w(command_queue);
#ifdef SHOW_ASSERT
	// This debug code checks if we aren't calling addRequest() twice for the same object.
	// That means that the main thread already called (and finished, this is also the
	// main thread) this function, which also follows from that we just locked command_queue.
	// That leaves three options: It's still in the queue, or it was removed and is currently
	// processed by the curl thread with again two options: either it was already added
	// to the multi session handle or not yet.

	// Find the last command added.
	command_st cmd = cmd_none;
	for (std::deque<Command>::iterator iter = command_queue_w->begin(); iter != command_queue_w->end(); ++iter)
	{
	  if (*iter == *this)
	  {
		cmd = iter->command();
		break;
	  }
	}
	llassert(cmd == cmd_none || cmd == cmd_remove);	// Not in queue, or last command was to remove it.
	if (cmd == cmd_none)
	{
	  // Read-lock command_being_processed.
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  if (*command_being_processed_r == *this)
	  {
		// May not be in-between being removed from the command queue but not added to the multi session handle yet.
		llassert(command_being_processed_r->command() == cmd_remove);
	  }
	  else
	  {
		// May not already be added to the multi session handle.
		llassert(!AICurlEasyRequest_wat(*get())->active());
	  }
	}
#endif
	// Add a command to add the new request to the multi session to the command queue.
	command_queue_w->push_back(Command(*this, cmd_add));
	AICurlEasyRequest_wat(*get())->add_queued();
  }
  // Something was added to the queue, wake up the thread to get it.
  wakeUpCurlThread();
}

void AICurlEasyRequest::removeRequest(void)
{
  using namespace AICurlPrivate;

  {
	// Write-lock the command queue.
	command_queue_wat command_queue_w(command_queue);
#ifdef SHOW_ASSERT
	// This debug code checks if we aren't calling removeRequest() twice for the same object.
	// That means that the thread calling this function already finished it, following from that
	// we just locked command_queue.
	// That leaves three options: It's still in the queue, or it was removed and is currently
	// processed by the curl thread with again two options: either it was already removed
	// from the multi session handle or not yet.

	// Find the last command added.
	command_st cmd = cmd_none;
	for (std::deque<Command>::iterator iter = command_queue_w->begin(); iter != command_queue_w->end(); ++iter)
	{
	  if (*iter == *this)
	  {
		cmd = iter->command();
		break;
	  }
	}
	llassert(cmd == cmd_none || cmd != cmd_remove);	// Not in queue, or last command was not a remove command.
	if (cmd == cmd_none)
	{
	  // Read-lock command_being_processed.
	  command_being_processed_rat command_being_processed_r(command_being_processed);
	  if (*command_being_processed_r == *this)
	  {
		// May not be in-between being removed from the command queue but not removed from the multi session handle yet.
		llassert(command_being_processed_r->command() != cmd_remove);
	  }
	  else
	  {
		// May not already have been removed from multi session handle as per command from the main thread (through this function thus).
		{
		  AICurlEasyRequest_wat curl_easy_request_w(*get());
		  llassert(curl_easy_request_w->active() || !curl_easy_request_w->mRemovedPerCommand);
		}
	  }
	}
#endif
	// Add a command to remove this request from the multi session to the command queue.
	command_queue_w->push_back(Command(*this, cmd_remove));
	// Suppress warning that would otherwise happen if the callbacks are revoked before the curl thread removed the request.
	AICurlEasyRequest_wat(*get())->remove_queued();
  }
  // Something was added to the queue, wake up the thread to get it.
  wakeUpCurlThread();
}

//-----------------------------------------------------------------------------

namespace AICurlInterface {

void startCurlThread(void)
{
  using namespace AICurlPrivate::curlthread;

  llassert(is_main_thread());
  AICurlThread::sInstance = new AICurlThread;
  AICurlThread::sInstance->start();
}

} // namespace AICurlInterface

