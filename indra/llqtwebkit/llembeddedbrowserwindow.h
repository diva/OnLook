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

#ifndef LLEMBEDDEDBROWSERWINDOW_H
#define LLEMBEDDEDBROWSERWINDOW_H

#include <string>
#include <list>
#include <algorithm>
#if defined _MSC_VER && _MSC_VER < 1600
#include "pstdint.h"
#else
#include <stdint.h>        // Use the C99 official header
#endif

#include "llqtwebkit.h"

class LLEmbeddedBrowser;
class LLWebPageOpenShim;
class QWebPage;

////////////////////////////////////////////////////////////////////////////////
// class for a "window" that holds a browser - there can be lots of these
class LLEmbeddedBrowserWindowPrivate;
class LLEmbeddedBrowserWindow
{
public:
    LLEmbeddedBrowserWindow();
    virtual ~LLEmbeddedBrowserWindow();

    // housekeeping
    void setParent(LLEmbeddedBrowser* parent);
    bool setSize(int16_t width, int16_t height);
    void focusBrowser(bool focus_browser);
    void scrollByLines(int16_t lines);
    void setWindowId(int window_id);
    int getWindowId();
	void proxyWindowOpened(const std::string target, const std::string uuid);
	void proxyWindowClosed(const std::string uuid);

    // random accessors
    int16_t getPercentComplete();
    std::string& getStatusMsg();
    std::string& getCurrentUri();

    // memory buffer management
    unsigned char* grabWindow(int x, int y, int width, int height);
    bool flipWindow(bool flip);
    unsigned char* getPageBuffer();
    int16_t getBrowserWidth();
    int16_t getBrowserHeight();
    int16_t getBrowserDepth();
    int32_t getBrowserRowSpan();

    // set background color that you see in between pages - default is white but sometimes useful to change
    void setBackgroundColor(const uint8_t red, const uint8_t green, const uint8_t blue);

    // can turn off updates to a page - e.g. when it's hidden by your windowing system
    void setEnabled(bool enabledIn);

    // navigation
    bool userAction(LLQtWebKit::EUserAction action);
    bool userActionIsEnabled(LLQtWebKit::EUserAction action);
    bool navigateTo(const std::string uri);

    // javascript access/control
    std::string evaluateJavaScript(std::string script);

    // redirection when you hit an error page
	void navigateErrorPage( int http_status_code );

    // host language setting
    void setHostLanguage(const std::string host_language);

    // mouse & keyboard events
    void mouseEvent(LLQtWebKit::EMouseEvent mouse_event, int16_t button, int16_t x, int16_t y, LLQtWebKit::EKeyboardModifier modifiers);
    void scrollWheelEvent(int16_t x, int16_t y, int16_t scroll_x, int16_t scroll_y, LLQtWebKit::EKeyboardModifier modifiers);
    void keyboardEvent(
		LLQtWebKit::EKeyEvent key_event,
		uint32_t key_code,
		const char *utf8_text,
		LLQtWebKit::EKeyboardModifier modifiers,
		uint32_t native_scan_code,
		uint32_t native_virtual_key,
		uint32_t native_modifiers);

    // allow consumers of this class and to observe browser events
    bool addObserver(LLEmbeddedBrowserWindowObserver* observer);
    bool remObserver(LLEmbeddedBrowserWindowObserver* observer);
    int getObserverNumber();

    // accessor/mutator for scheme that browser doesn't follow - e.g. secondlife.com://
    void setNoFollowScheme(std::string scheme);
    std::string getNoFollowScheme();

	// prepend the current history with the given url
	void prependHistoryUrl(std::string url);
	// clear the URL history
	void clearHistory();
	std::string dumpHistory();

	void cookieChanged(const std::string &cookie, const std::string &url, bool dead);

	QWebPage *createWindow();

	LLWebPageOpenShim *findShim(const std::string &uuid);
	void deleteShim(LLWebPageOpenShim *shim);
	void setTarget(const std::string &target);

	std::string requestFilePicker();

	void showWebInspector(bool enabled);

	bool authRequest(const std::string &in_url, const std::string &in_realm, std::string &out_username, std::string &out_password);
	bool certError(const std::string &in_url, const std::string &in_msg);

	void onQtDebugMessage( const std::string& msg, const std::string& msg_type);

	void enableLoadingOverlay(bool enable);

	void setWhiteListRegex( const std::string& regex );

	void setPageZoomFactor( double factor );

	// Second Life specific functions
	void setSLObjectEnabled( bool enabled );
	void setAgentLanguage( const std::string& agent_language );
	void setAgentRegion( const std::string& agent_region );
	void setAgentLocation( double x, double y, double z );
	void setAgentGlobalLocation( double x, double y, double z );
	void setAgentOrientation( double angle );
	void setAgentMaturity( const std::string& agent_maturity );
	void emitLocation();
	void emitMaturity();
	void emitLanguage();

private:
    friend class LLWebPage;
    friend class LLWebPageOpenShim;
    friend class LLGraphicsScene;
    friend class LLWebView;
    friend class LLEmbeddedBrowserPrivate;
    LLEmbeddedBrowserWindowPrivate *d;
    bool mEnableLoadingOverlay;

};


// QString::toStdString converts to ascii, not utf8.  Define our own versions that do utf8.

#ifdef QSTRING_H
std::string llToStdString(const QString &s);
#endif

#ifdef QBYTEARRAY_H
std::string llToStdString(const QByteArray &bytes);
#endif

#ifdef QURL_H
std::string llToStdString(const QUrl &url);
#endif

#endif // LLEMBEDEDDBROWSERWINDOW_H
