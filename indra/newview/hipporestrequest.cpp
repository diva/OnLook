
#include "llviewerprecompiledheaders.h"

#include "hipporestrequest.h"

#ifndef CURL_STATICLIB
#define CURL_STATICLIB 1
#endif

#include <stdtypes.h>
#include "llbufferstream.h"
#include "llerror.h"
#include "llhttpclient.h"
#include "llurlrequest.h"
#include "llxmltree.h"

#include <curl/curl.h>
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif

// ********************************************************************
 
 
class HippoRestComplete /* AIFIXME: public LLURLRequestComplete*/
{
	public:
		HippoRestComplete(HippoRestHandler *handler) :
			mHandler(handler),
			mStatus(499),
			mReason("Request completed w/o status")
		{
		}
 
		~HippoRestComplete()
		{
			delete mHandler;
		}
 
		// Called once for each header received, prior to httpStatus
		void header(const std::string& header, const std::string& value)
		{
			mHandler->addHeader(header, value);
		}

#if 0 // AIFIXME: doesn't compile
		// Always called on request completion, prior to complete
		void httpStatus(U32 status, const std::string& reason)
		{
			LLURLRequestComplete::httpStatus(status, reason);
			mStatus = status;
			mReason = reason;
		}

		void complete(const LLChannelDescriptors &channels, const buffer_ptr_t &buffer)
		{
			mHandler->handle(mStatus, mReason, channels, buffer);
		}
#endif

	private:
		HippoRestHandler *mHandler;
		int mStatus;
		std::string mReason;
};


// ********************************************************************


static std::string gEmptyString;

void HippoRestHandler::addHeader(const std::string &header, const std::string &content)
{
	mHeaders[header] = content;
}

bool HippoRestHandler::hasHeader(const std::string &header) const
{
	return (mHeaders.find(header) != mHeaders.end());
}

const std::string &HippoRestHandler::getHeader(const std::string &header) const
{
	std::map<std::string, std::string>::const_iterator it;
	it = mHeaders.find(header);
	if (it != mHeaders.end()) {
		return it->second;
	} else {
		return gEmptyString;
	}
}


// ********************************************************************


void HippoRestHandlerRaw::handle(int status, const std::string &reason,
								 const LLChannelDescriptors &channels,
								 const boost::shared_ptr<LLBufferArray> &body)
{
	if (status == 200) {
		std::string data;
		LLBufferArray *buffer = body.get();
		LLBufferArray::segment_iterator_t it, end = buffer->endSegment();
		for (it=buffer->beginSegment(); it!=end; ++it)
			if (it->isOnChannel(channels.in()))
				data.append((char*)it->data(), it->size());
		result(data);
	} else {
		llwarns << "Rest request error " << status << ": " << reason << llendl;
	}
}

void HippoRestHandlerXml::handle(int status, const std::string &reason,
								 const LLChannelDescriptors &channels,
								 const boost::shared_ptr<LLBufferArray> &body)
{
	if (status == 200) {
		LLXmlTree *tree = new LLXmlTree();
		bool success = tree->parseBufferStart();
		LLBufferArray *buffer = body.get();
		LLBufferArray::segment_iterator_t it, end = buffer->endSegment();
		for (it=buffer->beginSegment(); success && (it!=end); ++it)
			if (it->isOnChannel(channels.in()))
				success = success && tree->parseBuffer((char*)it->data(), it->size());
		success = success && tree->parseBufferFinalize();
		if (success) result(tree);
		delete tree;
	} else {
		llwarns << "Rest request error " << status << ": " << reason << llendl;
	}
}


// ********************************************************************

class BodyDataRaw : public Injector
{
	public:
		explicit BodyDataRaw(const std::string &data) : mData(data) { }
 
		/*virtual*/ char const* contentType(void) const { return "application/octet-stream"; }
 
		/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
		{
			LLBufferStream ostream(channels, buffer.get());
			ostream.write(mData.data(), mData.size());
			ostream << std::flush;
			return mData.size();
		}

	private:
		std::string mData;
};

class BodyDataXml : public Injector
{
	public:
		explicit BodyDataXml(const LLXmlTree *tree) :
			mTree(tree)
		{
		}

		virtual ~BodyDataXml()
		{
			if (mTree) delete mTree;
		}

		/*virtual*/ char const* contentType(void) const { return "application/xml"; }

		/*virtual*/ U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
		{
			std::string data;
			mTree->write(data);
			LLBufferStream ostream(channels, buffer.get());
			ostream.write(data.data(), data.size());
			ostream << std::flush;
			return data.size();
		}

	private:
		const LLXmlTree *mTree;
};


// ********************************************************************


static void request(const std::string &url,
					LLURLRequest::ERequestAction method,
					Injector *body,
					HippoRestHandler *handler, float timeout);


// static
void HippoRestRequest::get5(const std::string &url,
						   HippoRestHandler *handler, float timeout)
{
	request(url, LLURLRequest::HTTP_GET, 0, handler, timeout);
}

