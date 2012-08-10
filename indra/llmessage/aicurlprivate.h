/**
 * @file aicurlprivate.h
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

#ifndef AICURLPRIVATE_H
#define AICURLPRIVATE_H

#include <sstream>
#include "llatomic.h"

namespace AICurlPrivate {
namespace curlthread { class MultiHandle; }

struct Stats {
  static LLAtomicU32 easy_calls;
  static LLAtomicU32 easy_errors;
  static LLAtomicU32 easy_init_calls;
  static LLAtomicU32 easy_init_errors;
  static LLAtomicU32 easy_cleanup_calls;
  static LLAtomicU32 multi_calls;
  static LLAtomicU32 multi_errors;

 static void print(void);
};

void handle_multi_error(CURLMcode code);
inline CURLMcode check_multi_code(CURLMcode code) { Stats::multi_calls++; if (code != CURLM_OK) handle_multi_error(code); return code; }

bool curlThreadIsRunning(void);
void wakeUpCurlThread(void);
void stopCurlThread(void);

class ThreadSafeCurlEasyRequest;
class ThreadSafeBufferedCurlEasyRequest;

#define DECLARE_SETOPT(param_type) \
	CURLcode setopt(CURLoption option, param_type parameter)

// This class wraps CURL*'s.
// It guarantees that a pointer is cleaned up when no longer needed, as required by libcurl.
class CurlEasyHandle : public boost::noncopyable, protected AICurlEasyHandleEvents {
  public:
	CurlEasyHandle(void);
	~CurlEasyHandle();

  private:
	// Disallow assignment.
	CurlEasyHandle& operator=(CurlEasyHandle const*);

  public:
	// Reset all options of a libcurl session handle.
	void reset(void) { llassert(!mActiveMultiHandle); curl_easy_reset(mEasyHandle); }

	// Set options for a curl easy handle.
	DECLARE_SETOPT(long);
	DECLARE_SETOPT(long long);
	DECLARE_SETOPT(void const*);
	DECLARE_SETOPT(curl_debug_callback);
	DECLARE_SETOPT(curl_write_callback);
	//DECLARE_SETOPT(curl_read_callback);	Same type as curl_write_callback
	DECLARE_SETOPT(curl_ssl_ctx_callback);
	DECLARE_SETOPT(curl_conv_callback);
#if 0	// Not used by the viewer.
	DECLARE_SETOPT(curl_progress_callback);
	DECLARE_SETOPT(curl_seek_callback);
	DECLARE_SETOPT(curl_ioctl_callback);
	DECLARE_SETOPT(curl_sockopt_callback);
	DECLARE_SETOPT(curl_opensocket_callback);
	DECLARE_SETOPT(curl_closesocket_callback);
	DECLARE_SETOPT(curl_sshkeycallback);
	DECLARE_SETOPT(curl_chunk_bgn_callback);
	DECLARE_SETOPT(curl_chunk_end_callback);
	DECLARE_SETOPT(curl_fnmatch_callback);
#endif
#if __LP64__	// sizeof(long) > sizeof(S32), see http://en.cppreference.com/w/cpp/language/types
	// Automatically cast small int types to a long if they differ in size.
	CURLcode setopt(CURLoption option, U32 parameter) { return setopt(option, (long)parameter); }
	CURLcode setopt(CURLoption option, S32 parameter) { return setopt(option, (long)parameter); }
#endif

	// Clone a libcurl session handle using all the options previously set.
	//CurlEasyHandle(CurlEasyHandle const& orig);

	// URL encode/decode the given string.
	char* escape(char* url, int length);
	char* unescape(char* url, int inlength , int* outlength);

	// Extract information from a curl handle.
	CURLcode getinfo(CURLINFO info, void* data);
#if _WIN64 || __x86_64__ || __ppc64__
	// Overload for integer types that are too small (libcurl demands a long).
	CURLcode getinfo(CURLINFO info, S32* data) { long ldata; CURLcode res = getinfo(info, &ldata); *data = static_cast<S32>(ldata); return res; }
	CURLcode getinfo(CURLINFO info, U32* data) { long ldata; CURLcode res = getinfo(info, &ldata); *data = static_cast<U32>(ldata); return res; }
#endif

	// Perform a file transfer (blocking).
	CURLcode perform(void);
	// Pause and unpause a connection.
	CURLcode pause(int bitmask);

	// Called when a request is queued for removal. In that case a race between the actual removal
	// and revoking of the callbacks is harmless (and happens for the raw non-statemachine version).
	void remove_queued(void) { mQueuedForRemoval = true; }
	// In case it's added after being removed.
	void add_queued(void) { mQueuedForRemoval = false; }

  private:
	CURL* mEasyHandle;
	CURLM* mActiveMultiHandle;
	char* mErrorBuffer;
	bool mQueuedForRemoval;			// Set if the easy handle is (probably) added to the multi handle, but is queued for removal.
#ifdef SHOW_ASSERT
  public:
	bool mRemovedPerCommand;		// Set if mActiveMultiHandle was reset as per command from the main thread.
#endif

  private:
	// This should only be called from MultiHandle; add/remove an easy handle to/from a multi handle.
	friend class curlthread::MultiHandle;
	CURLMcode add_handle_to_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);
	CURLMcode remove_handle_from_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);

  public:
	// Returns true if this easy handle was added to a curl multi handle.
	bool active(void) const { return mActiveMultiHandle; }

	// If there was an error code as result, then this returns a human readable error string.
	// Only valid when setErrorBuffer was called and the curl_easy function returned an error.
	std::string getErrorString(void) const { return mErrorBuffer ? mErrorBuffer : "(null)"; }

	// Returns true when it is expected that the parent will revoke callbacks before the curl
	// easy handle is removed from the multi handle; that usually happens when an external
	// error demands termination of the request (ie, an expiration).
	bool no_warning(void) const { return mQueuedForRemoval || LLApp::isExiting(); }

	// Used for debugging purposes.
	bool operator==(CURL* easy_handle) const { return mEasyHandle == easy_handle; }

  private:
	// Call this prior to every curl_easy function whose return value is passed to check_easy_code.
	void setErrorBuffer(void);

	static void handle_easy_error(CURLcode code);

	// Always first call setErrorBuffer()!
	static inline CURLcode check_easy_code(CURLcode code)
	{
	  Stats::easy_calls++;
	  if (code != CURLE_OK)
		handle_easy_error(code);
	  return code;
	}

  protected:
	// Return the underlying curl easy handle.
	CURL* getEasyHandle(void) const { return mEasyHandle; }

  private:
	// Return, and possibly create, the curl (easy) error buffer used by the current thread.
	static char* getTLErrorBuffer(void);
};

// CurlEasyRequest adds a slightly more powerful interface that can be used
// to set the options on a curl easy handle.
//
// Calling sendRequest() will then connect to the given URL and perform
// the data exchange. If an error occurs related to this handle, it can
// be read by calling getErrorString().
//
// Note that the life cycle of a CurlEasyRequest is controlled by AICurlEasyRequest:
// a CurlEasyRequest is only ever created as base class of a ThreadSafeCurlEasyRequest,
// which is only created by creating a AICurlEasyRequest. When the last copy of such
// AICurlEasyRequest is deleted, then also the ThreadSafeCurlEasyRequest is deleted
// and the CurlEasyRequest destructed.
class CurlEasyRequest : public CurlEasyHandle {
  public:
	void setoptString(CURLoption option, std::string const& value);
	void setPost(char const* postdata, S32 size);
	void addHeader(char const* str);

  private:
	// Callback stubs.
	static size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t readCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static CURLcode SSLCtxCallback(CURL* curl, void* sslctx, void* userdata);

	curl_write_callback mHeaderCallback;
	void* mHeaderCallbackUserData;
	curl_write_callback mWriteCallback;
	void* mWriteCallbackUserData;
	curl_read_callback mReadCallback;
	void* mReadCallbackUserData;
	curl_ssl_ctx_callback mSSLCtxCallback;
	void* mSSLCtxCallbackUserData;

  public:
	void setHeaderCallback(curl_write_callback callback, void* userdata);
	void setWriteCallback(curl_write_callback callback, void* userdata);
	void setReadCallback(curl_read_callback callback, void* userdata);
	void setSSLCtxCallback(curl_ssl_ctx_callback callback, void* userdata);

	// Call this if the set callbacks are about to be invalidated.
	void revokeCallbacks(void);

	// Reset everything to the state it was in when this object was just created.
	void resetState(void);

  private:
	// Called from applyDefaultOptions.
	void applyProxySettings(void);

  public:
	// Set default options that we want applied to all curl easy handles.
	void applyDefaultOptions(void);

	// Prepare the request for adding it to a multi session, or calling perform.
	// This actually adds the headers that were collected with addHeader.
	void finalizeRequest(std::string const& url);

	// Store result code that is returned by getResult.
	void store_result(CURLcode result) { mResult = result; }

	// Called when the curl easy handle is done.
	void done(AICurlEasyRequest_wat& curl_easy_request_w) { finished(curl_easy_request_w); }

	// Fill info with the transfer info.
	void getTransferInfo(AICurlInterface::TransferInfo* info);

	// If result != CURLE_FAILED_INIT then also info was filled.
	void getResult(CURLcode* result, AICurlInterface::TransferInfo* info = NULL);

  private:
	curl_slist* mHeaders;
	bool mRequestFinalized;
	AICurlEasyHandleEvents* mEventsTarget;
	CURLcode mResult;

  private:
	// This class may only be created by constructing a ThreadSafeCurlEasyRequest.
	friend class ThreadSafeCurlEasyRequest;
	// Throws AICurlNoEasyHandle.
	CurlEasyRequest(void) :
	    mHeaders(NULL), mRequestFinalized(false), mEventsTarget(NULL), mResult(CURLE_FAILED_INIT)
      { applyDefaultOptions(); }
  public:
	~CurlEasyRequest();

  public:
	// Post-initialization, set the parent to pass the events to.
	void send_events_to(AICurlEasyHandleEvents* target) { mEventsTarget = target; }

	// For debugging purposes
	bool is_finalized(void) const { return mRequestFinalized; }

	// Return pointer to the ThreadSafe (wrapped) version of this object.
	ThreadSafeCurlEasyRequest* get_lockobj(void);

  protected:
	// Pass events to parent.
	/*virtual*/ void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void finished(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
};

