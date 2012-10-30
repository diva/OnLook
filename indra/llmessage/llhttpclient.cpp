/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include <boost/shared_ptr.hpp>
#include <xmlrpc-epi/xmlrpc.h>

#include "llhttpclient.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "llvfile.h"
#include "llurlrequest.h"
#include "llxmltree.h"

class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy blockingLLSDPost_timeout;
extern AIHTTPTimeoutPolicy blockingLLSDGet_timeout;
extern AIHTTPTimeoutPolicy blockingRawGet_timeout;

////////////////////////////////////////////////////////////////////////////

class XMLRPCInjector : public Injector {
public:
	XMLRPCInjector(XMLRPC_REQUEST request) : mRequest(request), mRequestText(NULL) { }
	~XMLRPCInjector() { XMLRPC_RequestFree(mRequest, 1); XMLRPC_Free(const_cast<char*>(mRequestText)); }

	/*virtual*/ char const* contentType(void) const { return "text/xml"; }
	/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		int requestTextSize;
		mRequestText = XMLRPC_REQUEST_ToXML(mRequest, &requestTextSize);
		if (!mRequestText)
			throw AICurlNoBody("XMLRPC_REQUEST_ToXML returned NULL.");
		LLBufferStream ostream(channels, buffer.get());
		ostream.write(mRequestText, requestTextSize);
		ostream << std::flush;          // Always flush a LLBufferStream when done writing to it.
		return requestTextSize;
	}

private:
	XMLRPC_REQUEST const mRequest;
	char const* mRequestText;
};

class XmlTreeInjector : public Injector
{
	public:
		XmlTreeInjector(LLXmlTree const* tree) : mTree(tree) { }
		~XmlTreeInjector() { delete const_cast<LLXmlTree*>(mTree); }

		/*virtual*/ char const* contentType(void) const { return "application/xml"; }
		/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
		{
			std::string data;
			mTree->write(data);
			LLBufferStream ostream(channels, buffer.get());
			ostream.write(data.data(), data.size());
			ostream << std::flush;          // Always flush a LLBufferStream when done writing to it.
			return data.size();
		}

	private:
		LLXmlTree const* mTree;
};

class LLSDInjector : public Injector
{
  public:
	LLSDInjector(LLSD const& sd) : mSD(sd) { }

	/*virtual*/ char const* contentType(void) const { return "application/llsd+xml"; }

	/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		LLSDSerialize::toXML(mSD, ostream);
		// Need to flush the LLBufferStream or count_out() returns more than the written data.
		ostream << std::flush;
		return ostream.count_out();
	}

	LLSD const mSD;
};

class RawInjector : public Injector
{
  public:
	RawInjector(char const* data, U32 size) : mData(data), mSize(size) { }
	/*virtual*/ ~RawInjector() { delete [] mData; }

	/*virtual*/ char const* contentType(void) const { return "application/octet-stream"; }

	/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		ostream.write(mData, mSize);
		ostream << std::flush;			// Always flush a LLBufferStream when done writing to it.
		return mSize;
	}

	char const* mData;
	U32 mSize;
};

class FileInjector : public Injector
{
  public:
	FileInjector(std::string const& filename) : mFilename(filename) { }

	char const* contentType(void) const { return "application/octet-stream"; }

	/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		llifstream fstream(mFilename, std::iostream::binary | std::iostream::out);
		if (!fstream.is_open())
		  throw AICurlNoBody(llformat("Failed to open \"%s\".", mFilename.c_str()));
		LLBufferStream ostream(channels, buffer.get());
		char tmpbuf[4096];
#ifdef SHOW_ASSERT
		size_t total_len = 0;
		fstream.seekg(0, std::ios::end);
		size_t file_size = fstream.tellg();
		fstream.seekg(0, std::ios::beg);
#endif
		while (fstream)
		{
			std::streamsize len = fstream.readsome(tmpbuf, sizeof(tmpbuf));
			if (len > 0)
			{
				ostream.write(tmpbuf, len);
#ifdef SHOW_ASSERT
				total_len += len;
#endif
			}
		}
		fstream.close();
		ostream << std::flush;
		llassert(total_len == file_size && total_len == ostream.count_out());
		return ostream.count_out();
	}

	std::string const mFilename;
};

class VFileInjector : public Injector
{
public:
	VFileInjector(LLUUID const& uuid, LLAssetType::EType asset_type) : mUUID(uuid), mAssetType(asset_type) { }

	/*virtual*/ char const* contentType(void) const { return "application/octet-stream"; }

	/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		
		LLVFile vfile(gVFS, mUUID, mAssetType, LLVFile::READ);
		S32 fileSize = vfile.getSize();
		std::vector<U8> fileBuffer(fileSize);
		vfile.read(&fileBuffer[0], fileSize);
		ostream.write((char*)&fileBuffer[0], fileSize);
		ostream << std::flush;
		
		return fileSize;
	}

	LLUUID const mUUID;
	LLAssetType::EType mAssetType;
};

