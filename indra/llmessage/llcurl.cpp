/**
 * @file llcurl.cpp
 * @author Zero / Donovan
 * @date 2006-10-15
 * @brief Implementation of wrapper around libcurl.
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

//////////////////////////////////////////////////////////////////////////////
/*
	The trick to getting curl to do keep-alives is to reuse the
	same easy handle for the requests.  It appears that curl
	keeps a pool of connections alive for each easy handle, but
	doesn't share them between easy handles.  Therefore it is
	important to keep a pool of easy handles and reuse them,
	rather than create and destroy them with each request.  This
	code does this.

	Furthermore, it would behoove us to keep track of which
	hosts an easy handle was used for and pick an easy handle
	that matches the next request.  This code does not current
	do this.
 */

//////////////////////////////////////////////////////////////////////////////

static const U32 EASY_HANDLE_POOL_SIZE		= 5;
static const S32 MULTI_PERFORM_CALL_REPEAT	= 5;
static const S32 CURL_REQUEST_TIMEOUT = 30; // seconds per operation
static const S32 MAX_ACTIVE_REQUEST_COUNT = 100;

//static
F32      LLCurl::sCurlRequestTimeOut = 120.f; //seconds
S32      LLCurl::sMaxHandles = 256; //max number of handles, (multi handles and easy handles combined).

//////////////////////////////////////////////////////////////////////////////

AIThreadSafeSimpleDC<LLCurl::Easy::Handles> LLCurl::Easy::sHandles;

//static
CURL* LLCurl::Easy::allocEasyHandle()
{
	llassert(*AIAccess<LLCurlThread*>(LLCurl::getCurlThread())) ;

	CURL* ret = NULL;

	//*** Multi-threaded.
	AIAccess<Handles> handles_w(sHandles);

	if (handles_w->free.empty())
	{
		ret = LLCurl::newEasyHandle();
	}
	else
	{
		ret = *(handles_w->free.begin());
		handles_w->free.erase(ret);
	}

	if (ret)
	{
		handles_w->active.insert(ret);
	}

	return ret;
}

//static
void LLCurl::Easy::releaseEasyHandle(CURL* handle)
{
	DoutEntering(dc::curl, "LLCurl::Easy::releaseEasyHandle(" << (void*)handle << ")");
	BACKTRACE;

	static const S32 MAX_NUM_FREE_HANDLES = 32 ;

	if (!handle)
	{
		return ; //handle allocation failed.
		//llerrs << "handle cannot be NULL!" << llendl;
	}

	//*** Multi-Threaded (logout only?)
	AIAccess<Handles> handles_w(sHandles);

	if (handles_w->active.find(handle) != handles_w->active.end())
	{
		handles_w->active.erase(handle);

		if (handles_w->free.size() < MAX_NUM_FREE_HANDLES)
		{
			curl_easy_reset(handle);
			handles_w->free.insert(handle);
		}
		else
		{
			LLCurl::deleteEasyHandle(handle) ;
		}
	}
	else
	{
		llerrs << "Invalid handle." << llendl;
	}
}

LLCurl::Easy::~Easy()
{
	AISTAccess<LLCurl::ResponderPtr> responder_w(mResponder);
	if (*responder_w && LLCurl::getNotQuitting()) //aborted
	{	
		std::string reason("Request timeout, aborted.") ;
		(*responder_w)->completedRaw(408, //HTTP_REQUEST_TIME_OUT, timeout, abort
			reason, mChannels, mOutput);		
	}
	*responder_w = NULL;
}

LLCurl::Easy* LLCurlRequest::allocEasy()
{
	if (!mActiveMulti ||
		mActiveRequestCount	>= MAX_ACTIVE_REQUEST_COUNT ||
		mActiveMulti->mErrorCount > 0)
	{
		addMulti();
	}
	if(!mActiveMulti)
	{
		return NULL ;
	}

	//llassert_always(mActiveMulti);
	++mActiveRequestCount;
	LLCurl::Easy* easy = mActiveMulti->allocEasy();
	return easy;
}
