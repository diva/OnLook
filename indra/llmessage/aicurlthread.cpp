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
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <deque>

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

// A PollSet can store at least 1024 file descriptors, or FD_SETSIZE if that is larger than 1024 [MAXSIZE].
// The number of stored file descriptors is mNrFds [0 <= mNrFds <= MAXSIZE].
// The largest file descriptor is stored is mMaxFd, which is -1 iff mNrFds == 0.
// The file descriptors are stored contiguous in mFileDescriptors[i], with 0 <= i < mNrFds.
// File descriptors with the highest priority should be stored first (low index).
//
// mNext is an index into mFileDescriptors that is copied first, the next call to refresh().
// It is set to 0 when mNrFds < FD_SETSIZE, even if mNrFds == 0.
//
// After a call to refresh():
//
// mFdSet has bits set for at most FD_SETSIZE - 1 file descriptors, copied from mFileDescriptors starting
// at index mNext (wrapping around to 0). If mNrFds < FD_SETSIZE then mNext is reset to 0 before copying starts.
// If mNrFds >= FD_SETSIZE then mNext is set to the next filedescriptor that was not copied (otherwise it is left at 0).
//
// mMaxFdSet is the largest filedescriptor in mFdSet or -1 if it is empty.

// Create an empty PollSet.
PollSet::PollSet(void) : mFileDescriptors(new curl_socket_t [std::max(1024, FD_SETSIZE)]), mSize(std::max(1024, FD_SETSIZE)),
                         mNrFds(0), mMaxFd(-1), mNext(0), mMaxFdSet(-1)
{
  FD_ZERO(&mFdSet);
}

// Add file descriptor s to the PollSet.
void PollSet::add(curl_socket_t s)
{
  llassert_always(mNrFds < mSize);
  mFileDescriptors[mNrFds++] = s;
  mMaxFd = std::max(mMaxFd, s);
}

// Remove file descriptor s from the PollSet.
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
  llassert(mNrFds > 0);
  int i = --mNrFds;
  int prev = mFileDescriptors[i];
  int max = -1;
  for (--i; i >= 0 && prev != s; --i)
  {
	int cur = mFileDescriptors[i];
	mFileDescriptors[i] = prev;
	max = std::max(max, prev);
	prev = cur;
  }
  if (s == mMaxFd)
  {
	while (i >= 0)
	{
	  int cur = mFileDescriptors[i];
	  max = std::max(max, cur);
	  --i;
	}
	mMaxFd = max;
	llassert(mMaxFd < s);
	llassert((mMaxFd == -1) == (mNrFds == 0));
  }
  if (mNext == mNrFds)
	mNext = 0;
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
  return FD_CLR(fd, &mFdSet);
}

// This function fills mFdSet with at most FD_SETSIZE - 1 filedescriptors,
// starting at index mNext (updating mNext when not all could be added),
// and updates mMaxFdSet to be the largest fd added to mFdSet, or -1 if it's empty.
refresh_t PollSet::refresh(void)
{
  FD_ZERO(&mFdSet);
  mCopiedFileDescriptors.clear();

  if (mNrFds == 0)
  {
	mMaxFdSet = -1;
	return empty_and_complete;
  }

  llassert_always(mNext < mNrFds);

  // Test if mNrFds is larger or equal FD_SETSIZE; equal, because we reserve one
  // file descriptor for the wakeup fd: we copy maximal FD_SETSIZE - 1 file descriptors.
  // If not then we're going to copy everything so that we can save on CPU cycles
  // by not calculating mMaxFdSet here.
  if (mNrFds >= FD_SETSIZE)
  {
	llwarns << "PollSet::reset: More than FD_SETSIZE (" << FD_SETSIZE << ") file descriptors active!" << llendl;
	// Calculate mMaxFdSet.
	int max = -1;
	int count = 0;
	int end = mNrFds;
	int i = mNext;
	while (++count < FD_SETSIZE)
	{
	  max = std::max(max, mFileDescriptors[i]);
	  if (++i == end)
	  {
		if (end == mNext)
		  break;
		end = mNext;
		i = 0;
		llassert(i < end);	// If mNext == 0 then the while loop terminates before we get here.
	  }
	}
	mMaxFdSet = max;
  }
  else
  {
	mNext = 0;				// Start at the beginning if we copy everything anyway.
	mMaxFdSet = mMaxFd;
  }
  int count = 0;
  int end = mNrFds;
  int i = mNext;
  for(;;)
  {
	if (++count == FD_SETSIZE)
	{
	  mNext = i;
	  return not_complete_not_empty;
	}
	FD_SET(mFileDescriptors[i], &mFdSet);
	mCopiedFileDescriptors.push_back(mFileDescriptors[i]);
	if (++i == end)
	{
	  if (mNext == 0)
		break;
	  // If this was true then we got here a second time, which means that we accessed all
	  // filedescriptors with mNext != 0, but count is still < FD_SETSIZE, which is not
	  // possible because mNext is only non-zero when mNrFds >= FD_SETSIZE.
	  llassert(end != mNext);
	  end = mNext;
	  i = 0;
	}
  }
  return complete_not_empty;
}