static void request(
	const std::string& url,
	LLURLRequest::ERequestAction method,
	Injector* body_injector,
	LLCurl::ResponderPtr responder,
	AIHTTPHeaders& headers,
	bool is_auth = false,
	bool no_compression = false)
{
	if (responder)
	{
		// For possible debug output from within the responder.
		responder->setURL(url);
	}

	LLURLRequest* req;
	try
	{
		req = new LLURLRequest(method, url, body_injector, responder, headers, is_auth, no_compression);
	}
	catch(AICurlNoEasyHandle& error)
	{
		llwarns << "Failed to create LLURLRequest: " << error.what() << llendl;
		// This is what the old LL code did: no recovery whatsoever (but also no leaks or crash).
		return ;
	}

	req->run();
}

void LLHTTPClient::getByteRange4(std::string const& url, S32 offset, S32 bytes, ResponderPtr responder, AIHTTPHeaders& headers)
{
	if(offset > 0 || bytes > 0)
	{
		headers.addHeader("Range", llformat("bytes=%d-%d", offset, offset + bytes - 1));
	}
    request(url, LLURLRequest::HTTP_GET, NULL, responder, headers);
}

void LLHTTPClient::head4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_HEAD, NULL, responder, headers);
}

void LLHTTPClient::get4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_GET, NULL, responder, headers);
}

void LLHTTPClient::getHeaderOnly4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_HEAD, NULL, responder, headers);
}

void LLHTTPClient::get4(std::string const& url, LLSD const& query, ResponderPtr responder, AIHTTPHeaders& headers)
{
	LLURI uri;
	
	uri = LLURI::buildHTTP(url, LLSD::emptyArray(), query);
	get4(uri.asString(), responder, headers);
}

//=============================================================================
// Blocking Responders.
//

class BlockingResponder : public AICurlInterface::LegacyPolledResponder {
private:
	LLCondition mSignal;	// Wait condition to wait till mFinished is true.
	static LLSD LLSD_dummy;
	static std::string Raw_dummy;

public:
	void wait(void);		// Blocks until mFinished is true.
	virtual LLSD const& getLLSD(void) const { llassert(false); return LLSD_dummy; }
	virtual std::string const& getRaw(void) const { llassert(false); return Raw_dummy; }

protected:
	void wakeup(void);		// Call this at the end of completedRaw.
};

LLSD BlockingResponder::LLSD_dummy;
std::string BlockingResponder::Raw_dummy;

void BlockingResponder::wait(void)
{
  if (AIThreadID::in_main_thread())
  {
	// We're the main thread, so we have to give AIStateMachine CPU cycles.
	while (!mFinished)
	{
	  AIStateMachine::mainloop();
	  ms_sleep(10);
	}
  }
  else	// Hopefully not the curl thread :p
  {
	mSignal.lock();
	while (!mFinished)
	  mSignal.wait();
	mSignal.unlock();
  }
}

void BlockingResponder::wakeup(void)
{
  // Normally mFinished is set immediately after returning from this function,
  // but we do it here, because we need to set it before calling mSignal.signal().
  mSignal.lock();
  mFinished = true;
  mSignal.unlock();
  mSignal.signal();
}

class BlockingLLSDResponder : public BlockingResponder {
private:
	LLSD mResponse;

protected:
	/*virtual*/ LLSD const& getLLSD(void) const { llassert(mFinished && mCode == CURLE_OK && mStatus == HTTP_OK); return mResponse; }
	/*virtual*/ void completedRaw(U32 status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		decode_llsd_body(status, reason, channels, buffer, mResponse);		// This puts the body asString() in mResponse in case of http error.
		wakeup();
	}
};

class BlockingRawResponder : public BlockingResponder {
private:
	std::string mResponse;

protected:
	/*virtual*/ std::string const& getRaw(void) const { llassert(mFinished && mCode == CURLE_OK && mStatus == HTTP_OK); return mResponse; }
	/*virtual*/ void completedRaw(U32 status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		decode_raw_body(mCode, reason, channels, buffer, mResponse);
		wakeup();
	}
};

class BlockingLLSDPostResponder : public BlockingLLSDResponder {
public:
	/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingLLSDPost_timeout; }
};

class BlockingLLSDGetResponder : public BlockingLLSDResponder {
public:
	/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingLLSDGet_timeout; }
};

class BlockingRawGetResponder : public BlockingRawResponder {
public:
	/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingRawGet_timeout; }
};

// End blocking responders.
//=============================================================================

// These calls are blocking! This is usually bad, unless you're a dataserver. Then it's awesome.

enum EBlockingRequestAction {
  HTTP_LLSD_POST,
  HTTP_LLSD_GET,
  HTTP_RAW_GET
};

/**
	@brief does a blocking request on the url, returning the data or bad status.

	@param url URI to verb on.
	@param method the verb to hit the URI with.
	@param body the body of the call (if needed - for instance not used for GET and DELETE, but is for POST and PUT)
	@param headers HTTP headers to use for the request.
	@param timeout Curl timeout to use. Defaults to 5. Rationale:
	Without this timeout, blockingGet() calls have been observed to take
	up to 90 seconds to complete.  Users of blockingGet() already must 
	check the HTTP return code for validity, so this will not introduce
	new errors.  A 5 second timeout will succeed > 95% of the time (and 
	probably > 99% of the time) based on my statistics. JC

	@returns an LLSD map: {status: integer, body: map}
  */
