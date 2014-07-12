/** 
 * @file llxmlrpcresponder.cpp
 * @brief LLXMLRPCResponder and related class implementations 
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * Copyright (c) 2012, Aleric Inglewood.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llxmlrpcresponder.h"
#include "llhttpclient.h"

#include "llcurl.h"
#include "llviewercontrol.h"
#include "llbufferstream.h"

// Have to include these last to avoid queue redefinition!
#include <xmlrpc-epi/xmlrpc.h>

#include "llappviewer.h"

#include "hippogridmanager.h"
#include "aicurleasyrequeststatemachine.h"

#ifdef CWDEBUG
#include <libcwd/buf2str.h>
#endif

LLXMLRPCValue LLXMLRPCValue::operator[](const char* id) const
{
	return LLXMLRPCValue(XMLRPC_VectorGetValueWithID(mV, id));
}

std::string LLXMLRPCValue::asString() const
{
	const char* s = XMLRPC_GetValueString(mV);
	return s ? s : "";
}

int		LLXMLRPCValue::asInt() const	{ return XMLRPC_GetValueInt(mV); }
bool	LLXMLRPCValue::asBool() const	{ return XMLRPC_GetValueBoolean(mV) != 0; }
double	LLXMLRPCValue::asDouble() const	{ return XMLRPC_GetValueDouble(mV); }

LLXMLRPCValue LLXMLRPCValue::rewind()
{
	return LLXMLRPCValue(XMLRPC_VectorRewind(mV));
}

LLXMLRPCValue LLXMLRPCValue::next()
{
	return LLXMLRPCValue(XMLRPC_VectorNext(mV));
}

bool LLXMLRPCValue::isValid() const
{
	return mV != NULL;
}

LLXMLRPCValue LLXMLRPCValue::createArray()
{
	return LLXMLRPCValue(XMLRPC_CreateVector(NULL, xmlrpc_vector_array));
}

LLXMLRPCValue LLXMLRPCValue::createStruct()
{
	return LLXMLRPCValue(XMLRPC_CreateVector(NULL, xmlrpc_vector_struct));
}


void LLXMLRPCValue::append(LLXMLRPCValue& v)
{
	XMLRPC_AddValueToVector(mV, v.mV);
}

void LLXMLRPCValue::appendString(const std::string& v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueString(NULL, v.c_str(), 0));
}

void LLXMLRPCValue::appendInt(int v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueInt(NULL, v));
}

void LLXMLRPCValue::appendBool(bool v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueBoolean(NULL, v));
}

void LLXMLRPCValue::appendDouble(double v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueDouble(NULL, v));
}


void LLXMLRPCValue::append(const char* id, LLXMLRPCValue& v)
{
	XMLRPC_SetValueID(v.mV, id, 0);
	XMLRPC_AddValueToVector(mV, v.mV);
}

void LLXMLRPCValue::appendString(const char* id, const std::string& v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueString(id, v.c_str(), 0));
}

void LLXMLRPCValue::appendInt(const char* id, int v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueInt(id, v));
}

void LLXMLRPCValue::appendBool(const char* id, bool v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueBoolean(id, v));
}

void LLXMLRPCValue::appendDouble(const char* id, double v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueDouble(id, v));
}

void LLXMLRPCValue::cleanup()
{
	XMLRPC_CleanupValue(mV);
	mV = NULL;
}

XMLRPC_VALUE LLXMLRPCValue::getValue() const
{
	return mV;
}

void XMLRPCResponder::completed_headers(U32 status, std::string const& reason, AITransferInfo* info)
{
	if (info)
	{
		mTransferInfo = *info;
	}
	// Call base class implementation.
	ResponderWithCompleted::completed_headers(status, reason, info);
}

void XMLRPCResponder::completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
{
	if (mCode == CURLE_OK && !is_internal_http_error(mStatus))
	{
		mBufferSize = buffer->count(channels.in());
		if (200 <= mStatus && mStatus < 400)
		{
			char* ptr = NULL;
			char* buf = NULL;
			LLMutexLock lock(buffer->getMutex());
			LLBufferArray::const_segment_iterator_t const end = buffer->endSegment();
			for (LLBufferArray::const_segment_iterator_t iter = buffer->beginSegment(); iter != end; ++iter)
			{
				LLSegment const& segment = *iter;
				if (segment.isOnChannel(channels.in()))
				{
					S32 const segment_size = segment.size();
					if (!buf)
					{
						if (segment_size == mBufferSize)
						{
							// It's contiguous, no need for copying.
							mResponse = XMLRPC_REQUEST_FromXML((char const*)segment.data(), mBufferSize, NULL);
							break;
						}
						ptr = buf = new char [mBufferSize];
					}
					llassert(ptr + segment_size <= buf + mBufferSize);
					memcpy(ptr, segment.data(), segment_size);
					ptr += segment_size;
				}
			}
			if (buf)
			{
				mResponse = XMLRPC_REQUEST_FromXML(buf, mBufferSize, NULL);
				delete [] buf;
			}
		}
	}
}

LLXMLRPCValue XMLRPCResponder::responseValue(void) const
{
	return LLXMLRPCValue(XMLRPC_RequestGetData(mResponse));
}

#ifdef AI_UNUSED
void LLXMLRPCTransaction::Impl::setStatus(Status status,
	const std::string& message, const std::string& uri)
{
	mStatus = status;
	mStatusMessage = message;
	mStatusURI = uri;

	if (mStatusMessage.empty())
	{
		switch (mStatus)
		{
			case StatusNotStarted:
				mStatusMessage = "(not started)";
				break;
				
			case StatusStarted:
				mStatusMessage = "(waiting for server response)";
				break;
				
			case StatusDownloading:
				mStatusMessage = "(reading server response)";
				break;
				
			case StatusComplete:
				mStatusMessage = "(done)";
				break;
				
			default:
				// Usually this means that there's a problem with the login server,
				// not with the client.  Direct user to status page.
				// NOTE: these should really be gHippoGridManager->getCurrentGrid()->getGridName()
				// but apparently that's broken as of 1.3 b2 -- MC
				mStatusMessage =
					"Despite our best efforts, something unexpected has gone wrong. \n"
					" \n"
					"Please check " + gHippoGridManager->getCurrentGrid()->getGridName() + "'s status \n"
					"to see if there is a known problem with the service.";

				//mStatusURI = "http://secondlife.com/status/";
		}
	}
}

void LLXMLRPCTransaction::Impl::setCurlStatus(CURLcode code)
{
	std::string message;
	std::string uri = gHippoGridManager->getCurrentGrid()->getSupportUrl();
	
	switch (code)
	{
		case CURLE_COULDNT_RESOLVE_HOST:
			message =
				"DNS could not resolve the host name.\n"
				"Please verify that you can connect to " + gHippoGridManager->getCurrentGrid()->getGridName() + "'s\n"
				"web site.  If you can, but continue to receive this error,\n"
				"please go to the support section and report this problem.";
			break;
			
		case CURLE_SSL_PEER_CERTIFICATE:
			message =
				"The login server couldn't verify itself via SSL.\n"
				"If you continue to receive this error, please go\n"
				"to the Support section of " + gHippoGridManager->getCurrentGrid()->getGridName() + "'s web site\n"
				"and report the problem.";
			break;
			
		case CURLE_SSL_CACERT:
		case CURLE_SSL_CONNECT_ERROR:
			message =
				"Often this means that your computer's clock is set incorrectly.\n"
				"Please go to Control Panels and make sure the time and date\n"
				"are set correctly.\n"
				"\n"
				"If you continue to receive this error, please go\n"
				"to the Support section of " + gHippoGridManager->getCurrentGrid()->getGridName() + "'s web site\n"
				"and report the problem.";
			break;
			
		default:
				break;
	}
	
	mCurlCode = code;
	setStatus(StatusCURLError, message, uri);
}
#endif // AI_UNUSED

F64 XMLRPCResponder::transferRate(void) const
{
	if (mTransferInfo.mSpeedDownload == 0.0)		// Don't print the below stats if this wasn't initialized.
	{
		return 0.0;
	}
	
	F64 rate_bits_per_sec = mTransferInfo.mSpeedDownload * 8.0;
	
	LL_INFOS("AppInit") << "Buffer size:   " << mBufferSize << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer size: " << mTransferInfo.mSizeDownload << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer time: " << mTransferInfo.mTotalTime << " s" << LL_ENDL;
	LL_INFOS("AppInit") << "Transfer rate: " << rate_bits_per_sec / 1000.0 << " kb/s" << LL_ENDL;

	return rate_bits_per_sec;
}

