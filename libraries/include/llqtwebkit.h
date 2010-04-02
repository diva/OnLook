/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Linden Lab Inc. (http://lindenlab.com) code.
 *
 * The Initial Developer of the Original Code is:
 *   Callum Prentice (callum@ubrowser.com)
 *
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Callum Prentice (callum@ubrowser.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef LLQTWEBKIT_H
#define LLQTWEBKIT_H

#include <string>
#include <map>

class LLEmbeddedBrowser;
class LLEmbeddedBrowserWindow;

////////////////////////////////////////////////////////////////////////////////
// data class that is passed with an event
class LLEmbeddedBrowserWindowEvent
{
	public:
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri) :
			mEventWindowId(window_id),
			mEventUri(uri)
		{
		};

		// single int passed with the event - e.g. progress
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, int value) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mIntVal(value)
		{
		};

		// string passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mStringVal(value)
		{
		};

		// 2 strings passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value, std::string value2) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mStringVal(value),
			mStringVal2(value2)
		{
		};

		// string and an int passed with the event
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, std::string value, int value2) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mIntVal(value2),
			mStringVal(value)
		{
		}

		// 4 ints passed (semantically as a rectangle but could be anything - didn't want to make a RECT type structure)
		LLEmbeddedBrowserWindowEvent(int window_id, std::string uri, int x, int y, int width, int height) :
			mEventWindowId(window_id),
			mEventUri(uri),
			mXVal(x),
			mYVal(y),
			mWidthVal(width),
			mHeightVal(height)
		{
		};

		virtual ~LLEmbeddedBrowserWindowEvent()
		{
		};

		int getEventWindowId() const
		{
			return mEventWindowId;
		};

		std::string getEventUri() const
		{
			return mEventUri;
		};

		int getIntValue() const
		{
			return mIntVal;
		};

		std::string getStringValue() const
		{
			return mStringVal;
		};

		std::string getStringValue2() const
		{
			return mStringVal2;
		};

		void getRectValue(int& x, int& y, int& width, int& height) const
		{
			x = mXVal;
			y = mYVal;
			width = mWidthVal;
			height = mHeightVal;
		};

	private:
		int mEventWindowId;
		std::string mEventUri;
		int mIntVal;
		std::string mStringVal;
		std::string mStringVal2;
		int mXVal;
		int mYVal;
		int mWidthVal;
		int mHeightVal;
};

////////////////////////////////////////////////////////////////////////////////
// derrive from this class and override these methods to observe these events
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
class LLEmbeddedBrowserWindowObserver
{
	public:
		virtual ~LLEmbeddedBrowserWindowObserver();
		typedef LLEmbeddedBrowserWindowEvent EventType;

		virtual void onCursorChanged(const EventType& event);
		virtual void onPageChanged(const EventType& event);
		virtual void onNavigateBegin(const EventType& event);
		virtual void onNavigateComplete(const EventType& event);
		virtual void onUpdateProgress(const EventType& event);
		virtual void onStatusTextChange(const EventType& event);
		virtual void onTitleChange(const EventType& event);
		virtual void onLocationChange(const EventType& event);
		virtual void onClickLinkHref(const EventType& event);
		virtual void onClickLinkNoFollow(const EventType& event);
};
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

////////////////////////////////////////////////////////////////////////////////
// main library class

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
class LLQtWebKit
{
	public:
		typedef enum e_cursor
                {
                    C_ARROW,
                    C_IBEAM,
                    C_SPLITV,
                    C_SPLITH,
                    C_POINTINGHAND
                } ECursor;

		typedef enum e_user_action
		{
			UA_EDIT_CUT,
			UA_EDIT_COPY,
			UA_EDIT_PASTE,
			UA_NAVIGATE_STOP,
			UA_NAVIGATE_BACK,
			UA_NAVIGATE_FORWARD,
			UA_NAVIGATE_RELOAD
		} EUserAction;

		typedef enum e_key_event
		{
			KE_KEY_DOWN,
			KE_KEY_REPEAT,
			KE_KEY_UP
		}EKeyEvent;

		typedef enum e_mouse_event
		{
			ME_MOUSE_MOVE,
			ME_MOUSE_DOWN,
			ME_MOUSE_UP,
			ME_MOUSE_DOUBLE_CLICK
		}EMouseEvent;

