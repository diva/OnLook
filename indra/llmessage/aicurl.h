/**
 * @file aicurl.h
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
 *   17/03/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AICURL_H
#define AICURL_H

#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "llpreprocessor.h"
#include <curl/curl.h>		// Needed for files that include this header (also for aicurlprivate.h).
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif

// Make sure we don't use this option: it is not thread-safe.
#undef CURLOPT_DNS_USE_GLOBAL_CACHE
#define CURLOPT_DNS_USE_GLOBAL_CACHE do_not_use_CURLOPT_DNS_USE_GLOBAL_CACHE

#include "stdtypes.h"		// U16, S32, U32, F64
#include "llatomic.h"		// LLAtomicU32
#include "aithreadsafe.h"
#include "llhttpstatuscodes.h"
#include "llhttpclient.h"

// Debug Settings.
extern bool gNoVerifySSLCert;

class LLSD;
class LLBufferArray;
class LLChannelDescriptors;
class AIHTTPTimeoutPolicy;

// Some pretty printing for curl easy handle related things:
// Print the lock object related to the current easy handle in every debug output.
#ifdef CWDEBUG
#include <libcwd/buf2str.h>
#include <sstream>
#define DoutCurl(x) do { \
	using namespace libcwd; \
	std::ostringstream marker; \
	marker << (void*)this->get_lockobj(); \
	libcw_do.push_marker(); \
	libcw_do.marker().assign(marker.str().data(), marker.str().size()); \
	libcw_do.inc_indent(2); \
	Dout(dc::curl, x); \
	libcw_do.dec_indent(2); \
	libcw_do.pop_marker(); \
  } while(0)
#define DoutCurlEntering(x) do { \
	using namespace libcwd; \
	std::ostringstream marker; \
	marker << (void*)this->get_lockobj(); \
	libcw_do.push_marker(); \
	libcw_do.marker().assign(marker.str().data(), marker.str().size()); \
	libcw_do.inc_indent(2); \
	DoutEntering(dc::curl, x); \
	libcw_do.dec_indent(2); \
	libcw_do.pop_marker(); \
  } while(0)
#else // !CWDEBUG
#define DoutCurl(x) Dout(dc::curl, x << " [" << (void*)this->get_lockobj() << ']')
#define DoutCurlEntering(x) DoutEntering(dc::curl, x << " [" << (void*)this->get_lockobj() << ']')
#endif // CWDEBUG

//-----------------------------------------------------------------------------
// Exceptions.
//

// A general curl exception.
//
class AICurlError : public std::runtime_error {
  public:
	AICurlError(std::string const& message) : std::runtime_error(message) { }
};

class AICurlNoEasyHandle : public AICurlError {
  public:
	AICurlNoEasyHandle(std::string const& message) : AICurlError(message) { }
};

class AICurlNoMultiHandle : public AICurlError {
  public:
	AICurlNoMultiHandle(std::string const& message) : AICurlError(message) { }
};

class AICurlNoBody : public AICurlError {
  public:
	AICurlNoBody(std::string const& message) : AICurlError(message) { }
};

// End Exceptions.
//-----------------------------------------------------------------------------

// Things defined in this namespace are called from elsewhere in the viewer code.
namespace AICurlInterface {

//-----------------------------------------------------------------------------
// Global functions.

// Called to handle changes in Debug Settings.
bool handleCurlConcurrentConnections(LLSD const& newvalue);
bool handleNoVerifySSLCert(LLSD const& newvalue);

// Called once at start of application (from newview/llappviewer.cpp by main thread (before threads are created)),
// with main purpose to initialize curl.
void initCurl(void (*)(void) = NULL);

// Called once at start of application (from LLAppViewer::initThreads), starts AICurlThread.
void startCurlThread(U32 CurlConcurrentConnections, bool NoVerifySSLCert);

// Called once at end of application (from newview/llappviewer.cpp by main thread),
// with purpose to stop curl threads, free curl resources and deinitialize curl.
void cleanupCurl(void);

// Called from indra/llmessage/llurlrequest.cpp to print debug output regarding
// an error code returned by EasyRequest::getResult.
// Just returns curl_easy_strerror(errorcode).
std::string strerror(CURLcode errorcode);

// Called from indra/newview/llfloaterabout.cpp for the About floater, and
// from newview/llappviewer.cpp in behalf of debug output.
// Just returns curl_version().
std::string getVersionString(void);

// Called from newview/llappviewer.cpp (and llcrashlogger/llcrashlogger.cpp) to set
// the Certificate Authority file used to verify HTTPS certs.
void setCAFile(std::string const& file);

// Not called from anywhere.
// Can be used to set the path to the Certificate Authority file.
void setCAPath(std::string const& file);

} // namespace AICurlInterface

// Forward declaration (see aicurlprivate.h).
namespace AICurlPrivate {
  class CurlEasyRequest;
} // namespace AICurlPrivate

// Define access types (_crat = Const Read Access Type, _rat = Read Access Type, _wat = Write Access Type).
// Typical usage is:
// AICurlEasyRequest h1;				// Create easy handle.
// AICurlEasyRequest h2(h1);			// Make lightweight copies.
// AICurlEasyRequest_wat h2_w(*h2);	// Lock and obtain write access to the easy handle.
// Use *h2_w, which is a reference to the locked CurlEasyRequest instance.
// Note: As it is not allowed to use curl easy handles in any way concurrently,
// read access would at most give access to a CURL const*, which will turn out
// to be completely useless; therefore it is sufficient and efficient to use
// an AIThreadSafeSimple and it's unlikely that AICurlEasyRequest_rat will be used.
typedef AIAccessConst<AICurlPrivate::CurlEasyRequest> AICurlEasyRequest_rat;
typedef AIAccess<AICurlPrivate::CurlEasyRequest> AICurlEasyRequest_wat;

// Events generated by AICurlPrivate::CurlEasyHandle.
struct AICurlEasyHandleEvents {
	// Events.
	virtual void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	virtual void finished(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	virtual void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	// Avoid compiler warning.
	virtual ~AICurlEasyHandleEvents() { }
};

// Pointer to data we're going to POST.
class AIPostField : public LLThreadSafeRefCount {
  protected:
	char const* mData;

  public:
	AIPostField(char const* data) : mData(data) { }
	char const* data(void) const { return mData; }
};

// The pointer to the data that we have to POST is passed around as AIPostFieldPtr,
// which causes it to automatically clean up when there are no pointers left
// pointing to it.
typedef LLPointer<AIPostField> AIPostFieldPtr;

#include "aicurlprivate.h"

// AICurlPrivate::CurlEasyRequestPtr, a boost::intrusive_ptr, is no more threadsafe than a
// builtin type, but wrapping it in AIThreadSafe is obviously not going to help here.
// Therefore we use the following trick: we wrap CurlEasyRequestPtr too, and only allow
// read accesses on it.

// AICurlEasyRequest: a thread safe, reference counting, auto-cleaning curl easy handle.
class AICurlEasyRequest {
  private:
	// Use AICurlEasyRequestStateMachine, not AICurlEasyRequest.
	friend class AICurlEasyRequestStateMachine;

	// Initial construction is allowed (thread-safe).
	// Note: If ThreadSafeCurlEasyRequest() throws then the memory allocated is still freed.
	// 'new' never returned however and neither the constructor nor destructor of mCurlEasyRequest is called in this case.
	// This might throw AICurlNoEasyHandle.
	AICurlEasyRequest(bool buffered) :
	    mCurlEasyRequest(buffered ? new AICurlPrivate::ThreadSafeBufferedCurlEasyRequest : new AICurlPrivate::ThreadSafeCurlEasyRequest) { }

  public:
	// Used for storing this object in a standard container (see MultiHandle::add_easy_request).
	AICurlEasyRequest(AICurlEasyRequest const& orig) : mCurlEasyRequest(orig.mCurlEasyRequest) { }

	// For the rest, only allow read operations.
	AIThreadSafeSimple<AICurlPrivate::CurlEasyRequest>& operator*(void) const { llassert(mCurlEasyRequest.get()); return *mCurlEasyRequest; }
	AIThreadSafeSimple<AICurlPrivate::CurlEasyRequest>* operator->(void) const { llassert(mCurlEasyRequest.get()); return mCurlEasyRequest.get(); }
	AIThreadSafeSimple<AICurlPrivate::CurlEasyRequest>* get(void) const { return mCurlEasyRequest.get(); }

	// Returns true if this object points to the same CurlEasyRequest object.
	bool operator==(AICurlEasyRequest const& cer) const { return mCurlEasyRequest == cer.mCurlEasyRequest; }
	
	// Returns true if this object points to a different CurlEasyRequest object.
	bool operator!=(AICurlEasyRequest const& cer) const { return mCurlEasyRequest != cer.mCurlEasyRequest; }
	
	// Queue this request for insertion in the multi session.
	void addRequest(void);

	// Queue a command to remove this request from the multi session (or cancel a queued command to add it).
	void removeRequest(void);

	// Returns true when this AICurlEasyRequest wraps a AICurlPrivate::ThreadSafeBufferedCurlEasyRequest.
	bool isBuffered(void) const { return mCurlEasyRequest->isBuffered(); }

  private:
	// The actual pointer to the ThreadSafeCurlEasyRequest instance.
	AICurlPrivate::CurlEasyRequestPtr mCurlEasyRequest;

  private:
	// Assignment would not be thread-safe; we may create this object and read from it.
	// Note: Destruction is implicitly assumed thread-safe, as it would be a logic error to
	// destruct it while another thread still needs it, concurrent or not.
	AICurlEasyRequest& operator=(AICurlEasyRequest const&) { return *this; }

  public:
	// Instead of assignment, it might be helpful to use swap.
	void swap(AICurlEasyRequest& cer) { mCurlEasyRequest.swap(cer.mCurlEasyRequest); }

  public:
	// The more exotic member functions of this class, to deal with passing this class
	// as CURLOPT_PRIVATE pointer to a curl handle and afterwards restore it.
	// For "internal use" only; don't use things from AICurlPrivate yourself.

	// It's thread-safe to give read access the underlaying boost::intrusive_ptr.
	// It's not OK to then call get() on that and store the AICurlPrivate::ThreadSafeCurlEasyRequest* separately.
	AICurlPrivate::CurlEasyRequestPtr const& get_ptr(void) const { return mCurlEasyRequest; }

	// If we have a correct (with regard to reference counting) AICurlPrivate::CurlEasyRequestPtr,
	// then it's OK to construct a AICurlEasyRequest from it.
	// Note that the external AICurlPrivate::CurlEasyRequestPtr needs its own locking, because
	// it's not thread-safe in itself.
	AICurlEasyRequest(AICurlPrivate::CurlEasyRequestPtr const& ptr) : mCurlEasyRequest(ptr) { }

	// This one is obviously dangerous. It's for use only in MultiHandle::check_run_count.
	// See also the long comment in CurlEasyRequest::finalizeRequest with regard to CURLOPT_PRIVATE.
	explicit AICurlEasyRequest(AICurlPrivate::ThreadSafeCurlEasyRequest* ptr) : mCurlEasyRequest(ptr) { }
};

// Write Access Type for the buffer.
struct AICurlResponderBuffer_wat : public AIAccess<AICurlPrivate::CurlResponderBuffer> {
  explicit AICurlResponderBuffer_wat(AICurlPrivate::ThreadSafeBufferedCurlEasyRequest& lockobj) :
	  AIAccess<AICurlPrivate::CurlResponderBuffer>(lockobj) { }
  AICurlResponderBuffer_wat(AIThreadSafeSimple<AICurlPrivate::CurlEasyRequest>& lockobj) :
	  AIAccess<AICurlPrivate::CurlResponderBuffer>(static_cast<AICurlPrivate::ThreadSafeBufferedCurlEasyRequest&>(lockobj)) { }
};

#define AICurlPrivate DONTUSE_AICurlPrivate

#endif
