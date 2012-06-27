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

namespace AICurlPrivate {
namespace curlthread { class MultiHandle; }

CURLcode check_easy_code(CURLcode code);
CURLMcode check_multi_code(CURLMcode code);

bool curlThreadIsRunning(void);
void wakeUpCurlThread(void);

class ThreadSafeCurlEasyRequest;
class ThreadSafeBufferedCurlEasyRequest;

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
	template<typename BUILTIN>
	  CURLcode setopt(CURLoption option, BUILTIN parameter);

	// Clone a libcurl session handle using all the options previously set.
	CurlEasyHandle(CurlEasyHandle const& orig) : mEasyHandle(curl_easy_duphandle(orig.mEasyHandle)), mActiveMultiHandle(NULL), mErrorBuffer(NULL) { }

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

  private:
	CURL* mEasyHandle;
	CURLM* mActiveMultiHandle;
	char* mErrorBuffer;
	static LLAtomicU32 sTotalEasyHandles;

  private:
	// This should only be called from MultiHandle; add/remove an easy handle to/from a multi handle.
	friend class curlthread::MultiHandle;
	CURLMcode add_handle_to_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);
	CURLMcode remove_handle_from_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);

  public:
	// Retuns total number of existing CURL* handles (excluding ones created outside this class).
	static U32 getTotalEasyHandles(void) { return sTotalEasyHandles; }

	// Returns true if this easy handle was added to a curl multi handle.
	bool active(void) const { return mActiveMultiHandle; }

	// Call this prior to every curl_easy function whose return value is passed to check_easy_code.
	void setErrorBuffer(void);

	// If there was an error code as result, then this returns a human readable error string.
	// Only valid when setErrorBuffer was called and the curl_easy function returned an error.
	std::string getErrorString(void) const { return mErrorBuffer; }

	// Used for debugging purposes.
	bool operator==(CURL* easy_handle) const { return mEasyHandle == easy_handle; }

  protected:
	// Return the underlaying curl easy handle.
	CURL* getEasyHandle(void) const { return mEasyHandle; }

  private:
	// Return, and possibly create, the curl (easy) error buffer used by the current thread.
	static char* getTLErrorBuffer(void);
};

template<typename BUILTIN>
CURLcode CurlEasyHandle::setopt(CURLoption option, BUILTIN parameter)
{
  llassert(!mActiveMultiHandle);
  setErrorBuffer();
  return check_easy_code(curl_easy_setopt(mEasyHandle, option, parameter));
}

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
	// Call back stubs.
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
	// Post initialization, set the parent to which to pass the events to.
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
// The life time of a CurlResponderBuffer is slightly shorter than it's
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

	std::stringstream& getInput() { return mInput; }
	std::stringstream& getHeaderOutput() { return mHeaderOutput; }
	LLIOPipe::buffer_ptr_t& getOutput() { return mOutput; }

	// Called after removed_from_multi_handle was called.
	void processOutput(AICurlEasyRequest_wat& curl_easy_request_w);

  protected:
	/*virtual*/ void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void finished(AICurlEasyRequest_wat& curl_easy_request_w);
	/*virtual*/ void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);

  private:
	std::stringstream mInput;
	std::stringstream mHeaderOutput;
	LLIOPipe::buffer_ptr_t mOutput;
	AICurlInterface::ResponderPtr mResponder;

  public:
	static LLChannelDescriptors const sChannels;		// Channel object for mOutput: we ONLY use channel 0, so this can be a constant.

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

  private:
	LLAtomicU32 mReferenceCount;

	friend void intrusive_ptr_add_ref(ThreadSafeCurlEasyRequest* p);	// Called by boost::intrusive_ptr when a new copy of a boost::intrusive_ptr<ThreadSafeCurlEasyRequest> is made.
	friend void intrusive_ptr_release(ThreadSafeCurlEasyRequest* p);	// Called by boost::intrusive_ptr when a boost::intrusive_ptr<ThreadSafeCurlEasyRequest> is destroyed.
};

// Same as the above but adds a CurlResponderBuffer. The latter has it's own locking in order to
// allow casting the underlaying CurlEasyRequest to ThreadSafeCurlEasyRequest, independent of
// what class it is part of: ThreadSafeCurlEasyRequest or ThreadSafeBufferedCurlEasyRequest.
// The virtual destructor of ThreadSafeCurlEasyRequest allows to treat each easy handle transparently
// as a ThreadSafeCurlEasyRequest object, or optionally dynamic_cast it to a ThreadSafeBufferedCurlEasyRequest.
// Note: the order of these base classes is important: AIThreadSafeSimple<CurlResponderBuffer> is now
// destructed before ThreadSafeCurlEasyRequest is.
class ThreadSafeBufferedCurlEasyRequest : public ThreadSafeCurlEasyRequest, public AIThreadSafeSimple<CurlResponderBuffer> {
  public:
	// Throws AICurlNoEasyHandle.
	ThreadSafeBufferedCurlEasyRequest(void) { new (AIThreadSafeSimple<CurlResponderBuffer>::ptr()) CurlResponderBuffer; }
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
	template<typename BUILTIN>
	  CURLMcode setopt(CURLMoption option, BUILTIN parameter);

	// Returns total number of existing CURLM* handles (excluding ones created outside this class).
	static U32 getTotalMultiHandles(void) { return sTotalMultiHandles; }
};

template<typename BUILTIN>
CURLMcode CurlMultiHandle::setopt(CURLMoption option, BUILTIN parameter)
{
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}

} // namespace AICurlPrivate

#endif