		typedef enum e_mouse_button
		{
			MB_MOUSE_BUTTON_LEFT,
			MB_MOUSE_BUTTON_RIGHT,
			MB_MOUSE_BUTTON_MIDDLE,
			MB_MOUSE_BUTTON_EXTRA_1,
			MB_MOUSE_BUTTON_EXTRA_2,
		}EMouseButton;

		typedef enum e_keyboard_modifier
		{
			KM_MODIFIER_NONE = 0x00,
			KM_MODIFIER_SHIFT = 0x01,
			KM_MODIFIER_CONTROL = 0x02,
			KM_MODIFIER_ALT = 0x04,
			KM_MODIFIER_META = 0x08
		}EKeyboardModifier;

		virtual ~LLQtWebKit();

		// singleton access
		static LLQtWebKit* getInstance();

		// housekeeping
		bool init(std::string application_directory,
                          std::string component_directory,
                          std::string profile_directory,
                          void* native_window_handle);
		bool reset();
		bool clearCache();
		int getLastError();
		std::string getVersion();
		void setBrowserAgentId(std::string id);
		bool enableProxy(bool enabled, std::string host_name, int port);
		bool enableCookies(bool enabled);
		bool clearAllCookies();
		bool enablePlugins(bool enabled);

		// browser window - creation/deletion, mutation etc.
		int createBrowserWindow(int width, int height);
		bool destroyBrowserWindow(int browser_window_id);
		bool setSize(int browser_window_id, int width, int height);
		bool scrollByLines(int browser_window_id, int lines);
		bool setBackgroundColor(int browser_window_id, const int red, const int green, const int blue);
		bool setCaretColor(int browser_window_id, const int red, const int green, const int blue);
		bool setEnabled(int browser_window_id, bool enabled);

		// add/remove yourself as an observer on browser events - see LLEmbeddedBrowserWindowObserver declaration
		bool addObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject);
		bool remObserver(int browser_window_id, LLEmbeddedBrowserWindowObserver* subject);

		// navigation - self explanatory
		bool navigateTo(int browser_window_id, const std::string uri);
		bool userAction(int browser_window_id, EUserAction action);
		bool userActionIsEnabled(int browser_window_id, EUserAction action);

		// javascript access/control
		std::string evaluateJavascript(int browser_window_id, const std::string script);

		// set/clear URL to redirect to when a 404 page is reached
		bool set404RedirectUrl(int browser_window_in, std::string redirect_url);
		bool clr404RedirectUrl(int browser_window_in);

		// access to rendered bitmap data
		const unsigned char* grabBrowserWindow(int browser_window_id);		// renders page to memory and returns pixels
		const unsigned char* getBrowserWindowPixels(int browser_window_id);	// just returns pixels - no render
		bool flipWindow(int browser_window_id, bool flip);			// optionally flip window (pixels) you get back
		int getBrowserWidth(int browser_window_id);						// current browser width (can vary slightly after page is rendered)
		int getBrowserHeight(int browser_window_id);					// current height
		int getBrowserDepth(int browser_window_id);						// depth in bytes
		int getBrowserRowSpan(int browser_window_id);					// width in pixels * depth in bytes

		// mouse/keyboard interaction
		bool mouseEvent(int browser_window_id, EMouseEvent mouse_event, int button, int x, int y, EKeyboardModifier modifiers); // send a mouse event to a browser window at given XY in browser space
		bool scrollWheelEvent(int browser_window_id, int x, int y, int scroll_x, int scroll_y, EKeyboardModifier modifiers);
		bool keyEvent(int browser_window_id, EKeyEvent key_event, int key_code, EKeyboardModifier modifiers);	// send a key press event to a browser window
		bool unicodeInput (int browser_window_id, unsigned long uni_char, EKeyboardModifier modifiers);		// send a unicode keypress event to a browser window
		bool focusBrowser(int browser_window_id, bool focus_browser);			// set/remove focus to given browser window

		// accessor/mutator for scheme that browser doesn't follow - e.g. secondlife.com://
		void setNoFollowScheme(int browser_window_id, std::string scheme);
		std::string getNoFollowScheme(int browser_window_id);

                void pump(int max_milliseconds);

		void prependHistoryUrl(int browser_window_id, std::string url);
		void clearHistory(int browser_window_id);
		std::string dumpHistory(int browser_window_id);

	private:
		LLQtWebKit();
		LLEmbeddedBrowserWindow* getBrowserWindowFromWindowId(int browser_window_id);
		static LLQtWebKit* sInstance;
		const int mMaxBrowserWindows;
		typedef std::map< int, LLEmbeddedBrowserWindow* > BrowserWindowMap;
		typedef std::map< int, LLEmbeddedBrowserWindow* >::iterator BrowserWindowMapIter;
		BrowserWindowMap mBrowserWindowMap;
};

