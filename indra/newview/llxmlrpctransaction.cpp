/** 
 * @file llxmlrpctransaction.cpp
 * @brief LLXMLRPCTransaction and related class implementations 
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llxmlrpctransaction.h"

#include "llcurl.h"
#include "llviewercontrol.h"

// Have to include these last to avoid queue redefinition!
#include <xmlrpc-epi/xmlrpc.h>

#include "llappviewer.h"

#include "hippogridmanager.h"
#include "statemachine/aicurleasyrequeststatemachine.h"

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


class LLXMLRPCTransaction::Impl
{
public:
	typedef LLXMLRPCTransaction::Status	Status;

	AICurlEasyRequestStateMachine*	mCurlEasyRequestStateMachinePtr;

	Status		mStatus;
	CURLcode	mCurlCode;
	std::string	mStatusMessage;
	std::string	mStatusURI;
	LLCurl::TransferInfo mTransferInfo;
	
	std::string			mURI;
	
	std::string			mProxyAddress;

	std::string			mResponseText;
	XMLRPC_REQUEST		mResponse;
	
	Impl(const std::string& uri, XMLRPC_REQUEST request, bool useGzip);
	Impl(const std::string& uri,
		 const std::string& method, LLXMLRPCValue params, bool useGzip);
	~Impl();
	
	bool is_finished(void) const;
	void curlEasyRequestCallback(bool success);
	
	void setStatus(Status code,
				   const std::string& message = "", const std::string& uri = "");
	void setCurlStatus(CURLcode);

private:
	void init(XMLRPC_REQUEST request, bool useGzip);

	static size_t curlDownloadCallback(
		char* data, size_t size, size_t nmemb, void* user_data);
};

LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		XMLRPC_REQUEST request, bool useGzip)
	: mCurlEasyRequestStateMachinePtr(NULL),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mResponse(0)
{
	init(request, useGzip);
}


LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip)
	: mCurlEasyRequestStateMachinePtr(NULL),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mResponse(0)
{
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method.c_str());
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
	XMLRPC_RequestSetData(request, params.getValue());
	
	init(request, useGzip);
}

// Store pointer to data allocated with XMLRPC_REQUEST_ToXML and call XMLRPC_Free to free it upon destruction.
class AIXMLRPCData : public AIPostField
{
  public:
	AIXMLRPCData(char const* allocated_data) : AIPostField(allocated_data) { }
	/*virtual*/ ~AIXMLRPCData() { XMLRPC_Free(const_cast<char*>(mData)); mData = NULL; }
};

void LLXMLRPCTransaction::Impl::init(XMLRPC_REQUEST request, bool useGzip)
{
	{
		try
		{
			mCurlEasyRequestStateMachinePtr = new AICurlEasyRequestStateMachine(false);
		}
		catch(AICurlNoEasyHandle const& error)
		{
			llwarns << "Failed to initialize LLXMLRPCTransaction: " << error.what() << llendl;
			setStatus(StatusOtherError, "No curl easy handle");
			return;
		}
		AICurlEasyRequest_wat curlEasyRequest_w(*mCurlEasyRequestStateMachinePtr->mCurlEasyRequest);

		curlEasyRequest_w->setWriteCallback(&curlDownloadCallback, (void*)this);
		BOOL vefifySSLCert = !gSavedSettings.getBOOL("NoVerifySSLCert");
		curlEasyRequest_w->setopt(CURLOPT_SSL_VERIFYPEER, vefifySSLCert);
		curlEasyRequest_w->setopt(CURLOPT_SSL_VERIFYHOST, vefifySSLCert ? 2 : 0);
		// Be a little impatient about establishing connections.
		curlEasyRequest_w->setopt(CURLOPT_CONNECTTIMEOUT, 40L);

		/* Setting the DNS cache timeout to -1 disables it completely.
		   This might help with bug #503 */
		curlEasyRequest_w->setopt(CURLOPT_DNS_CACHE_TIMEOUT, -1L);

		curlEasyRequest_w->addHeader("Content-Type: text/xml");

		if (useGzip)
		{
			curlEasyRequest_w->setoptString(CURLOPT_ENCODING, "");
		}
		
		int requestTextSize;
		char* requestText = XMLRPC_REQUEST_ToXML(request, &requestTextSize);
		if (requestText)
		{
			curlEasyRequest_w->setPost(new AIXMLRPCData(requestText), requestTextSize);
		}
		else
		{
			setStatus(StatusOtherError);
		}

		curlEasyRequest_w->finalizeRequest(mURI);
	}
	if (mStatus == LLXMLRPCTransaction::StatusNotStarted)	// It could be LLXMLRPCTransaction::StatusOtherError.
	{
	  mCurlEasyRequestStateMachinePtr->run(boost::bind(&LLXMLRPCTransaction::Impl::curlEasyRequestCallback, this, _1));
	  setStatus(LLXMLRPCTransaction::StatusStarted);
	}
	else
	{
	  // This deletes the statemachine immediately.
	  mCurlEasyRequestStateMachinePtr->kill();
	  mCurlEasyRequestStateMachinePtr = NULL;
	}
}