// FIXME: This needs a rewrite on windows, as FD_ISSET is slow there; it would make
// more sense to iterate directly over the fd's in mFdSet on windows.
void PollSet::reset(void)
{
  llassert((mNrFds == 0) == mCopiedFileDescriptors.empty());
  if (mCopiedFileDescriptors.empty())
	mIter = mCopiedFileDescriptors.end();
  else
  {
	mIter = mCopiedFileDescriptors.begin();
	if (!FD_ISSET(*mIter, &mFdSet))
	  next();
  }
}

inline int PollSet::get(void) const
{
  return (mIter == mCopiedFileDescriptors.end()) ? -1 : *mIter;
}

// FIXME: This needs a rewrite on windows, as FD_ISSET is slow there; it would make
// more sense to iterate directly over the fd's in mFdSet on windows.
void PollSet::next(void)
{
  llassert(mIter != mCopiedFileDescriptors.end());	// Only call next() if the last call to get() didn't return -1.
  while (++mIter != mCopiedFileDescriptors.end() && !FD_ISSET(*mIter, &mFdSet));
}

//-----------------------------------------------------------------------------
// MergeIterator

class MergeIterator
{
  public:
	MergeIterator(PollSet& readPollSet, PollSet& writePollSet);

	bool next(int& fd_out, int& ev_bitmask_out);

  private:
	PollSet& mReadPollSet;
	PollSet& mWritePollSet;
	int readIndx;
	int writeIndx;
};

MergeIterator::MergeIterator(PollSet& readPollSet, PollSet& writePollSet) :
    mReadPollSet(readPollSet), mWritePollSet(writePollSet), readIndx(0), writeIndx(0)
{
  mReadPollSet.reset();
  mWritePollSet.reset();
}