#ifdef __GNUC__
#pragma GCC visibility pop
#endif

// virtual keycodes.
// We don't want to suck in app headers so we copy these consts
// from nsIDOMKeyEvent.idl.

const unsigned long LL_DOM_VK_CANCEL         = 0x03;
const unsigned long LL_DOM_VK_HELP           = 0x06;
const unsigned long LL_DOM_VK_BACK_SPACE     = 0x08;
const unsigned long LL_DOM_VK_TAB            = 0x09;
const unsigned long LL_DOM_VK_CLEAR          = 0x0C;
const unsigned long LL_DOM_VK_RETURN         = 0x0D;
const unsigned long LL_DOM_VK_ENTER          = 0x0E;
const unsigned long LL_DOM_VK_SHIFT          = 0x10;
const unsigned long LL_DOM_VK_CONTROL        = 0x11;
const unsigned long LL_DOM_VK_ALT            = 0x12;
const unsigned long LL_DOM_VK_PAUSE          = 0x13;
const unsigned long LL_DOM_VK_CAPS_LOCK      = 0x14;
const unsigned long LL_DOM_VK_ESCAPE         = 0x1B;
const unsigned long LL_DOM_VK_SPACE          = 0x20;
const unsigned long LL_DOM_VK_PAGE_UP        = 0x21;
const unsigned long LL_DOM_VK_PAGE_DOWN      = 0x22;
const unsigned long LL_DOM_VK_END            = 0x23;
const unsigned long LL_DOM_VK_HOME           = 0x24;
const unsigned long LL_DOM_VK_LEFT           = 0x25;
const unsigned long LL_DOM_VK_UP             = 0x26;
const unsigned long LL_DOM_VK_RIGHT          = 0x27;
const unsigned long LL_DOM_VK_DOWN           = 0x28;
const unsigned long LL_DOM_VK_PRINTSCREEN    = 0x2C;
const unsigned long LL_DOM_VK_INSERT         = 0x2D;
const unsigned long LL_DOM_VK_DELETE         = 0x2E;

// LL_DOM_VK_0 - LL_DOM_VK_9 match their ASCII values
const unsigned long LL_DOM_VK_0              = 0x30;
const unsigned long LL_DOM_VK_1              = 0x31;
const unsigned long LL_DOM_VK_2              = 0x32;
const unsigned long LL_DOM_VK_3              = 0x33;
const unsigned long LL_DOM_VK_4              = 0x34;
const unsigned long LL_DOM_VK_5              = 0x35;
const unsigned long LL_DOM_VK_6              = 0x36;
const unsigned long LL_DOM_VK_7              = 0x37;
const unsigned long LL_DOM_VK_8              = 0x38;
const unsigned long LL_DOM_VK_9              = 0x39;

const unsigned long LL_DOM_VK_SEMICOLON      = 0x3B;
const unsigned long LL_DOM_VK_EQUALS         = 0x3D;

// LL_DOM_VK_A - LL_DOM_VK_Z match their ASCII values
const unsigned long LL_DOM_VK_A              = 0x41;
const unsigned long LL_DOM_VK_B              = 0x42;
const unsigned long LL_DOM_VK_C              = 0x43;
const unsigned long LL_DOM_VK_D              = 0x44;
const unsigned long LL_DOM_VK_E              = 0x45;
const unsigned long LL_DOM_VK_F              = 0x46;
const unsigned long LL_DOM_VK_G              = 0x47;
const unsigned long LL_DOM_VK_H              = 0x48;
const unsigned long LL_DOM_VK_I              = 0x49;
const unsigned long LL_DOM_VK_J              = 0x4A;
const unsigned long LL_DOM_VK_K              = 0x4B;
const unsigned long LL_DOM_VK_L              = 0x4C;
const unsigned long LL_DOM_VK_M              = 0x4D;
const unsigned long LL_DOM_VK_N              = 0x4E;
const unsigned long LL_DOM_VK_O              = 0x4F;
const unsigned long LL_DOM_VK_P              = 0x50;
const unsigned long LL_DOM_VK_Q              = 0x51;
const unsigned long LL_DOM_VK_R              = 0x52;
const unsigned long LL_DOM_VK_S              = 0x53;
const unsigned long LL_DOM_VK_T              = 0x54;
const unsigned long LL_DOM_VK_U              = 0x55;
const unsigned long LL_DOM_VK_V              = 0x56;
const unsigned long LL_DOM_VK_W              = 0x57;
const unsigned long LL_DOM_VK_X              = 0x58;
const unsigned long LL_DOM_VK_Y              = 0x59;
const unsigned long LL_DOM_VK_Z              = 0x5A;

