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

#ifndef _WINDOWS
extern "C" {
#include <unistd.h>
}
#endif

#ifdef _WINDOWS
#include <windows.h>
#include <direct.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <fstream>

#ifdef LL_OSX
// I'm not sure why STATIC_QT is getting defined, but the Q_IMPORT_PLUGIN thing doesn't seem to be necessary on the mac.
#undef STATIC_QT
#endif

#ifdef STATIC_QT
#include <QtPlugin>
Q_IMPORT_PLUGIN(qgif)
#endif

#include "llqtwebkit.h"

class textMode :
	public LLEmbeddedBrowserWindowObserver
{
	public:
		textMode( std::string url, bool ignore_ca_file, bool ignore_ssl_errors ) :
			mBrowserWindowWidth( 512 ),
			mBrowserWindowHeight( 512 ),
			mBrowserWindowHandle( 0 ),
			mNavigateInProgress( true ),
			mLogLine( "" )
		{

#ifdef _WINDOWS
			std::string cwd = std::string( _getcwd( NULL, 1024) );
			std::string profile_dir = cwd + "\\" + "textmode_profile";
			void* native_window_handle = (void*)GetDesktopWindow();
			std::string ca_file_loc = cwd + "\\" + "CA.pem";
#else
			std::string cwd = std::string( getcwd( NULL, 1024) );
			std::string profile_dir = cwd + "/" + "textmode_profile";
			void* native_window_handle = 0;
			std::string ca_file_loc = cwd + "/" + "CA.pem";
#endif
			mLogLine << "Current working dir is " << cwd << std::endl;
			mLogLine << "Profiles dir is " << profile_dir;
			writeLine( mLogLine.str() );

			LLQtWebKit::getInstance()->init( cwd, cwd, profile_dir, native_window_handle );

			LLQtWebKit::getInstance()->enableQtMessageHandler( true );

			LLQtWebKit::getInstance()->enableJavaScript( true );
			LLQtWebKit::getInstance()->enablePlugins( true );

			mBrowserWindowHandle = LLQtWebKit::getInstance()->createBrowserWindow( mBrowserWindowWidth, mBrowserWindowHeight );
			LLQtWebKit::getInstance()->setSize( mBrowserWindowHandle, mBrowserWindowWidth, mBrowserWindowHeight );

			LLQtWebKit::getInstance()->addObserver( mBrowserWindowHandle, this );

			if ( ! ignore_ca_file )
			{
				mLogLine.str("");
				mLogLine << "Expected certificate authority file location is " << ca_file_loc;
				writeLine( mLogLine.str() );
				LLQtWebKit::getInstance()->setCAFile( ca_file_loc.c_str() );
			}
			else
			{
				mLogLine.str("");
				mLogLine << "Not loading certificate authority file";
				writeLine( mLogLine.str() );
			};

			if ( ignore_ssl_errors )
			{
				LLQtWebKit::getInstance()->setIgnoreSSLCertErrors( true );
				mLogLine.str("");
				mLogLine << "Ignoring SSL errors";
				writeLine( mLogLine.str() );
			}
			else
			{
				mLogLine.str("");
				mLogLine << "Not ignoring SSL errors";
				writeLine( mLogLine.str() );
			};

			mLogLine.str("");
			mLogLine << "Navigating to " << url;
			writeLine( mLogLine.str() );

			LLQtWebKit::getInstance()->navigateTo( mBrowserWindowHandle, url );
		};

		bool idle( void )
		{
			LLQtWebKit::getInstance()->pump( 100 );

#if _WINDOWS
			MSG msg;
			while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
			{
				GetMessage( &msg, NULL, 0, 0 );
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			};
#endif
			return mNavigateInProgress;
		};

		~textMode()
		{
			LLQtWebKit::getInstance()->remObserver( mBrowserWindowHandle, this );
			LLQtWebKit::getInstance()->reset();
		};

		void onNavigateBegin( const EventType& eventIn )
		{
			mNavigateInProgress = true;
			mLogLine.str("");
			mLogLine << "Event: Begin navigation to " << eventIn.getEventUri();
			writeLine( mLogLine.str() );
		};

		void onNavigateComplete( const EventType& eventIn )
		{
			mLogLine.str("");
			mLogLine << "Event: End navigation to " << eventIn.getEventUri();
			writeLine( mLogLine.str() );
			mNavigateInProgress = false;
		};

		void onUpdateProgress( const EventType& eventIn )
		{
			mLogLine.str("");
			mLogLine << "Event: progress value updated to " << eventIn.getIntValue();
			writeLine( mLogLine.str() );
		};

		void onStatusTextChange( const EventType& eventIn )
		{
			mLogLine.str("");
			mLogLine << "Event: status updated to " << eventIn.getStringValue();
			writeLine( mLogLine.str() );
		};

		void onTitleChange( const EventType& eventIn )
		{
			mLogLine.str("");
			mLogLine << "Event: title change to " << eventIn.getStringValue();
			writeLine( mLogLine.str() );
		};

		void onLocationChange( const EventType& eventIn )
		{
			mLogLine.str("");
			mLogLine << "Event: location changed to " << eventIn.getStringValue();
			writeLine( mLogLine.str() );
		};

		bool onCertError(const std::string &in_url, const std::string &in_msg)
		{
			mLogLine.str("");
			mLogLine << "Cert error triggered: " << std::endl << in_url << "\n" << in_msg;
			writeLine( mLogLine.str() );
			return true;
		}

		void onCookieChanged(const EventType& event)
		{
			std::string url = event.getEventUri();
			std::string cookie = event.getStringValue();
			int dead = event.getIntValue();
			mLogLine.str("");
			if ( ! dead )
				mLogLine << "Cookie added:" << cookie;
			else
				mLogLine << "Cookie deleted:" << cookie;
			writeLine( mLogLine.str() );
		}

		virtual void onQtDebugMessage( const std::string& msg, const std::string& msg_type)
		{
			mLogLine.str("");
			mLogLine << "QtDebugMsg (" << msg_type << "): " << msg.substr(msg.length() - 1);
			writeLine( mLogLine.str() );
		}

		void writeLine( std::string line )
		{
			double elapsed_seconds = (double)clock() / (double)CLOCKS_PER_SEC;

			std::cout << "[" << std::setprecision(7) << std::setw(3) << std::setfill('0') << elapsed_seconds << "] ";
			const int max_len = 140;
			if ( line.length() > max_len )
			{
				std::cout << line.substr(0, max_len);
				std::cout << "....";
			}
			else
			{
				std::cout << line;
			}
			std::cout << std::endl;
			//std::cout << std::endl;
		}

	private:
		int mBrowserWindowWidth;
		int mBrowserWindowHeight;
		int mBrowserWindowHandle;
		bool mNavigateInProgress;
		std::ostringstream mLogLine;
};

int main( int argc, char* argv[] )
{
	bool ingore_ssl_errors = false;
	bool ignore_ca_file = false;

	for( int i = 1; i < argc; ++i )
	{
		if ( std::string( argv[ i ] ) == "--help" )
		{
			std::cout << std::endl << "textmode <url>" << std::endl;
			exit( 0 );
		};

		if ( std::string( argv[ i ] ) == "--ignoresslerrors" )
			ingore_ssl_errors = true;

		if ( std::string( argv[ i ] ) == "--ignorecafile" )
			ignore_ca_file = true;
	};

	std::string url ( "https://my.secondlife.com/callum.linden" );
	for( int i = 1; i < argc; ++i )
	{
		if ( std::string( argv[ i ] ).substr( 0, 2 ) != "--" )
		{
			url = std::string( argv[ i ] );
			break;
		};
	};

	textMode* app = new textMode( url, ignore_ca_file, ingore_ssl_errors );

	bool result = app->idle();
	while( result )
	{
		result = app->idle();
	};

	delete app;

	return 0;
}
