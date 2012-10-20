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

#include "llhttpclient.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "llvfile.h"
#include "llurlrequest.h"

class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy blockingGet_timeout;
extern AIHTTPTimeoutPolicy blockingPost_timeout;

////////////////////////////////////////////////////////////////////////////

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

// A simple class for managing data returned from a curl http request.
class LLHTTPBuffer
{
public:
	LLHTTPBuffer() { }

	static size_t curl_write(char* ptr, size_t size, size_t nmemb, void* user_data)
	{
		LLHTTPBuffer* self = (LLHTTPBuffer*)user_data;
		
		size_t bytes = (size * nmemb);
		self->mBuffer.append(ptr,bytes);
		return nmemb;
	}

	LLSD asLLSD()
	{
		LLSD content;

		if (mBuffer.empty()) return content;
		
		std::istringstream istr(mBuffer);
		LLSDSerialize::fromXML(content, istr);
		return content;
	}

	std::string asString()
	{
		return mBuffer;
	}

private:
	std::string mBuffer;
};

// These calls are blocking! This is usually bad, unless you're a dataserver. Then it's awesome.

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
	LLURLRequest::ERequestAction method,
	LLSD const& body,
	AIHTTPHeaders& headers,
	AIHTTPTimeoutPolicy const& timeout)
{
	lldebugs << "blockingRequest of " << url << llendl;

	S32 http_status = 499;
	LLSD response = LLSD::emptyMap();

#if 0 // AIFIXME: rewrite to use AICurlEasyRequestStateMachine
	try
	{
		AICurlEasyRequest easy_request(false);
		AICurlEasyRequest_wat curlEasyRequest_w(*easy_request);

		LLHTTPBuffer http_buffer;
		
		// * Set curl handle options
		curlEasyRequest_w->setopt(CURLOPT_TIMEOUT, (long)timeout);	// seconds, see warning at top of function.
		curlEasyRequest_w->setWriteCallback(&LLHTTPBuffer::curl_write, &http_buffer);

		// * Setup headers.
		if (headers.isMap())
		{
			LLSD::map_const_iterator iter = headers.beginMap();
			LLSD::map_const_iterator end  = headers.endMap();
			for (; iter != end; ++iter)
			{
				std::ostringstream header;
				header << iter->first << ": " << iter->second.asString() ;
				lldebugs << "header = " << header.str() << llendl;
				curlEasyRequest_w->addHeader(header.str().c_str());
			}
		}
		
		// Needs to stay alive until after the call to perform().
		std::ostringstream ostr;

		// * Setup specific method / "verb" for the URI (currently only GET and POST supported + poppy)
		if (method == LLURLRequest::HTTP_GET)
		{
			curlEasyRequest_w->setopt(CURLOPT_HTTPGET, 1);
		}
		else if (method == LLURLRequest::HTTP_POST)
		{
			//copied from PHP libs, correct?
			curlEasyRequest_w->addHeader("Content-Type: application/llsd+xml");
			LLSDSerialize::toXML(body, ostr);
			curlEasyRequest_w->setPost(ostr.str().c_str(), ostr.str().length());
		}
		
		// * Do the action using curl, handle results
		curlEasyRequest_w->addHeader("Accept: application/llsd+xml");
		curlEasyRequest_w->finalizeRequest(url);

		S32 curl_success = curlEasyRequest_w->perform();
		curlEasyRequest_w->getinfo(CURLINFO_RESPONSE_CODE, &http_status);
		// if we get a non-404 and it's not a 200 OR maybe it is but you have error bits,
		if ( http_status != 404 && (http_status != 200 || curl_success != 0) )
		{
			// We expect 404s, don't spam for them.
			llwarns << "CURL REQ URL: " << url << llendl;
			llwarns << "CURL REQ METHOD TYPE: " << LLURLRequest::actionAsVerb(method) << llendl;
			llwarns << "CURL REQ HEADERS: " << headers.asString() << llendl;
			llwarns << "CURL REQ BODY: " << ostr.str() << llendl;
			llwarns << "CURL HTTP_STATUS: " << http_status << llendl;
			llwarns << "CURL ERROR BODY: " << http_buffer.asString() << llendl;
			response["body"] = http_buffer.asString();
		}
		else
		{
			response["body"] = http_buffer.asLLSD();
			lldebugs << "CURL response: " << http_buffer.asString() << llendl;
		}
	}
	catch(AICurlNoEasyHandle const& error)
	{
		response["body"] = error.what();
	}
#endif

	response["status"] = http_status;
	return response;
}

LLSD LLHTTPClient::blockingGet(const std::string& url)
{
	AIHTTPHeaders empty_headers;
	return blocking_request(url, LLURLRequest::HTTP_GET, LLSD(), empty_headers, blockingGet_timeout);
}

LLSD LLHTTPClient::blockingPost(const std::string& url, const LLSD& body)
{
	AIHTTPHeaders empty_headers;
	return blocking_request(url, LLURLRequest::HTTP_POST, body, empty_headers, blockingPost_timeout);
}

void LLHTTPClient::put4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_PUT, new LLSDInjector(body), responder, headers);
}

void LLHTTPClient::post4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers)
{
	request(url, LLURLRequest::HTTP_POST, new LLSDInjector(body), responder, headers);
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