const unsigned long LL_DOM_VK_CONTEXT_MENU   = 0x5D;

const unsigned long LL_DOM_VK_NUMPAD0        = 0x60;
const unsigned long LL_DOM_VK_NUMPAD1        = 0x61;
const unsigned long LL_DOM_VK_NUMPAD2        = 0x62;
const unsigned long LL_DOM_VK_NUMPAD3        = 0x63;
const unsigned long LL_DOM_VK_NUMPAD4        = 0x64;
const unsigned long LL_DOM_VK_NUMPAD5        = 0x65;
const unsigned long LL_DOM_VK_NUMPAD6        = 0x66;
const unsigned long LL_DOM_VK_NUMPAD7        = 0x67;
const unsigned long LL_DOM_VK_NUMPAD8        = 0x68;
const unsigned long LL_DOM_VK_NUMPAD9        = 0x69;
const unsigned long LL_DOM_VK_MULTIPLY       = 0x6A;
const unsigned long LL_DOM_VK_ADD            = 0x6B;
const unsigned long LL_DOM_VK_SEPARATOR      = 0x6C;
const unsigned long LL_DOM_VK_SUBTRACT       = 0x6D;
const unsigned long LL_DOM_VK_DECIMAL        = 0x6E;
const unsigned long LL_DOM_VK_DIVIDE         = 0x6F;
const unsigned long LL_DOM_VK_F1             = 0x70;
const unsigned long LL_DOM_VK_F2             = 0x71;
const unsigned long LL_DOM_VK_F3             = 0x72;
const unsigned long LL_DOM_VK_F4             = 0x73;
const unsigned long LL_DOM_VK_F5             = 0x74;
const unsigned long LL_DOM_VK_F6             = 0x75;
const unsigned long LL_DOM_VK_F7             = 0x76;
const unsigned long LL_DOM_VK_F8             = 0x77;
const unsigned long LL_DOM_VK_F9             = 0x78;
const unsigned long LL_DOM_VK_F10            = 0x79;
const unsigned long LL_DOM_VK_F11            = 0x7A;
const unsigned long LL_DOM_VK_F12            = 0x7B;
const unsigned long LL_DOM_VK_F13            = 0x7C;
const unsigned long LL_DOM_VK_F14            = 0x7D;
const unsigned long LL_DOM_VK_F15            = 0x7E;
const unsigned long LL_DOM_VK_F16            = 0x7F;
const unsigned long LL_DOM_VK_F17            = 0x80;
const unsigned long LL_DOM_VK_F18            = 0x81;
const unsigned long LL_DOM_VK_F19            = 0x82;
const unsigned long LL_DOM_VK_F20            = 0x83;
const unsigned long LL_DOM_VK_F21            = 0x84;
const unsigned long LL_DOM_VK_F22            = 0x85;
const unsigned long LL_DOM_VK_F23            = 0x86;
const unsigned long LL_DOM_VK_F24            = 0x87;

const unsigned long LL_DOM_VK_NUM_LOCK       = 0x90;
const unsigned long LL_DOM_VK_SCROLL_LOCK    = 0x91;

const unsigned long LL_DOM_VK_COMMA          = 0xBC;
const unsigned long LL_DOM_VK_PERIOD         = 0xBE;
const unsigned long LL_DOM_VK_SLASH          = 0xBF;
const unsigned long LL_DOM_VK_BACK_QUOTE     = 0xC0;
const unsigned long LL_DOM_VK_OPEN_BRACKET   = 0xDB;
const unsigned long LL_DOM_VK_BACK_SLASH     = 0xDC;
const unsigned long LL_DOM_VK_CLOSE_BRACKET  = 0xDD;
const unsigned long LL_DOM_VK_QUOTE          = 0xDE;

const unsigned long LL_DOM_VK_META           = 0xE0;

#endif // LLQTWEBKIT_H