static LLSD blocking_request(
	std::string const& url,
	EBlockingRequestAction method,
	LLSD const& body)				// Only used for HTTP_LLSD_POST
{
	lldebugs << "blockingRequest of " << url << llendl;

	AIHTTPHeaders headers;
	boost::intrusive_ptr<BlockingResponder> responder;
	if (method == HTTP_LLSD_POST)
	{
		responder = new BlockingLLSDPostResponder;
		LLHTTPClient::post4(url, body, responder, headers);
	}
	else if (method == HTTP_LLSD_GET)
	{
		responder = new BlockingLLSDGetResponder;
		LLHTTPClient::get4(url, responder, headers);
	}
	else // method == HTTP_RAW_GET
	{
		responder = new BlockingRawGetResponder;
		LLHTTPClient::get4(url, responder, headers);
	}

	responder->wait();

	S32 http_status = HTTP_INTERNAL_ERROR;
	LLSD response = LLSD::emptyMap();
	CURLcode result = responder->result_code();

	http_status = responder->http_status();
	bool http_success = http_status >= 200 && http_status < 300;
	if (result == CURLE_OK && http_success)
	{
		if (method == HTTP_RAW_GET)
		{
			response["body"] = responder->getRaw();
		}
		else
		{
			response["body"] = responder->getLLSD();
		}
	}
	else if (result == CURLE_OK)
	{
		// We expect 404s, don't spam for them.
		if (http_status != 404)
		{
			llwarns << "CURL REQ URL: " << url << llendl;
			llwarns << "CURL REQ METHOD TYPE: " << method << llendl;
			llwarns << "CURL REQ HEADERS: " << headers << llendl;
			if (method == HTTP_LLSD_POST)
			{
				llwarns << "CURL REQ BODY: " << body.asString() << llendl;
			}
			llwarns << "CURL HTTP_STATUS: " << http_status << llendl;
			if (method == HTTP_RAW_GET)
			{
				llwarns << "CURL ERROR BODY: " << responder->getRaw() << llendl;
			}
			else
			{
				llwarns << "CURL ERROR BODY: " << responder->getLLSD().asString() << llendl;
			}
		}
		if (method == HTTP_RAW_GET)
		{
			response["body"] = responder->getRaw();
		}
		else
		{
			response["body"] = responder->getLLSD().asString();
		}
	}
	else
	{
		response["body"] = responder->reason();
	}

	response["status"] = http_status;
	return response;
}

LLSD LLHTTPClient::blockingPost(const std::string& url, const LLSD& body)
{
	return blocking_request(url, HTTP_LLSD_POST, body);
}

LLSD LLHTTPClient::blockingGet(const std::string& url)
{
	return blocking_request(url, HTTP_LLSD_GET, LLSD());
}

U32 LLHTTPClient::blockingGetRaw(const std::string& url, std::string& body)
{
	LLSD result = blocking_request(url, HTTP_RAW_GET, LLSD());
	body = result["body"].asString();
	return result["status"].asInteger();
}

void LLHTTPClient::put4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_PUT, new LLSDInjector(body), responder, headers);
}

void LLHTTPClient::post4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_POST, new LLSDInjector(body), responder, headers);
}

void LLHTTPClient::postXMLRPC(std::string const& url, XMLRPC_REQUEST xmlrpc_request, ResponderPtr responder, AIHTTPHeaders& headers)
{
  	request(url, LLURLRequest::HTTP_POST, new XMLRPCInjector(xmlrpc_request), responder, headers, true, false);		// Does use compression.
}

void LLHTTPClient::postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder, AIHTTPHeaders& headers)
{
	XMLRPC_REQUEST xmlrpc_request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(xmlrpc_request, method);
	XMLRPC_RequestSetRequestType(xmlrpc_request, xmlrpc_request_call);
	XMLRPC_RequestSetData(xmlrpc_request, value);
	// XMLRPCInjector takes ownership of xmlrpc_request and will free it when done.
	// LLURLRequest takes ownership of the XMLRPCInjector object and will free it when done.
  	request(url, LLURLRequest::HTTP_POST, new XMLRPCInjector(xmlrpc_request), responder, headers, true, true);		// Does not use compression.
}

void LLHTTPClient::postRaw4(std::string const& url, char const* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_POST, new RawInjector(data, size), responder, headers);
}

void LLHTTPClient::postFile4(std::string const& url, std::string const& filename, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_POST, new FileInjector(filename), responder, headers);
}

void LLHTTPClient::postFile4(std::string const& url, LLUUID const& uuid, LLAssetType::EType asset_type, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_POST, new VFileInjector(uuid, asset_type), responder, headers);
}

// static
void LLHTTPClient::del4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_DELETE, NULL, responder, headers);
}

// static
void LLHTTPClient::move4(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers)
{
	headers.addHeader("Destination", destination);
	request(url, LLURLRequest::HTTP_MOVE, NULL, responder, headers);
}