LLXMLRPCTransaction::Impl::~Impl()
{
	if (mCurlEasyRequestStateMachinePtr && mCurlEasyRequestStateMachinePtr->running())
	{
		llwarns << "Calling LLXMLRPCTransaction::Impl::~Impl while mCurlEasyRequestStateMachinePtr is still running" << llendl;
		mCurlEasyRequestStateMachinePtr->abort();
	}

	if (mResponse)
	{
		XMLRPC_RequestFree(mResponse, 1);
	}
}

bool LLXMLRPCTransaction::Impl::is_finished(void) const
{
	// Nothing to process anymore. Just wait till the statemachine finished.
	return mStatus != LLXMLRPCTransaction::StatusNotStarted &&
	       mStatus != LLXMLRPCTransaction::StatusStarted &&
		   mStatus != LLXMLRPCTransaction::StatusDownloading;
}

void LLXMLRPCTransaction::Impl::curlEasyRequestCallback(bool success)
{
	llassert(mStatus == LLXMLRPCTransaction::StatusStarted || mStatus == LLXMLRPCTransaction::StatusDownloading);

	AICurlEasyRequestStateMachine* state_machine = mCurlEasyRequestStateMachinePtr;
	// We're done with the statemachine, one way or another.
	// Set mCurlEasyRequestStateMachinePtr to NULL so we won't call mCurlEasyRequestStateMachinePtr->running() in the destructor.
	// Note that the state machine auto-cleaning: it will be deleted by the main-thread after this function returns.
	mCurlEasyRequestStateMachinePtr = NULL;

	if (!success)
	{
		// AICurlEasyRequestStateMachine did abort.
		// This currently only happens when libcurl didn't finish before the timer expired.
		std::ostringstream msg;
		F32 timeout_value = gSavedSettings.getF32("CurlRequestTimeOut");
		msg << "Connection to " << mURI << " timed out (" << timeout_value << " s)!";
		if (timeout_value < 40)
		{
			msg << "\nTry increasing CurlRequestTimeOut in Debug Settings.";
		}
		setStatus(LLXMLRPCTransaction::StatusOtherError, msg.str());
		return;
	}

	AICurlEasyRequest_wat curlEasyRequest_w(*state_machine->mCurlEasyRequest);
	CURLcode result;
	curlEasyRequest_w->getResult(&result, &mTransferInfo);

	if (result != CURLE_OK)
	{
		setCurlStatus(result);
		llwarns << "LLXMLRPCTransaction CURL error "
				<< mCurlCode << ": " << curlEasyRequest_w->getErrorString() << llendl;
		llwarns << "LLXMLRPCTransaction request URI: "
				<< mURI << llendl;
			
		return;
	}
	
	setStatus(LLXMLRPCTransaction::StatusComplete);

	mResponse = XMLRPC_REQUEST_FromXML(
			mResponseText.data(), mResponseText.size(), NULL);

	bool		hasError = false;
	bool		hasFault = false;
	int			faultCode = 0;
	std::string	faultString;

	LLXMLRPCValue error(XMLRPC_RequestGetError(mResponse));
	if (error.isValid())
	{
		hasError = true;
		faultCode = error["faultCode"].asInt();
		faultString = error["faultString"].asString();
	}
	else if (XMLRPC_ResponseIsFault(mResponse))
	{
		hasFault = true;
		faultCode = XMLRPC_GetResponseFaultCode(mResponse);
		faultString = XMLRPC_GetResponseFaultString(mResponse);
	}

	if (hasError || hasFault)
	{
		setStatus(LLXMLRPCTransaction::StatusXMLRPCError);
		
		llwarns << "LLXMLRPCTransaction XMLRPC "
				<< (hasError ? "error " : "fault ")
				<< faultCode << ": "
				<< faultString << llendl;
		llwarns << "LLXMLRPCTransaction request URI: "
				<< mURI << llendl;
	}
}

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

