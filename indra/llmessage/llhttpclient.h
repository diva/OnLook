/** 
 * @file llhttpclient.h
 * @brief Declaration of classes for making HTTP client requests.
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

#ifndef LL_LLHTTPCLIENT_H
#define LL_LLHTTPCLIENT_H

/**
 * These classes represent the HTTP client framework.
 */

#include <string>

#include "llassettype.h"
#include "llcurl.h"
#include "aihttpheaders.h"

class LLUUID;
class LLPumpIO;
class LLSD;
class AIHTTPTimeoutPolicy;

extern AIHTTPTimeoutPolicy responderIgnore_timeout;
typedef struct _xmlrpc_request* XMLRPC_REQUEST;
typedef struct _xmlrpc_value* XMLRPC_VALUE;

class LLHTTPClient
{
public:
	// For convenience
	typedef LLCurl::Responder Responder;
	typedef LLCurl::ResponderPtr ResponderPtr;

	// The default actually already ignores responses.
	class ResponderIgnore : public Responder { virtual AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return responderIgnore_timeout;} };

	/** @name non-blocking API */
	//@{
	static void head4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers);
	static void head4(std::string const& url, ResponderPtr responder)
	    { AIHTTPHeaders headers; head4(url, responder, headers); }

	static void getByteRange4(std::string const& url, S32 offset, S32 bytes, ResponderPtr responder, AIHTTPHeaders& headers);
	static void getByteRange4(std::string const& url, S32 offset, S32 bytes, ResponderPtr responder)
	    { AIHTTPHeaders headers; getByteRange4(url, offset, bytes, responder, headers); }

	static void get4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers);
	static void get4(std::string const& url, ResponderPtr responder)
	    { AIHTTPHeaders headers; get4(url, responder, headers); }

	static void get4(std::string const& url, LLSD const& query, ResponderPtr responder, AIHTTPHeaders& headers);
	static void get4(std::string const& url, LLSD const& query, ResponderPtr responder)
	    { AIHTTPHeaders headers; get4(url, query, responder, headers); }

	static void put4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers);
	static void put4(std::string const& url, LLSD const& body, ResponderPtr responder)
	    { AIHTTPHeaders headers; put4(url, body, responder, headers); }

	static void getHeaderOnly4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers);
	static void getHeaderOnly4(std::string const& url, ResponderPtr responder)
	    { AIHTTPHeaders headers; getHeaderOnly4(url, responder, headers); }

	static void post4(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers);
	static void post4(std::string const& url, LLSD const& body, ResponderPtr responder)
	    { AIHTTPHeaders headers; post4(url, body, responder, headers); }

	/** Takes ownership of request and deletes it when sent */
	static void postXMLRPC(std::string const& url, XMLRPC_REQUEST request, ResponderPtr responder, AIHTTPHeaders& headers);
	static void postXMLRPC(std::string const& url, XMLRPC_REQUEST request, ResponderPtr responder)
	    { AIHTTPHeaders headers; postXMLRPC(url, request, responder, headers); }

	static void postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder, AIHTTPHeaders& headers);
	static void postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder)
	    { AIHTTPHeaders headers; postXMLRPC(url, method, value, responder, headers); }

	/** Takes ownership of data and deletes it when sent */
	static void postRaw4(std::string const& url, const char* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers);
	static void postRaw4(std::string const& url, const char* data, S32 size, ResponderPtr responder)
	    { AIHTTPHeaders headers; postRaw4(url, data, size, responder, headers); }

	static void postFile4(std::string const& url, std::string const& filename, ResponderPtr responder, AIHTTPHeaders& headers);
	static void postFile4(std::string const& url, std::string const& filename, ResponderPtr responder)
	    { AIHTTPHeaders headers; postFile4(url, filename, responder, headers); }

	static void postFile4(std::string const& url, const LLUUID& uuid, LLAssetType::EType asset_type, ResponderPtr responder, AIHTTPHeaders& headers);
	static void postFile4(std::string const& url, const LLUUID& uuid, LLAssetType::EType asset_type, ResponderPtr responder)
	    { AIHTTPHeaders headers; postFile4(url, uuid, asset_type, responder, headers); }

	static void del4(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers);
	static void del4(std::string const& url, ResponderPtr responder)
	    { AIHTTPHeaders headers; del4(url, responder, headers); }

		///< sends a DELETE method, but we can't call it delete in c++
	
	/**
	 * @brief Send a MOVE webdav method
	 *
	 * @param url The complete serialized (and escaped) url to get.
	 * @param destination The complete serialized destination url.
	 * @param responder The responder that will handle the result.
	 * @param headers A map of key:value headers to pass to the request
	 * @param timeout The number of seconds to give the server to respond.
	 */
	static void move4(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers);
	static void move4(std::string const& url, std::string const& destination, ResponderPtr responder)
	    { AIHTTPHeaders headers; move4(url, destination, responder, headers); }

	//@}

	/**
	 * @brief Blocking HTTP get that returns an LLSD map of status and body.
	 *
	 * @param url the complete serialized (and escaped) url to get
	 * @return An LLSD of { 'status':status, 'body':payload }
	 */
	static LLSD blockingGet(std::string const& url);

	/**
	 * @brief Blocking HTTP POST that returns an LLSD map of status and body.
	 *
	 * @param url the complete serialized (and escaped) url to get
	 * @param body the LLSD post body
	 * @return An LLSD of { 'status':status (an int), 'body':payload (an LLSD) }
	 */
	static LLSD blockingPost(std::string const& url, LLSD const& body);
};

#endif // LL_LLHTTPCLIENT_H
