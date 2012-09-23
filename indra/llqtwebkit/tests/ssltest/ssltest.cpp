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

class sslTest :
	public LLEmbeddedBrowserWindowObserver
{
	public:
		sslTest( std::string url, bool ignore_ca_file, bool ignore_ssl_errors ) :
			mBrowserWindowWidth( 512 ),
			mBrowserWindowHeight( 512 ),
			mBrowserWindowHandle( 0 ),
			mNavigateInProgress( true )
		{
#ifdef _WINDOWS
			std::string cwd = std::string( _getcwd( NULL, 1024) );
			std::string profile_dir = cwd + "\\" + "ssltest_profile";
			void* native_window_handle = (void*)GetDesktopWindow();
			std::string ca_file_loc = cwd + "\\" + "CA.pem";
#else
			std::string cwd = std::string( getcwd( NULL, 1024) );
			std::string profile_dir = cwd + "/" + "ssltest_profile";
			void* native_window_handle = 0;
			std::string ca_file_loc = cwd + "/" + "CA.pem";
#endif
			std::cout << "ssltest> === begin ===" << std::endl;
			std::cout << "ssltest> current working dir is " << cwd << std::endl;
			std::cout << "ssltest> profiles dir location is " << profile_dir << std::endl;

			LLQtWebKit::getInstance()->init( cwd, cwd, profile_dir, native_window_handle );

			LLQtWebKit::getInstance()->enableJavaScript( true );
			LLQtWebKit::getInstance()->enablePlugins( true );

			mBrowserWindowHandle = LLQtWebKit::getInstance()->createBrowserWindow( mBrowserWindowWidth, mBrowserWindowHeight );
			LLQtWebKit::getInstance()->setSize( mBrowserWindowHandle, mBrowserWindowWidth, mBrowserWindowHeight );

			LLQtWebKit::getInstance()->addObserver( mBrowserWindowHandle, this );

			if ( ! ignore_ca_file )
			{
				std::cout << "ssltest> Expected certificate authority file location is " << ca_file_loc << std::endl;
				LLQtWebKit::getInstance()->setCAFile( ca_file_loc.c_str() );
			}
			else
			{
				std::cout << "ssltest> Not loading certificate authority file" << std::endl;
			};

			if ( ignore_ssl_errors )
			{
				LLQtWebKit::getInstance()->setIgnoreSSLCertErrors( true );
				std::cout << "ssltest> Ignoring SSL errors " << std::endl;
			}
			else
			{
				std::cout << "ssltest> Not ignoring SSL errors " << std::endl;
			};

			LLQtWebKit::getInstance()->navigateTo( mBrowserWindowHandle, url );

			std::cout << "ssltest> navigating to " << url << std::endl;
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

		~sslTest()
		{
			LLQtWebKit::getInstance()->remObserver( mBrowserWindowHandle, this );
			LLQtWebKit::getInstance()->reset();
			std::cout << "ssltest> === end ===" << std::endl;
		};

		void onNavigateBegin( const EventType& eventIn )
		{
			mNavigateInProgress = true;
			std::cout << "ssltest> Event: begin navigation to " << eventIn.getEventUri() << std::endl;
		};

		void onNavigateComplete( const EventType& eventIn )
		{
			std::cout << "ssltest> Event: end navigation to " << eventIn.getEventUri() << std::endl;
			mNavigateInProgress = false;
		};

		void onUpdateProgress( const EventType& eventIn )
		{
			std::cout << "ssltest> Event: progress value updated to " << eventIn.getIntValue() << std::endl;
		};

		void onStatusTextChange( const EventType& eventIn )
		{
			std::cout << "ssltest> Event: status updated to " << eventIn.getStringValue() << std::endl;
		};

		void onTitleChange( const EventType& eventIn )
		{
			std::cout << "ssltest> Event: title changed to  " << eventIn.getStringValue() << std::endl;
		};

		void onLocationChange( const EventType& eventIn )
		{
			std::cout << "ssltest> Event: location changed to " << eventIn.getStringValue() << std::endl;
		};

		bool onCertError(const std::string &in_url, const std::string &in_msg)
		{
			std::cout << "ssltest> Cert error triggered\n" << in_url << "\n" << in_msg << std::endl;
			return true;
		}

	private:
		int mBrowserWindowWidth;
		int mBrowserWindowHeight;
		int mBrowserWindowHandle;
		bool mNavigateInProgress;
};

int main( int argc, char* argv[] )
{
	bool ingore_ssl_errors = false;
	bool ignore_ca_file = false;

	for( int i = 1; i < argc; ++i )
	{
		if ( std::string( argv[ i ] ) == "--help" )
		{
			std::cout << std::endl << "ssltest <url> [--ignoresslerrors] [--ignorecafile]" << std::endl;
			std::cout << "Looks for cert file CA.pem in the current working directory";

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

	std::cout << std::endl << " --------- sslTest application starting --------- " << std::endl;
	std::cout << "ssltest> URL specified is " << url << std::endl;

	sslTest* app = new sslTest( url, ignore_ca_file, ingore_ssl_errors );

	bool result = app->idle();
	while( result )
	{
		result = app->idle();
	};

	delete app;

	std::cout << " --------- sslTest application ending  --------- " << std::endl;

	return 0;
}
