/* Copyright (c) 2006-2010, Linden Research, Inc.
 *
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include <sstream>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "llqtwebkit.h"

#include "llembeddedbrowser.h"
#include "llembeddedbrowserwindow.h"

LLQtWebKit* LLQtWebKit::sInstance = 0;

////////////////////////////////////////////////////////////////////////////////
//
LLQtWebKit::LLQtWebKit() :
    mMaxBrowserWindows(16)
{
}

////////////////////////////////////////////////////////////////////////////////
//
LLQtWebKit* LLQtWebKit::getInstance()
{
    if (! sInstance)
    {
        sInstance = new LLQtWebKit;
    }

    return sInstance;
}

////////////////////////////////////////////////////////////////////////////////
//
LLQtWebKit::~LLQtWebKit()
{
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::init(std::string application_directory,
                          std::string component_directory,
                          std::string profile_directory,
                          void* native_window_handle)
{
    return LLEmbeddedBrowser::getInstance()->init(application_directory,
                                                  component_directory,
                                                  profile_directory,
                                                  native_window_handle);
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::getLastError()
{
    return LLEmbeddedBrowser::getInstance()->getLastError();
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::reset()
{
    mBrowserWindowMap.clear();
    return LLEmbeddedBrowser::getInstance()->reset();
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::clearCache()
{
    return LLEmbeddedBrowser::getInstance()->clearCache();
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLQtWebKit::getVersion()
{
    const int majorVersion = 2;
    const int minorVersion = 2;

    // number of hours since "time began" for this library - used to identify builds of same version
    const int magicNumber = static_cast< int >((time(NULL) / 3600L) - (321190L));

    // return as a string for now - don't think we need to expose actual version numbers
    std::ostringstream codec;
    codec << std::setw(1) << std::setfill('0');
    codec << majorVersion << ".";
    codec << std::setw(2) << std::setfill('0');
    codec << minorVersion << ".";
    codec << std::setw(5) << std::setfill('0');
    codec << magicNumber;
    codec << " (QtWebKit version ";
    codec << LLEmbeddedBrowser::getInstance()->getGREVersion();
    codec << ")";

    return codec.str();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::setBrowserAgentId(std::string id)
{
    LLEmbeddedBrowser::getInstance()->setBrowserAgentId(id);
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::enableProxy(bool enabled, std::string host_name, int port)
{
    return LLEmbeddedBrowser::getInstance()->enableProxy(enabled, host_name, port);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::setHostLanguage(const std::string& host_language )
{
    LLEmbeddedBrowser::getInstance()->setHostLanguage(host_language);
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::createBrowserWindow(int width, int height, const std::string target)
{
    LLEmbeddedBrowserWindow* browser_window = LLEmbeddedBrowser::getInstance()->createBrowserWindow(width, height, target);

    if (browser_window)
    {
        // arbitrary limit so we don't exhaust system resources
        int id(0);
        while (++id < mMaxBrowserWindows)
        {
            std::pair< BrowserWindowMapIter, bool > result = mBrowserWindowMap.insert(std::make_pair(id, browser_window));

            // find first place the insert succeeds and use that index as the id
            if (result.second)
            {
                browser_window->setWindowId(id);
                return id;
            }
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::proxyWindowOpened(int browser_window_id, const std::string target, const std::string uuid)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->proxyWindowOpened(target, uuid);
    }
}
////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::proxyWindowClosed(int browser_window_id, const std::string uuid)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->proxyWindowClosed(uuid);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::destroyBrowserWindow(int browser_window_id)
{
    // don't use the utility method here since we need the iteratorator to remove the entry from the map
    BrowserWindowMapIter iterator = mBrowserWindowMap.find(browser_window_id);
    LLEmbeddedBrowserWindow* browser_window = (*iterator).second;

    if (browser_window)
    {
        LLEmbeddedBrowser::getInstance()->destroyBrowserWindow(browser_window);
    }

    mBrowserWindowMap.erase(iterator);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::setBackgroundColor(int browser_window_id, const int red, const int green, const int blue)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->setBackgroundColor(red, green, blue);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::setEnabled(int browser_window_id, bool enabled)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->setEnabled(enabled);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::setSize(int browser_window_id, int width, int height)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->setSize(width, height);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::scrollByLines(int browser_window_id, int lines)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->scrollByLines(lines);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::addObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->addObserver(subject);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::remObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->remObserver(subject);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::navigateTo(int browser_window_id, const std::string uri)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->navigateTo(uri) ? true : false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::userAction(int browser_window_id, EUserAction action)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->userAction(action);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::userActionIsEnabled(int browser_window_id, EUserAction action)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->userActionIsEnabled(action);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
const unsigned char* LLQtWebKit::grabBrowserWindow(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->grabWindow(0, 0, browser_window->getBrowserWidth(), browser_window->getBrowserHeight());
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
const unsigned char* LLQtWebKit::getBrowserWindowPixels(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getPageBuffer();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::flipWindow(int browser_window_id, bool flip)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->flipWindow(flip);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::getBrowserWidth(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getBrowserWidth();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::getBrowserHeight(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getBrowserHeight();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::getBrowserDepth(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getBrowserDepth();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
int LLQtWebKit::getBrowserRowSpan(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getBrowserRowSpan();
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::mouseEvent(int browser_window_id, EMouseEvent mouse_event, int button, int x, int y, EKeyboardModifier modifiers)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->mouseEvent(mouse_event, button, x, y, modifiers);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::scrollWheelEvent(int browser_window_id, int x, int y, int scroll_x, int scroll_y, EKeyboardModifier modifiers)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->scrollWheelEvent(x, y, scroll_x, scroll_y, modifiers);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::keyboardEvent(
		int browser_window_id,
		EKeyEvent key_event,
		uint32_t key_code,
		const char *utf8_text,
		EKeyboardModifier modifiers,
		uint32_t native_scan_code,
		uint32_t native_virtual_key,
		uint32_t native_modifiers)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->keyboardEvent(key_event, key_code, utf8_text, modifiers, native_scan_code, native_virtual_key, native_modifiers);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::focusBrowser(int browser_window_id, bool focus_browser)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->focusBrowser(focus_browser);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::setNoFollowScheme(int browser_window_id, std::string scheme)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->setNoFollowScheme(scheme);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLQtWebKit::getNoFollowScheme(int browser_window_id)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->getNoFollowScheme();
    }

    return ("");
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::pump(int max_milliseconds)
{
    LLEmbeddedBrowser::getInstance()->pump(max_milliseconds);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::enableCookies(bool enabled)
{
    LLEmbeddedBrowser::getInstance()->enableCookies( enabled );
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::clearAllCookies()
{
    return LLEmbeddedBrowser::getInstance()->clearAllCookies();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::setCookies(const std::string &cookies)
{
    return LLEmbeddedBrowser::getInstance()->setCookies(cookies);
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLQtWebKit::getAllCookies()
{
    return LLEmbeddedBrowser::getInstance()->getAllCookies();
}


////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::enablePlugins(bool enabled)
{
    LLEmbeddedBrowser::getInstance()->enablePlugins(enabled);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::enableJavaScript(bool enabled)
{
    LLEmbeddedBrowser::getInstance()->enableJavaScript(enabled);
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::showWebInspector(bool show)
{
    return LLEmbeddedBrowser::getInstance()->showWebInspector(show);
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLQtWebKit::evaluateJavaScript(int browser_window_id, const std::string script)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        return browser_window->evaluateJavaScript(script);
    }

    return "";
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::prependHistoryUrl(int browser_window_id, std::string url)
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->prependHistoryUrl(url);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::clearHistory(int browser_window_id)
{
	LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
	if (browser_window)
	{
		browser_window->clearHistory();
	}
}

std::string LLQtWebKit::dumpHistory(int browser_window_id)
{
	LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
	if (browser_window)
	{
		return browser_window->dumpHistory();
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::setCAFile(const std::string &ca_file)
{
	return LLEmbeddedBrowser::getInstance()->setCAFile(ca_file);
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::addCAFile(const std::string &ca_file)
{
	return LLEmbeddedBrowser::getInstance()->addCAFile(ca_file);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLQtWebKit::setIgnoreSSLCertErrors(bool ignore)
{
	LLEmbeddedBrowser::getInstance()->setIgnoreSSLCertErrors(ignore);
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLQtWebKit::getIgnoreSSLCertErrors()
{
	return LLEmbeddedBrowser::getInstance()-> getIgnoreSSLCertErrors();
}

////////////////////////////////////////////////////////////////////////////////
//
const std::vector< std::string > LLQtWebKit::getInstalledCertsList()
{
	return LLEmbeddedBrowser::getInstance()->getInstalledCertsList();
}

////////////////////////////////////////////////////////////////////////////////
// utility method to get an LLEmbeddedBrowserWindow* from a window id (int)
LLEmbeddedBrowserWindow* LLQtWebKit::getBrowserWindowFromWindowId(int browser_window_id)
{
    BrowserWindowMapIter iterator = mBrowserWindowMap.find(browser_window_id);

    if (iterator != mBrowserWindowMap.end())
        return (*iterator).second;
    else
        return 0;
}

LLEmbeddedBrowserWindowObserver::~LLEmbeddedBrowserWindowObserver()
{
}

void LLEmbeddedBrowserWindowObserver::onCursorChanged(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onPageChanged(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onNavigateBegin(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onNavigateComplete(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onUpdateProgress(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onStatusTextChange(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onTitleChange(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onLocationChange(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onNavigateErrorPage(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onClickLinkHref(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onClickLinkNoFollow(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onCookieChanged(const EventType&)
{
}

std::string LLEmbeddedBrowserWindowObserver::onRequestFilePicker(const EventType&)
{
	return std::string();
}

void LLEmbeddedBrowserWindowObserver::onWindowCloseRequested(const EventType&)
{
}

void LLEmbeddedBrowserWindowObserver::onWindowGeometryChangeRequested(const EventType&)
{
}

bool LLEmbeddedBrowserWindowObserver::onAuthRequest(const std::string &, const std::string &, std::string &, std::string &)
{
	return false;
}

bool LLEmbeddedBrowserWindowObserver::onCertError(const std::string &, const std::string &)
{
	return false; // cancel and abort after cert error
}

void LLEmbeddedBrowserWindowObserver::onQtDebugMessage( const std::string &, const std::string &)
{
}

void LLEmbeddedBrowserWindowObserver::onLinkHovered(const EventType&)
{
}

// set the regex used to determine if a page is trusted or not
void LLQtWebKit::setWhiteListRegex( int browser_window_id, const std::string& regex )
{
    LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
    if (browser_window)
    {
        browser_window->setWhiteListRegex(regex);
    }
}

// Second Life viewer specific functions
void LLQtWebKit::setSLObjectEnabled( bool enabled )
{
	LLEmbeddedBrowser::getInstance()->setSLObjectEnabled( enabled );
}

void LLQtWebKit::setAgentLanguage( const std::string& agent_language )
{
	LLEmbeddedBrowser::getInstance()->setAgentLanguage( agent_language );
}

void LLQtWebKit::setAgentRegion( const std::string& agent_region )
{
	LLEmbeddedBrowser::getInstance()->setAgentRegion( agent_region );
}

void LLQtWebKit::setAgentLocation( double x, double y, double z )
{
	LLEmbeddedBrowser::getInstance()->setAgentLocation( x, y, z );
}

void LLQtWebKit::setAgentGlobalLocation( double x, double y, double z )
{
	LLEmbeddedBrowser::getInstance()->setAgentGlobalLocation( x, y, z );
}

void LLQtWebKit::setAgentOrientation( double angle )
{
	LLEmbeddedBrowser::getInstance()->setAgentOrientation( angle );
}


void LLQtWebKit::setAgentMaturity( const std::string& agent_maturity )
{
	LLEmbeddedBrowser::getInstance()->setAgentMaturity( agent_maturity );
}

void LLQtWebKit::emitLocation()
{
	LLEmbeddedBrowser::getInstance()->emitLocation();
}

void LLQtWebKit::emitMaturity()
{
	LLEmbeddedBrowser::getInstance()->emitMaturity();
}

void LLQtWebKit::emitLanguage()
{
	LLEmbeddedBrowser::getInstance()->emitLanguage();
}

void LLQtWebKit::enableQtMessageHandler( bool enable )
{
	LLEmbeddedBrowser::getInstance()->enableQtMessageHandler( enable );
}

void LLQtWebKit::enableLoadingOverlay( int browser_window_id, bool enable)
{
	LLEmbeddedBrowserWindow* browser_window = getBrowserWindowFromWindowId(browser_window_id);
	if (browser_window)
	{
		browser_window->enableLoadingOverlay( enable );
	}
}

void LLQtWebKit::setPageZoomFactor( double factor )
{
	LLEmbeddedBrowser::getInstance()->setPageZoomFactor( factor );
}
