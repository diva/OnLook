/**
 * @file llviewermedia.cpp
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewermedia.h"

#include "llappviewer.h"
#include "lldir.h"
#include "lldiriterator.h"
#include "llevent.h"		// LLSimpleListener
#include "llhoverview.h"
#include "llkeyboard.h"
#include "llmimetypes.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpluginclassmedia.h"
#include "llplugincookiestore.h"
#include "llurldispatcher.h"
#include "lluuid.h"
#include "llviewermediafocus.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llvieweraudio.h"
#include "llweb.h"
#include "llwebprofile.h"

#include "llfloateravatarinfo.h"	// for getProfileURL() function
//#include "viewerversion.h"

class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy mimeDiscoveryResponder_timeout;
extern AIHTTPTimeoutPolicy viewerMediaOpenIDResponder_timeout;
extern AIHTTPTimeoutPolicy viewerMediaWebProfileResponder_timeout;

// Merov: Temporary definitions while porting the new viewer media code to Snowglobe
const int LEFT_BUTTON  = 0;
const int RIGHT_BUTTON = 1;

///////////////////////////////////////////////////////////////////////////////
// Helper class that tries to download a URL from a web site and calls a method
// on the Panel Land Media and to discover the MIME type
class LLMimeDiscoveryResponder : public LLHTTPClient::ResponderHeadersOnly
{
LOG_CLASS(LLMimeDiscoveryResponder);
public:
	LLMimeDiscoveryResponder(viewer_media_t media_impl, std::string const& default_mime_type)
		: mMediaImpl(media_impl),
		  mDefaultMimeType(default_mime_type),
		  mInitialized(false)
	{}

	/*virtual*/ void completedHeaders(U32 status, std::string const& reason, AIHTTPReceivedHeaders const& headers)
	{
		if (200 <= status && status < 300 || status == 405)		// Using HEAD may result in a 405 METHOD NOT ALLOWED, but still have the right Content-TYpe header.
		{
			std::string media_type;
			if (headers.getFirstValue("content-type", media_type))
			{
				std::string::size_type idx1 = media_type.find_first_of(";");
				std::string mime_type = media_type.substr(0, idx1);
				completeAny(status, mime_type);
				return;
			}
			if (200 <= status && status < 300)
			{
				llwarns << "LLMimeDiscoveryResponder::completedHeaders: OK HTTP status (" << status << ") but no Content-Type! Received headers: " << headers << llendl;
			}
		}
		llwarns << "LLMimeDiscoveryResponder::completedHeaders: Got status " << status << ". Using default mime-type: " << mDefaultMimeType << llendl;
		completeAny(status, mDefaultMimeType);
	}

	void completeAny(U32 status, const std::string& mime_type)
	{
		if(!mInitialized && ! mime_type.empty())
		{
			if (mMediaImpl->initializeMedia(mime_type))
			{
				mInitialized = true;
				mMediaImpl->play();
			}
		}
	}

	/*virtual*/ AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return mimeDiscoveryResponder_timeout; }

	public:
		viewer_media_t mMediaImpl;
		std::string mDefaultMimeType;
		bool mInitialized;
};

class LLViewerMediaOpenIDResponder : public LLHTTPClient::ResponderWithCompleted
{
LOG_CLASS(LLViewerMediaOpenIDResponder);
public:
	LLViewerMediaOpenIDResponder( )
	{
	}

	~LLViewerMediaOpenIDResponder()
	{
	}

	/* virtual */ bool needsHeaders(void) const { return true; }

	/* virtual */ void completedHeaders(U32 status, std::string const& reason, AIHTTPReceivedHeaders const& headers)
	{
		LL_DEBUGS("MediaAuth") << "status = " << status << ", reason = " << reason << LL_ENDL;
		LL_DEBUGS("MediaAuth") << headers << LL_ENDL;
		LLViewerMedia::openIDCookieResponse(get_cookie("agni_sl_session_id"));
	}

	/* virtual */ void completedRaw(
		U32 status,
		const std::string& reason,
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		// This is just here to disable the default behavior (attempting to parse the response as llsd).
		// We don't care about the content of the response, only the set-cookie header.
	}

	virtual AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return viewerMediaOpenIDResponder_timeout; }
};

class LLViewerMediaWebProfileResponder : public LLHTTPClient::ResponderWithCompleted
{
LOG_CLASS(LLViewerMediaWebProfileResponder);
public:
	LLViewerMediaWebProfileResponder(std::string host)
	{
		mHost = host;
	}

	~LLViewerMediaWebProfileResponder()
	{
	}

	/* virtual */ bool followRedir(void) const { return true; }
	/* virtual */ bool needsHeaders(void) const { return true; }

	/* virtual */ void completedHeaders(U32 status, std::string const& reason, AIHTTPReceivedHeaders const& headers)
	{
		LL_INFOS("MediaAuth") << "status = " << status << ", reason = " << reason << LL_ENDL;
		LL_INFOS("MediaAuth") << headers << LL_ENDL;

		bool found = false;
		AIHTTPReceivedHeaders::range_type cookies;
		if (headers.getValues("set-cookie", cookies))
		{
			for (AIHTTPReceivedHeaders::iterator_type cookie = cookies.first; cookie != cookies.second; ++cookie)
			{
			  LLViewerMedia::getCookieStore()->setCookiesFromHost(cookie->second, mHost);

			  std::string key = cookie->second.substr(0, cookie->second.find('='));
			  if (key == "_my_secondlife_session")
			  {
				// Set cookie for snapshot publishing.
				std::string auth_cookie = cookie->second.substr(0, cookie->second.find(";")); // strip path
				LL_INFOS("MediaAuth") << "Setting openID auth cookie \"" << auth_cookie << "\"." << LL_ENDL;
				LLWebProfile::setAuthCookie(auth_cookie);
				found = true;
				break;
			  }
			}
		}
		if (!found)
		{
			llwarns << "LLViewerMediaWebProfileResponder did not receive a session ID cookie \"_my_secondlife_session\"! OpenID authentications will fail!" << llendl;
		}
	}