// Buffers used by the AICurlInterface::Request API.
// Curl callbacks write into and read from these buffers.
// The interface with the rest of the code is through AICurlInterface::Responder.
//
// The lifetime of a CurlResponderBuffer is slightly shorter than its
// associated CurlEasyRequest; this class can only be created as base class
// of ThreadSafeBufferedCurlEasyRequest, and is therefore constructed after
// the construction of the associated CurlEasyRequest and destructed before it.
// Hence, it's safe to use get_lockobj() and through that access the CurlEasyRequest
// object at all times.
//
// A CurlResponderBuffer is thus created when a ThreadSafeBufferedCurlEasyRequest
// is created which only happens by creating a AICurlEasyRequest(true) instance,
// and when the last AICurlEasyRequest is deleted, then the ThreadSafeBufferedCurlEasyRequest
// is deleted and the CurlResponderBuffer destructed.
class CurlResponderBuffer : protected AICurlEasyHandleEvents {
  public:
	void resetState(AICurlEasyRequest_wat& curl_easy_request_w);
	void prepRequest(AICurlEasyRequest_wat& buffered_curl_easy_request_w, std::vector<std::string> const& headers, AICurlInterface::ResponderPtr responder, S32 time_out = 0, bool post = false);

	LLIOPipe::buffer_ptr_t& getInput(void) { return mInput; }
	std::stringstream& getHeaderOutput(void) { return mHeaderOutput; }
	LLIOPipe::buffer_ptr_t& getOutput(void) { return mOutput; }