bool MergeIterator::next(int& fd_out, int& ev_bitmask_out)
{
  int rfd = mReadPollSet.get();
  int wfd = mWritePollSet.get();

  if (rfd == -1 && wfd == -1)
	return false;

  if (rfd == wfd)
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN | CURL_CSELECT_OUT;
	mReadPollSet.next();

  }
  else if ((unsigned int)rfd < (unsigned int)wfd)	// Use and increment smaller one, unless it's -1.
  {
	fd_out = rfd;
	ev_bitmask_out = CURL_CSELECT_IN;
	mReadPollSet.next();
	if (wfd != -1 && mWritePollSet.is_set(rfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_OUT;
	  mWritePollSet.clr(rfd);
	}
  }
  else
  {
	fd_out = wfd;
	ev_bitmask_out = CURL_CSELECT_OUT;
	mWritePollSet.next();
	if (rfd != -1 && mReadPollSet.is_set(wfd))
	{
	  ev_bitmask_out |= CURL_CSELECT_IN;
	  mReadPollSet.clr(wfd);
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
  llassert(!mMultiHandle.mReadPollSet.contains(s));
  llassert(!mMultiHandle.mWritePollSet.contains(s));
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
	  mMultiHandle.mReadPollSet.add(mSocketFd);
	else
	  mMultiHandle.mReadPollSet.remove(mSocketFd);
  }
  if ((toggle_action & CURL_POLL_OUT))
  {
	if ((action & CURL_POLL_OUT))
	  mMultiHandle.mWritePollSet.add(mSocketFd);
	else
	  mMultiHandle.mWritePollSet.remove(mSocketFd);
  }
}

//-----------------------------------------------------------------------------
// AICurlThread

class AICurlThread : public LLThread
{
  public:
	static AICurlThread* sInstance;

  public:
	// MAIN-THREAD
	AICurlThread(void);
	virtual ~AICurlThread();

	// MAIN-THREAD
	void wakeup_thread(void);

  protected:
	virtual void run(void);
	void wakeup(AICurlMultiHandle_wat const& multi_handle_w);

  private:
	// MAIN-THREAD
	void create_wakeup_fds(void);
	void cleanup_wakeup_fds(void);

	int mWakeUpFd_in;
	int mWakeUpFd;

	int mZeroTimeOut;
};

// Only the main thread is accessing this.
AICurlThread* AICurlThread::sInstance = NULL;

// MAIN-THREAD
AICurlThread::AICurlThread(void) : LLThread("AICurlThread"), mWakeUpFd_in(-1), mWakeUpFd(-1), mZeroTimeOut(0)
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

// MAIN-THREAD
void AICurlThread::create_wakeup_fds(void)
{
#ifdef WINDOWS
// Probably need to use sockets here, cause windows select doesn't work for a pipe.
#error Missing implementation
#else
  int pipefd[2];
  if (pipe(pipefd))
  {
	llerrs << "Failed to create wakeup pipe: " << strerror(errno) << llendl;
  }
  long flags = O_NONBLOCK;
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
  if (mWakeUpFd_in != -1)
	close(mWakeUpFd_in);
  if (mWakeUpFd != -1)
	close(mWakeUpFd);
}

// MAIN-THREAD
void AICurlThread::wakeup_thread(void)
{
  DoutEntering(dc::curl, "AICurlThread::wakeup_thread");

#ifdef WINDOWS
#error Missing implementation
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

#ifdef WINDOWS
#error Missing implementation
#else
  // If a read() is interrupted by a signal before it reads any data, it shall return -1 with errno set to [EINTR].
  // If a read() is interrupted by a signal after it has successfully read some data, it shall return the number of bytes read.
  // When attempting to read from an empty pipe or FIFO:
  // If no process has the pipe open for writing, read() shall return 0 to indicate end-of-file.
  // If some process has the pipe open for writing and O_NONBLOCK is set, read() shall return -1 and set errno to [EAGAIN].
  char buf[256];
  ssize_t len;
  do
  {
    len = read(mWakeUpFd, buf, sizeof(buf));
    if (len == -1 && errno == EAGAIN)
	  return;
  }
  while(len == -1 && errno == EINTR);
  if (len == -1)
  {
	llerrs << "read(3) from mWakeUpFd: " << strerror(errno) << llendl;
  }
  if (LL_UNLIKELY(len == 0))
  {
	llwarns << "read(3) from mWakeUpFd returned 0, indicating that the pipe on the other end was closed! Shutting down curl thread." << llendl;
	close(mWakeUpFd);
	mWakeUpFd = -1;
    shutdown();
	return;
  }
#endif
  // If we get here then the main thread called wakeup_thread() recently.
  for(;;)
  {
	// Access command_queue, and move command to command_being_processed.
	{
	  command_queue_wat command_queue_w(command_queue);
	  if (command_queue_w->empty())
		break;
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
		  multi_handle_w->remove_easy_request(AICurlEasyRequest(command_being_processed_r->easy_request()));
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

  AICurlMultiHandle_wat multi_handle_w(AICurlMultiHandle::getInstance());
  for(;;)
  {
	refresh_t rres = multi_handle_w->mReadPollSet.refresh();
	refresh_t wres = multi_handle_w->mWritePollSet.refresh();
	fd_set* read_fd_set = multi_handle_w->mReadPollSet.access();
	if (LL_LIKELY(mWakeUpFd != -1))
	  FD_SET(mWakeUpFd, read_fd_set);
	else if ((rres & empty))
	  read_fd_set = NULL;
	fd_set* write_fd_set = ((wres & empty)) ? NULL : multi_handle_w->mWritePollSet.access();
	int const max_rfd = std::max(multi_handle_w->mReadPollSet.get_max_fd(), mWakeUpFd);
	int const max_wfd = multi_handle_w->mWritePollSet.get_max_fd();
	int nfds = std::max(max_rfd, max_wfd) + 1;
	llassert(0 <= nfds && nfds <= FD_SETSIZE);
	llassert((max_rfd == -1) == (read_fd_set == NULL) &&
		     (max_wfd == -1) == (write_fd_set == NULL));	// Needed on windows.
	llassert((max_rfd == -1 || multi_handle_w->mReadPollSet.is_set(max_rfd)) &&
		     (max_wfd == -1 || multi_handle_w->mWritePollSet.is_set(max_wfd)));
	int ready = 0;
	if (LL_UNLIKELY(nfds == 0))	// Only happens when the thread is shutting down.
	  ms_sleep(1000);
	else
	{
	  struct timeval timeout;
	  long timeout_ms = multi_handle_w->getTimeOut();
	  // If no timeout is set, sleep 1 second.
	  if (timeout_ms < 0)
		timeout_ms = 1000;
	  if (timeout_ms == 0)
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
		if (mZeroTimeOut >= 10000)
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
	}
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
	  if (multi_handle_w->mReadPollSet.is_set(mWakeUpFd))
	  {
		wakeup(multi_handle_w);
		--ready;
	  }
	  MergeIterator iter(multi_handle_w->mReadPollSet, multi_handle_w->mWritePollSet);
	  int fd, ev_bitmask;
	  while (ready > 0 && iter.next(fd, ev_bitmask))
	  {
		ready -= (ev_bitmask == (CURL_CSELECT_IN|CURL_CSELECT_OUT)) ? 2 : 1;
		multi_handle_w->socket_action(fd, ev_bitmask);
		llassert(ready >= 0);
	  }
	  llassert(ready == 0);
	}
	multi_handle_w->check_run_count();
  }
}

//-----------------------------------------------------------------------------
// MultiHandle

MultiHandle::MultiHandle(void) : mHandleAddedOrRemoved(false), mPrevRunningHandles(0), mRunningHandles(0), mTimeOut(-1)
{
  curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETFUNCTION, &MultiHandle::socket_callback);
  curl_multi_setopt(mMultiHandle, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERFUNCTION, &MultiHandle::timer_callback);
  curl_multi_setopt(mMultiHandle, CURLMOPT_TIMERDATA, this);
}

MultiHandle::~MultiHandle()
{
  // This thread was terminated.
  // Curl demands that all handles are removed from the multi session handle before calling curl_multi_cleanup.
  for(addedEasyRequests_type::iterator iter = mAddedEasyRequests.begin(); iter != mAddedEasyRequests.end(); iter = mAddedEasyRequests.begin())
  {
	remove_easy_request(*iter);
  }
}

#ifdef CWDEBUG
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
  return curl_multi_info_read(mMultiHandle, msgs_in_queue);
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

CURLMcode MultiHandle::remove_easy_request(AICurlEasyRequest const& easy_request)
{
  addedEasyRequests_type::iterator iter = mAddedEasyRequests.find(easy_request);
  if (iter == mAddedEasyRequests.end())
	return (CURLMcode)-2;				// Was already removed before.
  CURLMcode res;
  {
	AICurlEasyRequest_wat curl_easy_request_w(**iter);
	res = curl_easy_request_w->remove_handle_from_multi(curl_easy_request_w, mMultiHandle);
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
		curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
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

//=============================================================================
// MAIN-THREAD (needing to access the above declarations).

//static
AICurlMultiHandle& AICurlMultiHandle::getInstance(void)
{
  LLThreadLocalData& tldata = LLThreadLocalData::tldata();
  if (!tldata.mCurlMultiHandle)
  {
	llinfos << "Creating AICurlMultiHandle for thread \"" << tldata.mName << "\"." << llendl;
	tldata.mCurlMultiHandle = new AICurlMultiHandle;
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
		// May not be inbetween being removed from the command queue but not added to the multi session handle yet.
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
		// May not be inbetween being removed from the command queue but not removed from the multi session handle yet.
		llassert(command_being_processed_r->command() != cmd_remove);
	  }
	  else
	  {
		// May not already have been removed from multi session handle.
		llassert(AICurlEasyRequest_wat(*get())->active());
	  }
	}
#endif
	// Add a command to remove this request from the multi session to the command queue.
	command_queue_w->push_back(Command(*this, cmd_remove));
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