size_t LLXMLRPCTransaction::Impl::curlDownloadCallback(
		char* data, size_t size, size_t nmemb, void* user_data)
{
	Impl& impl(*(Impl*)user_data);
	
	size_t n = size * nmemb;

#ifdef CWDEBUG
	if (n < 80)
	  Dout(dc::curl, "Entering LLXMLRPCTransaction::Impl::curlDownloadCallback(\"" << buf2str(data, n) << "\", " << size << ", " << nmemb << ", " << user_data << ")");
	else
	  Dout(dc::curl, "Entering LLXMLRPCTransaction::Impl::curlDownloadCallback(\"" << buf2str(data, 40) << "\"...\"" << buf2str(data + n - 40, 40) << "\", " << size << ", " << nmemb << ", " << user_data << ")");
#endif

	impl.mResponseText.append(data, n);
	
	if (impl.mStatus == LLXMLRPCTransaction::StatusStarted)
	{
		impl.setStatus(LLXMLRPCTransaction::StatusDownloading);
	}
	
	return n;
}


LLXMLRPCTransaction::LLXMLRPCTransaction(
	const std::string& uri, XMLRPC_REQUEST request, bool useGzip)
: impl(* new Impl(uri, request, useGzip))
{ }


LLXMLRPCTransaction::LLXMLRPCTransaction(
	const std::string& uri,
	const std::string& method, LLXMLRPCValue params, bool useGzip)
: impl(* new Impl(uri, method, params, useGzip))
{ }

LLXMLRPCTransaction::~LLXMLRPCTransaction()
{
	delete &impl;
}

bool LLXMLRPCTransaction::is_finished(void) const
{
	return impl.is_finished();
}

LLXMLRPCTransaction::Status LLXMLRPCTransaction::status(int* curlCode)
{
	if (curlCode)
	{
		*curlCode = 
			(impl.mStatus == StatusCURLError)
				? impl.mCurlCode
				: CURLE_OK;
	}
		
	return impl.mStatus;
}

std::string LLXMLRPCTransaction::statusMessage()
{
	return impl.mStatusMessage;
}

std::string LLXMLRPCTransaction::statusURI()
{
	return impl.mStatusURI;
}

XMLRPC_REQUEST LLXMLRPCTransaction::response()
{
	return impl.mResponse;
}

LLXMLRPCValue LLXMLRPCTransaction::responseValue()
{
	return LLXMLRPCValue(XMLRPC_RequestGetData(impl.mResponse));
}


F64 LLXMLRPCTransaction::transferRate()
{
	if (impl.mStatus != StatusComplete)
	{
		return 0.0L;
	}
	
	double rate_bits_per_sec = impl.mTransferInfo.mSpeedDownload * 8.0;
	
	LL_INFOS("AppInit") << "Buffer size:   " << impl.mResponseText.size() << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer size: " << impl.mTransferInfo.mSizeDownload << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer time: " << impl.mTransferInfo.mTotalTime << " s" << LL_ENDL;
	LL_INFOS("AppInit") << "Transfer rate: " << rate_bits_per_sec / 1000.0 << " Kb/s" << LL_ENDL;

	return rate_bits_per_sec;
}