	// Called if libcurl doesn't deliver within CurlRequestTimeOut seconds.
	void timed_out(void);

	// Called after removed_from_multi_handle was called.
	void processOutput(AICurlEasyRequest_wat& curl_easy_request_w);

  protected:
	/*virtual*/ void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void finished(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);

  private:
	LLIOPipe::buffer_ptr_t mInput;
	U8* mLastRead;										// Pointer into mInput where we last stopped reading (or NULL to start at the beginning).
	std::stringstream mHeaderOutput;
	LLIOPipe::buffer_ptr_t mOutput;
	AICurlInterface::ResponderPtr mResponder;

  public:
	static LLChannelDescriptors const sChannels;		// Channel object for mInput (channel out()) and mOutput (channel in()).

  private:
	// This class may only be created by constructing a ThreadSafeBufferedCurlEasyRequest.
	friend class ThreadSafeBufferedCurlEasyRequest;
	CurlResponderBuffer(void);
  public:
	~CurlResponderBuffer();

  private:
    static size_t curlWriteCallback(char* data, size_t size, size_t nmemb, void* user_data);
    static size_t curlReadCallback(char* data, size_t size, size_t nmemb, void* user_data);
    static size_t curlHeaderCallback(char* data, size_t size, size_t nmemb, void* user_data);

  public:
	// Return pointer to the ThreadSafe (wrapped) version of this object.
	ThreadSafeBufferedCurlEasyRequest* get_lockobj(void);

	// Return true when prepRequest was already called and the object has not been
	// invalidated as a result of calling timed_out().
	bool isValid(void) const { return mResponder; }
};