// static
void HippoRestRequest::put5(const std::string &url, const std::string &body,
						   HippoRestHandler *handler, float timeout)
{
	request(url, LLURLRequest::HTTP_PUT, new BodyDataRaw(body), handler, timeout);
}

// static
void HippoRestRequest::put5(const std::string &url, const LLXmlTree *body,
						   HippoRestHandler *handler, float timeout)
{
	request(url, LLURLRequest::HTTP_PUT, new BodyDataXml(body), handler, timeout);
}

// static
void HippoRestRequest::post5(const std::string &url, const std::string &body,
							HippoRestHandler *handler, float timeout)
{
	request(url, LLURLRequest::HTTP_POST, new BodyDataRaw(body), handler, timeout);
}

// static
void HippoRestRequest::post5(const std::string &url, const LLXmlTree *body,
							HippoRestHandler *handler, float timeout)
{
	request(url, LLURLRequest::HTTP_POST, new BodyDataXml(body), handler, timeout);
}


// ********************************************************************


static void request(const std::string &url,
					LLURLRequest::ERequestAction method,
					Injector *body,
					HippoRestHandler *handler, float timeout)
{
	LLURLRequest *req;
	try
	{
		AIHTTPHeaders empty_headers;
		//AIFIXME (doesn't compile): req = new LLURLRequest(method, url, body, handler, empty_headers);
	}
	catch(AICurlNoEasyHandle const& error)
	{
		llwarns << "Failed to create LLURLRequest: " << error.what() << llendl;
		return;
	}
	// Already done by default. req->checkRootCertificate(true);

	/*
	// Insert custom headers if the caller sent any
	if (headers.isMap())
	{
		LLSD::map_const_iterator iter = headers.beginMap();
		LLSD::map_const_iterator end  = headers.endMap();
 
		for (; iter != end; ++iter)
		{
			std::ostringstream header;
			//if the header is "Pragma" with no value
			//the caller intends to force libcurl to drop
			//the Pragma header it so gratuitously inserts
			//Before inserting the header, force libcurl
			//to not use the proxy (read: llurlrequest.cpp)
			static const std::string PRAGMA("Pragma");
			if ((iter->first == PRAGMA) && (iter->second.asString().empty()))
			{
				req->useProxy(false);
			}
			header << iter->first << ": " << iter->second.asString() ;
			lldebugs << "header = " << header.str() << llendl;
			req->addHeader(header.str().c_str());
		}
	}
	*/

	if ((method != LLURLRequest::HTTP_PUT) && (method != LLURLRequest::HTTP_POST)) {
		std::string accept = "Accept: ";
		accept += handler->getAcceptMimeType();
		//AIFIXME req is not defined: req->addHeader(accept.c_str());
	}

	//AIFIXME: req->setCallback(new HippoRestComplete(handler));

	if ((method == LLURLRequest::HTTP_PUT) || (method == LLURLRequest::HTTP_POST)) {
		std::string content = "Content-Type: ";
		content += body->contentType();
		//AIFIXME req is not defined: req->addHeader(content.c_str());
		//AIFIXME: chain.push_back(LLIOPipe::ptr_t(body));
	}

	//AIFIXME: chain.push_back(LLIOPipe::ptr_t(req));
	//LLHTTPClient::getPump().addChain(chain, timeout);
}


// ********************************************************************


static size_t curlWrite(void *ptr, size_t size, size_t nmemb, void *userData)
{
	std::string *result = (std::string*)userData;
	size_t bytes = (size * nmemb);
	result->append((char*)ptr, bytes);
	return nmemb;
}


// static
int HippoRestRequest::getBlocking(const std::string &url, std::string *result)
{
	llinfos << "Requesting: " << url << llendl;

	char curlErrorBuffer[CURL_ERROR_SIZE];
	CURL* curlp = curl_easy_init();
	llassert_always(curlp);

	curl_easy_setopt(curlp, CURLOPT_NOSIGNAL, 1);	// don't use SIGALRM for timeouts
	curl_easy_setopt(curlp, CURLOPT_TIMEOUT, 30);	// seconds (including DNS lookups)
	curl_easy_setopt(curlp, CURLOPT_CAINFO, gDirUtilp->getCAFile().c_str());

	curl_easy_setopt(curlp, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curlp, CURLOPT_WRITEDATA, result);
	curl_easy_setopt(curlp, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlp, CURLOPT_ERRORBUFFER, curlErrorBuffer);
	curl_easy_setopt(curlp, CURLOPT_FAILONERROR, 1);

	*result = "";
	S32 curlSuccess = curl_easy_perform(curlp);
	long httpStatus = 499L;		// curl_easy_getinfo demands pointer to long.
	curl_easy_getinfo(curlp, CURLINFO_RESPONSE_CODE, &httpStatus);

	if (curlSuccess != 0) {
		llwarns << "CURL ERROR (HTTP Status " << httpStatus << "): " << curlErrorBuffer << llendl;
	} else if (httpStatus != 200) {
		llwarns << "HTTP Error " << httpStatus << ", but no Curl error." << llendl;
	}
	
	curl_easy_cleanup(curlp);
	return httpStatus;
}