	void completedRaw(
		U32 status,
		const std::string& reason,
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		// This is just here to disable the default behavior (attempting to parse the response as llsd).
		// We don't care about the content of the response, only the set-cookie header.
	}

	virtual AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return viewerMediaWebProfileResponder_timeout; }

	std::string mHost;
};

LLPluginCookieStore *LLViewerMedia::sCookieStore = NULL;
LLURL LLViewerMedia::sOpenIDURL;
std::string LLViewerMedia::sOpenIDCookie;
typedef std::list<LLViewerMediaImpl*> impl_list;
static impl_list sViewerMediaImplList;
static std::string sUpdatedCookies;
static const char *PLUGIN_COOKIE_FILE_NAME = "plugin_cookies.txt";

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMedia

//////////////////////////////////////////////////////////////////////////////////////////
// static
viewer_media_t LLViewerMedia::newMediaImpl(const std::string& media_url,
											 const LLUUID& texture_id,
											 S32 media_width, 
											 S32 media_height, 
											 U8 media_auto_scale,
											 U8 media_loop,
											 std::string mime_type)
{
	LLViewerMediaImpl* media_impl = getMediaImplFromTextureID(texture_id);
	if(media_impl == NULL || texture_id.isNull())
	{
		// Create the media impl
		media_impl = new LLViewerMediaImpl(media_url, texture_id, media_width, media_height, media_auto_scale, media_loop, mime_type);
		sViewerMediaImplList.push_back(media_impl);
	}
	else
	{
		media_impl->stop();
		media_impl->mTextureId = texture_id;
		media_impl->mMediaURL = media_url;
		media_impl->mMediaWidth = media_width;
		media_impl->mMediaHeight = media_height;
		media_impl->mMediaAutoScale = media_auto_scale;
		media_impl->mMediaLoop = media_loop;
		if(! media_url.empty())
			media_impl->navigateTo(media_url, mime_type, true);
	}
	return media_impl;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::removeMedia(LLViewerMediaImpl* media)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	
	for(; iter != end; iter++)
	{
		if(media == *iter)
		{
			sViewerMediaImplList.erase(iter);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
LLViewerMediaImpl* LLViewerMedia::getMediaImplFromTextureID(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* media_impl = *iter;
		if(media_impl->getMediaTextureID() == texture_id)
		{
			return media_impl;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
std::string LLViewerMedia::getCurrentUserAgent()
{
	// Don't include version, channel, or skin -- MC

	// Don't use user-visible string to avoid 
	// punctuation and strange characters.
	//std::string skin_name = gSavedSettings.getString("SkinCurrent");

	// Just in case we need to check browser differences in A/B test
	// builds.
	//std::string channel = gSavedSettings.getString("VersionChannelName");

	// append our magic version number string to the browser user agent id
	// See the HTTP 1.0 and 1.1 specifications for allowed formats:
	// http://www.ietf.org/rfc/rfc1945.txt section 10.15
	// http://www.ietf.org/rfc/rfc2068.txt section 3.8
	// This was also helpful:
	// http://www.mozilla.org/build/revised-user-agent-strings.html
	std::ostringstream codec;
	codec << "SecondLife/";
	codec << "C64 Basic V2";
	//codec << ViewerVersion::getImpMajorVersion() << "." << ViewerVersion::getImpMinorVersion() << "." << ViewerVersion::getImpPatchVersion() << " " << ViewerVersion::getImpTestVersion();
 	//codec << " (" << channel << "; " << skin_name << " skin)";
// 	llinfos << codec.str() << llendl;
	
	return codec.str();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateBrowserUserAgent()
{
	std::string user_agent = getCurrentUserAgent();
	
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin && plugin->pluginSupportsMediaBrowser())
		{
			plugin->setBrowserUserAgent(user_agent);
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::handleSkinCurrentChanged(const LLSD& /*newvalue*/)
{
	// gSavedSettings is already updated when this function is called.
	updateBrowserUserAgent();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::textureHasMedia(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->getMediaTextureID() == texture_id)
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setVolume(F32 volume)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->setVolume(volume);
		}
		else
		{
			llwarns << "Plug-in already destroyed" << llendl;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateMedia()
{
	sUpdatedCookies = getCookieStore()->getChangedCookies();
	if(!sUpdatedCookies.empty())
	{
		lldebugs << "updated cookies will be sent to all loaded plugins: " << llendl;
		lldebugs << sUpdatedCookies << llendl;
	}
	
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->update();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::clearAllCookies()
{
	// Clear all cookies for all plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->clear_cookies();
		}
	}
	
	// Clear all cookies from the cookie store
	getCookieStore()->setAllCookies("");

	// FIXME: this may not be sufficient, since the on-disk cookie file won't get written until some browser instance exits cleanly.
	// It also won't clear cookies for other accounts, or for any account if we're not logged in, and won't do anything at all if there are no webkit plugins loaded.
	// Until such time as we can centralize cookie storage, the following hack should cover these cases:
	
	// HACK: Look for cookie files in all possible places and delete them.
	// NOTE: this assumes knowledge of what happens inside the webkit plugin (it's what adds 'browser_profile' to the path and names the cookie file)
	
	// Places that cookie files can be:
	// <getOSUserAppDir>/browser_profile/cookies
	// <getOSUserAppDir>/first_last/browser_profile/cookies  (note that there may be any number of these!)
	// <getOSUserAppDir>/first_last/plugin_cookies.txt  (note that there may be any number of these!)
	
	std::string base_dir = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter();
	std::string target;
	std::string filename;
	
	lldebugs << "base dir = " << base_dir << llendl;

	// The non-logged-in version is easy
	target = base_dir;
	target += "browser_profile";
	target += gDirUtilp->getDirDelimiter();
	target += "cookies";
	lldebugs << "target = " << target << llendl;
	if(LLFile::isfile(target))
	{
		LLFile::remove(target);
	}
	
	// the hard part: iterate over all user directories and delete the cookie file from each one
	LLDirIterator dir_iter(base_dir, "*_*");
	while (dir_iter.next(filename))
	{
		target = base_dir;
		target += filename;
		target += gDirUtilp->getDirDelimiter();
		target += "browser_profile";
		target += gDirUtilp->getDirDelimiter();
		target += "cookies";
		lldebugs << "target = " << target << llendl;
		if(LLFile::isfile(target))
		{	
			LLFile::remove(target);
		}
		
		// Other accounts may have new-style cookie files too -- delete them as well
		target = base_dir;
		target += filename;
		target += gDirUtilp->getDirDelimiter();
		target += PLUGIN_COOKIE_FILE_NAME;
		lldebugs << "target = " << target << llendl;
		if(LLFile::isfile(target))
		{	
			LLFile::remove(target);
		}
	}
	
	// If we have an OpenID cookie, re-add it to the cookie store.
	setOpenIDCookie();
}
	
/////////////////////////////////////////////////////////////////////////////////////////
// static 
void LLViewerMedia::clearAllCaches()
{
	// Clear all plugins' caches
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->clearCache();
	}
}
	
/////////////////////////////////////////////////////////////////////////////////////////
// static 
void LLViewerMedia::setCookiesEnabled(bool enabled)
{
	// Set the "cookies enabled" flag for all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->enable_cookies(enabled);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static 
/////////////////////////////////////////////////////////////////////////////////////////
// static
LLPluginCookieStore *LLViewerMedia::getCookieStore()
{
	if(sCookieStore == NULL)
	{
		sCookieStore = new LLPluginCookieStore;
	}
	
	return sCookieStore;
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::loadCookieFile()
{
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PLUGIN_COOKIE_FILE_NAME);

	if (resolved_filename.empty())
	{
		llinfos << "can't get path to plugin cookie file - probably not logged in yet." << llendl;
		return;
	}
	
	// open the file for reading
	llifstream file(resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't load plugin cookies from file \"" << PLUGIN_COOKIE_FILE_NAME << "\"" << llendl;
		return;
	}
	
	getCookieStore()->readAllCookies(file, true);

	file.close();
	
	// send the clear_cookies message to all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		LLPluginClassMedia* plugin = pimpl->getMediaPlugin();
		if(plugin)
		{
			plugin->clear_cookies();
		}
	}
	
	// If we have an OpenID cookie, re-add it to the cookie store.
	setOpenIDCookie();
}


/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::saveCookieFile()
{
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PLUGIN_COOKIE_FILE_NAME);

	if (resolved_filename.empty())
	{
		llinfos << "can't get path to plugin cookie file - probably not logged in yet." << llendl;
		return;
	}

	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't open plugin cookie file \"" << PLUGIN_COOKIE_FILE_NAME << "\" for writing" << llendl;
		return;
	}

	getCookieStore()->writePersistentCookies(file);

	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::addCookie(const std::string &name, const std::string &value, const std::string &domain, const LLDate &expires, const std::string &path, bool secure)
{
	std::stringstream cookie;
	
	cookie << name << "=" << LLPluginCookieStore::quoteString(value);
	
	if(expires.notNull())
	{
		cookie << "; expires=" << expires.asRFC1123();
	}
	
	cookie << "; domain=" << domain;

	cookie << "; path=" << path;
	
	if(secure)
	{
		cookie << "; secure";
	}
	
	getCookieStore()->setCookies(cookie.str());
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::addSessionCookie(const std::string &name, const std::string &value, const std::string &domain, const std::string &path, bool secure)
{
	// A session cookie just has a NULL date.
	addCookie(name, value, domain, LLDate(), path, secure);
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::removeCookie(const std::string &name, const std::string &domain, const std::string &path )
{
	// To remove a cookie, add one with the same name, domain, and path that expires in the past.
	
	addCookie(name, "", domain, LLDate(LLDate::now().secondsSinceEpoch() - 1.0), path);
}


AIHTTPHeaders LLViewerMedia::getHeaders()
{
	AIHTTPHeaders headers;
	headers.addHeader("Accept", "*/*");
	headers.addHeader("Content-Type", "application/xml");
	headers.addHeader("Cookie", sOpenIDCookie);
	headers.addHeader("User-Agent", getCurrentUserAgent());

	return headers;
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setOpenIDCookie()
{
	if(!sOpenIDCookie.empty())
	{
		// The LLURL can give me the 'authority', which is of the form: [username[:password]@]hostname[:port]
		// We want just the hostname for the cookie code, but LLURL doesn't seem to have a way to extract that.
		// We therefore do it here.
		std::string authority = sOpenIDURL.mAuthority;
		std::string::size_type host_start = authority.find('@'); 
		if(host_start == std::string::npos)
		{
			// no username/password
			host_start = 0;
		}
		else
		{
			// Hostname starts after the @. 
			// (If the hostname part is empty, this may put host_start at the end of the string.  In that case, it will end up passing through an empty hostname, which is correct.)
			++host_start;
		}
		std::string::size_type host_end = authority.rfind(':'); 
		if((host_end == std::string::npos) || (host_end < host_start))
		{
			// no port
			host_end = authority.size();
		}
		
		getCookieStore()->setCookiesFromHost(sOpenIDCookie, authority.substr(host_start, host_end - host_start));

		// Do a web profile get so we can store the cookie 
		AIHTTPHeaders headers;
		headers.addHeader("Accept", "*/*");
		headers.addHeader("Cookie", sOpenIDCookie);
		headers.addHeader("User-Agent", getCurrentUserAgent());

		std::string profile_url = getProfileURL("");
		LLURL raw_profile_url( profile_url.c_str() );

		LL_DEBUGS("MediaAuth") << "Requesting " << profile_url << llendl;
		LL_DEBUGS("MediaAuth") << "sOpenIDCookie = [" << sOpenIDCookie << "]" << llendl;
		LLHTTPClient::get(profile_url,  
			new LLViewerMediaWebProfileResponder(raw_profile_url.getAuthority()),
			headers);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::openIDSetup(const std::string &openid_url, const std::string &openid_token)
{
	LL_DEBUGS("MediaAuth") << "url = \"" << openid_url << "\", token = \"" << openid_token << "\"" << LL_ENDL;

	// post the token to the url 
	// the responder will need to extract the cookie(s).

	// Save the OpenID URL for later -- we may need the host when adding the cookie.
	sOpenIDURL.init(openid_url.c_str());
	
	// We shouldn't ever do this twice, but just in case this code gets repurposed later, clear existing cookies.
	sOpenIDCookie.clear();

	AIHTTPHeaders headers;
	// Keep LLHTTPClient from adding an "Accept: application/llsd+xml" header
	headers.addHeader("Accept", "*/*");
	// and use the expected content-type for a post, instead of the LLHTTPClient::postRaw() default of "application/octet-stream"
	headers.addHeader("Content-Type", "application/x-www-form-urlencoded");

	// postRaw() takes ownership of the buffer and releases it later, so we need to allocate a new buffer here.
	size_t size = openid_token.size();
	char* data = new char[size];
	memcpy(data, openid_token.data(), size);

	LLHTTPClient::postRaw(
		openid_url, 
		data, 
		size, 
		new LLViewerMediaOpenIDResponder(),
		headers);
			
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::openIDCookieResponse(const std::string &cookie)
{
	LL_DEBUGS("MediaAuth") << "Cookie received: \"" << cookie << "\"" << LL_ENDL;
	
	sOpenIDCookie += cookie;

	setOpenIDCookie();
}
//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::cleanupClass()
{
	// This is no longer necessary, since the list is no longer smart pointers.
#if 0
	while(!sViewerMediaImplList.empty())
	{
		sViewerMediaImplList.pop_back();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMediaImpl
//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::LLViewerMediaImpl(const std::string& media_url, 
										  const LLUUID& texture_id, 
										  S32 media_width, 
										  S32 media_height, 
										  U8 media_auto_scale, 
										  U8 media_loop,
										  const std::string& mime_type)
:	
	mMovieImageHasMips(false),
	mTextureId(texture_id),
	mMediaWidth(media_width),
	mMediaHeight(media_height),
	mMediaAutoScale(media_auto_scale),
	mMediaLoop(media_loop),
	mMediaURL(media_url),
	mMimeType(mime_type),
	mNeedsNewTexture(true),
	mTextureUsedWidth(0),
	mTextureUsedHeight(0),
	mSuspendUpdates(false),
	mVisible(true),
	mHasFocus(false),
	mClearCache(false),
	mBackgroundColor(LLColor4::white)
{ 
	createMediaSource();
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::~LLViewerMediaImpl()
{
	destroyMediaSource();
	LLViewerMedia::removeMedia(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializeMedia(const std::string& mime_type)
{
	if((mPluginBase == NULL) || (mMimeType != mime_type))
	{
		if(! initializePlugin(mime_type))
		{
			LL_WARNS("Plugin") << "plugin intialization failed for mime type: " << mime_type << LL_ENDL;
			LLSD args;
			args["MIME_TYPE"] = mime_type;
			LLNotificationsUtil::add("NoPlugin", args);

			return false;
		}
	}

	// play();
	return (mPluginBase != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::createMediaSource()
{
	if(! mMediaURL.empty())
	{
		navigateTo(mMediaURL, mMimeType, true);
	}
	else if(! mMimeType.empty())
	{
		initializeMedia(mMimeType);
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::destroyMediaSource()
{
	mNeedsNewTexture = true;
	if (!mPluginBase)
	{
		return;
	}
	// Restore the texture
	updateMovieImage(LLUUID::null, false);
	destroyPlugin();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setMediaType(const std::string& media_type)
{
	mMimeType = media_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*static*/
LLPluginClassMedia* LLViewerMediaImpl::newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner /* may be NULL */, S32 default_width, S32 default_height)
{
	std::string plugin_basename = LLMIMETypes::implType(media_type);
	
	if(plugin_basename.empty())
	{
		LL_WARNS_ONCE("Media") << "Couldn't find plugin for media type " << media_type << LL_ENDL;
	}
	else
	{
		std::string launcher_name = gDirUtilp->getLLPluginLauncher();
		std::string plugin_name = gDirUtilp->getLLPluginFilename(plugin_basename);
		std::string user_data_path = gDirUtilp->getOSUserAppDir();
		user_data_path += gDirUtilp->getDirDelimiter();

		// Fix for EXT-5960 - make browser profile specific to user (cache, cookies etc.)
		// If the linden username returned is blank, that can only mean we are
		// at the login page displaying login Web page or Web browser test via Develop menu.
		// In this case we just use whatever gDirUtilp->getOSUserAppDir() gives us (this
		// is what we always used before this change)
		std::string linden_user_dir = gDirUtilp->getLindenUserDir(true);
		if ( ! linden_user_dir.empty() )
		{
			// gDirUtilp->getLindenUserDir() is whole path, not just Linden name
			user_data_path = linden_user_dir;
			user_data_path += gDirUtilp->getDirDelimiter();
		};

		// See if the plugin executable exists
		llstat s;
		if(LLFile::stat(launcher_name, &s))
		{
			LL_WARNS_ONCE("Media") << "Couldn't find launcher at " << launcher_name << LL_ENDL;
			llassert(false);	// Fail in debugging mode.
		}
		else if(LLFile::stat(plugin_name, &s))
		{
			LL_WARNS_ONCE("Media") << "Couldn't find plugin at " << plugin_name << LL_ENDL;
			llassert(false);	// Fail in debugging mode.
		}
		else
		{
			LLPluginClassMedia* media_source = new LLPluginClassMedia(owner);
			media_source->setSize(default_width, default_height);
			media_source->setUserDataPath(user_data_path);
			media_source->setLanguageCode(LLUI::getLanguage());

			// collect 'cookies enabled' setting from prefs and send to embedded browser
			bool cookies_enabled = gSavedSettings.getBOOL( "CookiesEnabled" );
			media_source->enable_cookies( cookies_enabled );

			// collect 'plugins enabled' setting from prefs and send to embedded browser
			bool plugins_enabled = gSavedSettings.getBOOL( "BrowserPluginsEnabled" );
			media_source->setPluginsEnabled( plugins_enabled );

			// collect 'javascript enabled' setting from prefs and send to embedded browser
			bool javascript_enabled = gSavedSettings.getBOOL( "BrowserJavascriptEnabled" );
			media_source->setJavascriptEnabled( javascript_enabled );

			bool media_plugin_debugging_enabled = gSavedSettings.getBOOL("MediaPluginDebugging");
			media_source->enableMediaPluginDebugging( media_plugin_debugging_enabled );

			const std::string plugin_dir = gDirUtilp->getLLPluginDir();
			if (media_source->init(launcher_name, plugin_dir, plugin_name, gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins")))
			{
				return media_source;
			}
			else
			{
				LL_WARNS("Media") << "Failed to init plugin.  Destroying." << LL_ENDL;
				delete media_source;
			}
		}
	}
	
	LL_WARNS_ONCE("Plugin") << "plugin initialization failed for mime type: " << media_type << LL_ENDL;
	LLSD args;
	args["MIME_TYPE"] = media_type;
	LLNotificationsUtil::add("NoPlugin", args);

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializePlugin(const std::string& media_type)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		// Save the previous media source's last set size before destroying it.
		mMediaWidth = plugin->getSetWidth();
		mMediaHeight = plugin->getSetHeight();
	}
	
	// Always delete the old media impl first.
	destroyMediaSource();
	
	// and unconditionally set the mime type
	mMimeType = media_type;

	LLPluginClassMedia* media_source = newSourceFromMediaType(mMimeType, this, mMediaWidth, mMediaHeight);
	
	if (media_source)
	{
		media_source->setDisableTimeout(gSavedSettings.getBOOL("DebugPluginDisableTimeout"));
		media_source->setLoop(mMediaLoop);
		media_source->setAutoScale(mMediaAutoScale);
		media_source->setBrowserUserAgent(LLViewerMedia::getCurrentUserAgent());
		media_source->focus(mHasFocus);
		media_source->setBackgroundColor(mBackgroundColor);

		if(gSavedSettings.getBOOL("BrowserIgnoreSSLCertErrors"))
		{
			media_source->ignore_ssl_cert_errors(true);
		}

		// the correct way to deal with certs it to load ours from CA.pem and append them to the ones
		// Qt/WebKit loads from your system location.
		// Note: This needs the new CA.pem file with the Equifax Secure Certificate Authority 
		// cert at the bottom: (MIIDIDCCAomgAwIBAgIENd70zzANBg)
		std::string ca_path = gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "CA.pem" );
		media_source->addCertificateFilePath( ca_path );

		if(mClearCache)
		{
			mClearCache = false;
			media_source->clear_cache();
		}
		
		// TODO: Only send cookies to plugins that need them
		//  Ideally, the plugin should tell us whether it handles cookies or not -- either via the init response or through a separate message.
		//  Due to the ordering of messages, it's possible we wouldn't get that information back in time to send cookies before sending a navigate message,
		//  which could cause odd race conditions.
		std::string all_cookies = LLViewerMedia::getCookieStore()->getAllCookies();
		lldebugs << "setting cookies: " << all_cookies << llendl;
		if(!all_cookies.empty())
		{
			media_source->set_cookies(all_cookies);
		}

		mPluginBase = media_source;

		return true;
	}

	return false;
}

void LLViewerMediaImpl::setSize(int width, int height)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	mMediaWidth = width;
	mMediaHeight = height;
	if (plugin)
	{
		plugin->setSize(width, height);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::play()
{
	LLPluginClassMedia* plugin = getMediaPlugin();

	// first stop any previously playing media
	// stop();

	// plugin->addObserver( this );
	if (!plugin)
	{
	 	if(!initializePlugin(mMimeType))
		{
			// Plugin failed initialization... should assert or something
			return;
		}
		plugin = getMediaPlugin();
	}
	
	// updateMovieImage(mTextureId, true);

	plugin->loadURI( mMediaURL );
	if(/*plugin->pluginSupportsMediaTime()*/ true)
	{
		start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::stop()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->stop();
		// destroyMediaSource();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::pause()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->pause();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::start()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::seek(F32 time)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->seek(time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVolume(F32 volume)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->setVolume(volume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::focus(bool focus)
{
	mHasFocus = focus;

	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		// call focus just for the hell of it, even though this apopears to be a nop
		plugin->focus(focus);
		if (focus)
		{
			// spoof a mouse click to *actually* pass focus
			// Don't do this anymore -- it actually clicks through now.
//			plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, 1, 1, 0);
//			plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 1, 1, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::hasFocus() const
{
	// FIXME: This might be able to be a bit smarter by hooking into LLViewerMediaFocus, etc.
	return mHasFocus;
}

void LLViewerMediaImpl::clearCache()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin)
	{
		plugin->clear_cache();
	}
	else
	{
		mClearCache = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (plugin)
	{
		plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseUp(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (plugin)
	{
		plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseMove(S32 x, S32 y, MASK mask)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (plugin)
	{
		plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_MOVE, 0, x, y, mask);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
//static 
void LLViewerMediaImpl::scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	F32 texture_x = texture_coords.mV[VX];
	F32 texture_y = texture_coords.mV[VY];
	
	// Deal with repeating textures by wrapping the coordinates into the range [0, 1.0)
	texture_x = fmodf(texture_x, 1.0f);
	if(texture_x < 0.0f)
		texture_x = 1.0 + texture_x;
		
	texture_y = fmodf(texture_y, 1.0f);
	if(texture_y < 0.0f)
		texture_y = 1.0 + texture_y;

	// scale x and y to texel units.
	*x = llround(texture_x * plugin->getTextureWidth());
	*y = llround((1.0f - texture_y) * plugin->getTextureHeight());

	// Adjust for the difference between the actual texture height and the amount of the texture in use.
	*y -= (plugin->getTextureHeight() - plugin->getHeight());
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(const LLVector2& texture_coords, MASK mask, S32 button)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseDown(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseUp(const LLVector2& texture_coords, MASK mask, S32 button)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin)
	{		
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseUp(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseMove(const LLVector2& texture_coords, MASK mask)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin)
	{		
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseMove(x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (plugin)
	{
		plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOUBLE_CLICK, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scrollWheel(S32 x, S32 y, MASK mask)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (plugin)
	{
		plugin->scrollEvent(x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::onMouseCaptureLost()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 0, mLastMouseX, mLastMouseY, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
BOOL LLViewerMediaImpl::handleMouseUp(S32 x, S32 y, MASK mask) 
{ 
	// NOTE: this is called when the mouse is released when we have capture.
	// Due to the way mouse coordinates are mapped to the object, we can't use the x and y coordinates that come in with the event.
	
	if(hasMouseCapture())
	{
		// Release the mouse -- this will also send a mouseup to the media
		gFocusMgr.setMouseCapture( FALSE );
	}

	return TRUE; 
}

//////////////////////////////////////////////////////////////////////////////////////////
const std::string& LLViewerMediaImpl::getName() const 
{ 
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		return plugin->getMediaName();
	}
	
	return LLStringUtil::null; 
};
//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateHome()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->loadURI( mHomeURL );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateTo(const std::string& url, const std::string& mime_type,  bool rediscover_type)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(rediscover_type)
	{

		LLURI uri(url);
		std::string scheme = uri.scheme();

		if(scheme.empty() || ("http" == scheme || "https" == scheme))
		{
			if(mime_type.empty())
			{
				LLHTTPClient::getHeaderOnly(url, new LLMimeDiscoveryResponder(this, "text/html"));
			}
			else if(initializeMedia(mime_type) && (plugin = getMediaPlugin()))
			{
				plugin->loadURI( url );
			}
		}
		else if("data" == scheme || "file" == scheme || "about" == scheme)
		{
			// FIXME: figure out how to really discover the type for these schemes
			// We use "data" internally for a text/html url for loading the login screen
			if(initializeMedia("text/html") && (plugin = getMediaPlugin()))
			{
				plugin->loadURI( url );
			}
		}
		else
		{
			// This catches 'rtsp://' urls
			if(initializeMedia(scheme) && (plugin = getMediaPlugin()))
			{
				plugin->loadURI( url );
			}
		}
	}
	else if (plugin)
	{
		plugin->loadURI( url );
	}
	else if(initializeMedia(mime_type) && (plugin = getMediaPlugin()))
	{
		plugin->loadURI( url );
	}
	else
	{
		LL_WARNS("Media") << "Couldn't navigate to: " << url << " as there is no media type for: " << mime_type << LL_ENDL;
		return;
	}
	mMediaURL = url;

}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateStop()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		plugin->browse_stop();
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();
	
	if (plugin)
	{
		// FIXME: THIS IS SO WRONG.
		// Menu keys should be handled by the menu system and not passed to UI elements, but this is how LLTextEditor and LLLineEditor do it...
		if( MASK_CONTROL & mask )
		{
			if( 'C' == key )
			{
				plugin->copy();
				result = true;
			}
			else
			if( 'V' == key )
			{
				plugin->paste();
				result = true;
			}
			else
			if( 'X' == key )
			{
				plugin->cut();
				result = true;
			}
		}
		
		if(!result)
		{
			
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			
			result = plugin->keyEvent(LLPluginClassMedia::KEY_EVENT_DOWN ,key, mask, native_key_data);
			// Since the viewer internal event dispatching doesn't give us key-up events, simulate one here.
			(void)plugin->keyEvent(LLPluginClassMedia::KEY_EVENT_UP ,key, mask, native_key_data);
		}
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();
	
	if (plugin)
	{
		// only accept 'printable' characters, sigh...
		if (uni_char >= 32 // discard 'control' characters
			&& uni_char != 127) // SDL thinks this is 'delete' - yuck.
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			
			plugin->textInput(wstring_to_utf8str(LLWString(1, uni_char)), gKeyboard->currentMask(FALSE), native_key_data);
		}
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateForward()
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		result = plugin->getHistoryForwardAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateBack()
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
	{
		result = plugin->getHistoryBackAvailable();
	}
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateMovieImage(const LLUUID& uuid, BOOL active)
{
	// IF the media image hasn't changed, do nothing
	if (mTextureId == uuid)
	{
		return;
	}
	// If we have changed media uuid, restore the old one
	if (!mTextureId.isNull())
	{
		LLViewerTexture* oldImage = LLViewerTextureManager::findTexture( mTextureId );
		if (oldImage) 
		{
			// Casting to LLViewerMediaTexture is a huge hack. Implement LLViewerMediaTexture some time later.
			((LLViewerMediaTexture*)oldImage)->reinit(mMovieImageHasMips);
			oldImage->mIsMediaTexture = FALSE;
		}
	}
	// If the movie is playing, set the new media image
	if (active && !uuid.isNull())
	{
		LLViewerTexture* viewerImage = LLViewerTextureManager::findTexture( uuid );
		if( viewerImage )
		{
			mTextureId = uuid;
			// Can't use mipmaps for movies because they don't update the full image
			// Casting to LLViewerMediaTexture is a huge hack. Implement LLViewerMediaTexture some time later.
			mMovieImageHasMips = ((LLViewerMediaTexture*)viewerImage)->getUseMipMaps();
			((LLViewerMediaTexture*)viewerImage)->reinit(FALSE);
			viewerImage->mIsMediaTexture = TRUE;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::update()
{
	LLPluginClassMedia* plugin = getMediaPlugin();

  	if(plugin)
	{
  		// If we didn't just create the impl, it may need to get cookie updates.
		if(!sUpdatedCookies.empty())
		{
			// TODO: Only send cookies to plugins that need them
			plugin->set_cookies(sUpdatedCookies);
		}
	}

	if (!plugin)
	{
		return;
	}
	
	plugin->idle();
	
	if (plugin->isPluginExited())
	{
		destroyMediaSource();
		return;
	}

	if (!plugin->textureValid())
	{
		return;
	}
	
	if(mSuspendUpdates || !mVisible)
	{
		return;
	}
	
	LLViewerTexture* placeholder_image = updatePlaceholderImage();
		
	if(placeholder_image)
	{
		LLRect dirty_rect;
		if (plugin->getDirty(&dirty_rect))
		{
			// Constrain the dirty rect to be inside the texture
			S32 x_pos = llmax(dirty_rect.mLeft, 0);
			S32 y_pos = llmax(dirty_rect.mBottom, 0);
			S32 width = llmin(dirty_rect.mRight, placeholder_image->getWidth()) - x_pos;
			S32 height = llmin(dirty_rect.mTop, placeholder_image->getHeight()) - y_pos;
			
			if(width > 0 && height > 0)
			{

				U8* data = plugin->getBitsData();

				// Offset the pixels pointer to match x_pos and y_pos
				data += ( x_pos * plugin->getTextureDepth() * plugin->getBitsWidth() );
				data += ( y_pos * plugin->getTextureDepth() );
				
				placeholder_image->setSubImage(
						data, 
						plugin->getBitsWidth(), 
						plugin->getBitsHeight(),
						x_pos, 
						y_pos, 
						width, 
						height,
						TRUE);		// force a fast update (i.e. don't call analyzeAlpha, etc.)

			}
			
			plugin->resetDirty();
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateImagesMediaStreams()
{
}


//////////////////////////////////////////////////////////////////////////////////////////
/*LLViewerMediaTexture*/LLViewerTexture* LLViewerMediaImpl::updatePlaceholderImage()
{
	if(mTextureId.isNull())
	{
		// The code that created this instance will read from the plugin's bits.
		return NULL;
	}
	
	LLViewerMediaTexture* placeholder_image = (LLViewerMediaTexture*)LLViewerTextureManager::getFetchedTexture( mTextureId );
	placeholder_image->getLastReferencedTimer()->reset();

	LLPluginClassMedia* plugin = getMediaPlugin();

	if (mNeedsNewTexture 
		|| placeholder_image->getUseMipMaps()
		|| !placeholder_image->mIsMediaTexture
		|| (placeholder_image->getWidth() != plugin->getTextureWidth())
		|| (placeholder_image->getHeight() != plugin->getTextureHeight())
		|| (mTextureUsedWidth != plugin->getWidth())
		|| (mTextureUsedHeight != plugin->getHeight())
		)
	{
		LL_DEBUGS("Media") << "initializing media placeholder" << LL_ENDL;
		LL_DEBUGS("Media") << "movie image id " << mTextureId << LL_ENDL;

		int texture_width = plugin->getTextureWidth();
		int texture_height = plugin->getTextureHeight();
		int texture_depth = plugin->getTextureDepth();
		
		// MEDIAOPT: check to see if size actually changed before doing work
		placeholder_image->destroyGLTexture();
		// MEDIAOPT: apparently just calling setUseMipMaps(FALSE) doesn't work?
		placeholder_image->reinit(FALSE);	// probably not needed

		// MEDIAOPT: seems insane that we actually have to make an imageraw then
		// immediately discard it
		LLPointer<LLImageRaw> raw = new LLImageRaw(texture_width, texture_height, texture_depth);
		// Clear the texture to the background color, ignoring alpha.
		// convert background color channels from [0.0, 1.0] to [0, 255];
		raw->clear(int(mBackgroundColor.mV[VX] * 255.0f), int(mBackgroundColor.mV[VY] * 255.0f), int(mBackgroundColor.mV[VZ] * 255.0f), 0xff);
		int discard_level = 0;

		// ask media source for correct GL image format constants
		placeholder_image->setExplicitFormat(plugin->getTextureFormatInternal(),
											 plugin->getTextureFormatPrimary(),
											 plugin->getTextureFormatType(),
											 plugin->getTextureFormatSwapBytes());

		placeholder_image->createGLTexture(discard_level, raw);

		// placeholder_image->setExplicitFormat()
		placeholder_image->setUseMipMaps(FALSE);

		// MEDIAOPT: set this dynamically on play/stop
		placeholder_image->mIsMediaTexture = true;
		mNeedsNewTexture = false;
				
		// If the amount of the texture being drawn by the media goes down in either width or height, 
		// recreate the texture to avoid leaving parts of the old image behind.
		mTextureUsedWidth = plugin->getWidth();
		mTextureUsedHeight = plugin->getHeight();
	}
	
	return placeholder_image;
}


//////////////////////////////////////////////////////////////////////////////////////////
LLUUID LLViewerMediaImpl::getMediaTextureID()
{
	return mTextureId;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVisible(bool visible)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	mVisible = visible;
	
	if(mVisible)
	{
		if(plugin && plugin->isPluginExited())
		{
			destroyMediaSource();
		}
		
		if(!plugin)
		{
			createMediaSource();
		}
	}
	
	if(plugin)
	{
		plugin->setPriority(mVisible?LLPluginClassBasic::PRIORITY_NORMAL:LLPluginClassBasic::PRIORITY_SLEEP);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseCapture()
{
	gFocusMgr.setMouseCapture(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::getTextureSize(S32 *texture_width, S32 *texture_height)
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin && plugin->textureValid())
	{
		S32 real_texture_width = plugin->getBitsWidth();
		S32 real_texture_height = plugin->getBitsHeight();

		{
			// The "texture width" coming back from the plugin may not be a power of two (thanks to webkit).
			// It will be the correct "data width" to pass to setSubImage
			int i;
			
			for(i = 1; i < real_texture_width; i <<= 1)
				;
			*texture_width = i;

			for(i = 1; i < real_texture_height; i <<= 1)
				;
			*texture_height = i;
		}
			
	}
	else
	{
		*texture_width = 0;
		*texture_height = 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scaleMouse(S32 *mouse_x, S32 *mouse_y)
{
#if 0
	S32 media_width, media_height;
	S32 texture_width, texture_height;
	getMediaSize( &media_width, &media_height );
	getTextureSize( &texture_width, &texture_height );
	S32 y_delta = texture_height - media_height;

	*mouse_y -= y_delta;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPlaying()
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();
	
	if(plugin)
	{
		EMediaStatus status = plugin->getStatus();
		if(status == MEDIA_PLAYING || status == MEDIA_LOADING)
			result = true;
	}
	
	return result;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPaused()
{
	bool result = false;
	LLPluginClassMedia* plugin = getMediaPlugin();

	if(plugin)
	{
		if(plugin->getStatus() == MEDIA_PAUSED)
			result = true;
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::hasMedia()
{
	return mPluginBase != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::handleMediaEvent(LLPluginClassMedia* self, LLPluginClassMediaOwner::EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mMimeType);
			LLNotificationsUtil::add("MediaPluginFailed", args);
		}
		break;
		default:
		break;
	}
	// Just chain the event to observers.
	emitEvent(self, event);
}
////////////////////////////////////////////////////////////////////////////////
// virtual
void LLViewerMediaImpl::handleCookieSet(LLPluginClassMedia* self, const std::string &cookie)
{
	LLViewerMedia::getCookieStore()->setCookies(cookie);
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::cut()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		plugin->cut();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canCut() const
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		return plugin->canCut();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::copy()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		plugin->copy();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canCopy() const
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		return plugin->canCopy();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::paste()
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		plugin->paste();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL
LLViewerMediaImpl::canPaste() const
{
	LLPluginClassMedia* plugin = getMediaPlugin();
	if (plugin)
		return plugin->canPaste();
	else
		return FALSE;
}

void LLViewerMediaImpl::setBackgroundColor(LLColor4 color)
{
	mBackgroundColor = color; 

	LLPluginClassMedia* plugin = getMediaPlugin();
	if(plugin)
	{
		plugin->setBackgroundColor(mBackgroundColor);
	}
};