// This class wraps CurlEasyRequest for thread-safety and adds a reference counter so we can
// copy it around cheaply and it gets destructed automatically when the last instance is deleted.
// It guarantees that the CURL* handle is never used concurrently, which is not allowed by libcurl.
// As AIThreadSafeSimpleDC contains a mutex, it cannot be copied. Therefore we need a reference counter for this object.
class ThreadSafeCurlEasyRequest : public AIThreadSafeSimple<CurlEasyRequest> {
  public:
	// Throws AICurlNoEasyHandle.
	ThreadSafeCurlEasyRequest(void) : mReferenceCount(0)
        { new (ptr()) CurlEasyRequest;
		  Dout(dc::curl, "Creating ThreadSafeCurlEasyRequest with this = " << (void*)this); }
	virtual ~ThreadSafeCurlEasyRequest()
	    { Dout(dc::curl, "Destructing ThreadSafeCurlEasyRequest with this = " << (void*)this); }

	// Returns true if this is a base class of ThreadSafeBufferedCurlEasyRequest.
	virtual bool isBuffered(void) const { return false; }

  private:
	LLAtomicU32 mReferenceCount;

	friend void intrusive_ptr_add_ref(ThreadSafeCurlEasyRequest* p);	// Called by boost::intrusive_ptr when a new copy of a boost::intrusive_ptr<ThreadSafeCurlEasyRequest> is made.
	friend void intrusive_ptr_release(ThreadSafeCurlEasyRequest* p);	// Called by boost::intrusive_ptr when a boost::intrusive_ptr<ThreadSafeCurlEasyRequest> is destroyed.
};

// Same as the above but adds a CurlResponderBuffer. The latter has its own locking in order to
// allow casting the underlying CurlEasyRequest to ThreadSafeCurlEasyRequest, independent of
// what class it is part of: ThreadSafeCurlEasyRequest or ThreadSafeBufferedCurlEasyRequest.
// The virtual destructor of ThreadSafeCurlEasyRequest allows to treat each easy handle transparently
// as a ThreadSafeCurlEasyRequest object, or optionally dynamic_cast it to a ThreadSafeBufferedCurlEasyRequest.
// Note: the order of these base classes is important: AIThreadSafeSimple<CurlResponderBuffer> is now
// destructed before ThreadSafeCurlEasyRequest is.
class ThreadSafeBufferedCurlEasyRequest : public ThreadSafeCurlEasyRequest, public AIThreadSafeSimple<CurlResponderBuffer> {
  public:
	// Throws AICurlNoEasyHandle.
	ThreadSafeBufferedCurlEasyRequest(void) { new (AIThreadSafeSimple<CurlResponderBuffer>::ptr()) CurlResponderBuffer; }

	/*virtual*/ bool isBuffered(void) const { return true; }
};

// The curl easy request type wrapped in a reference counting pointer.
typedef boost::intrusive_ptr<AICurlPrivate::ThreadSafeCurlEasyRequest> CurlEasyRequestPtr;

// This class wraps CURLM*'s.
// It guarantees that a pointer is cleaned up when no longer needed, as required by libcurl.
class CurlMultiHandle : public boost::noncopyable {
  public:
	CurlMultiHandle(void);
	~CurlMultiHandle();

  private:
	// Disallow assignment.
	CurlMultiHandle& operator=(CurlMultiHandle const*);

  private:
	static LLAtomicU32 sTotalMultiHandles;

  protected:
	CURLM* mMultiHandle;

  public:
	// Set options for a curl multi handle.
	CURLMcode setopt(CURLMoption option, long parameter);
	CURLMcode setopt(CURLMoption option, curl_socket_callback parameter);
	CURLMcode setopt(CURLMoption option, curl_multi_timer_callback parameter);
	CURLMcode setopt(CURLMoption option, void* parameter);

	// Returns total number of existing CURLM* handles (excluding ones created outside this class).
	static U32 getTotalMultiHandles(void) { return sTotalMultiHandles; }
};

// Overload the setopt methods in order to enforce the correct types (ie, convert an int to a long).

// curl_multi_setopt may only be passed a long,
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, long parameter)
{
  llassert(option == CURLMOPT_MAXCONNECTS || option == CURLMOPT_PIPELINING);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}

// ... or a function pointer,
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, curl_socket_callback parameter)
{
  llassert(option == CURLMOPT_SOCKETFUNCTION);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}

inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, curl_multi_timer_callback parameter)
{
  llassert(option == CURLMOPT_TIMERFUNCTION);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}

// ... or an object pointer.
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, void* parameter)
{
  llassert(option == CURLMOPT_SOCKETDATA || option == CURLMOPT_TIMERDATA);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}

} // namespace AICurlPrivate

#endif
