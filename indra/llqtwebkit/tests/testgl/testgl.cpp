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
#include <commdlg.h>  // file choser dialog
#include <direct.h>	// for local file access
#endif

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <vector>

#ifdef LL_OSX
// I'm not sure why STATIC_QT is getting defined, but the Q_IMPORT_PLUGIN thing doesn't seem to be necessary on the mac.
#undef STATIC_QT
#endif

#ifdef STATIC_QT
#include <QtPlugin>
Q_IMPORT_PLUGIN(qgif)
#endif

#ifdef LL_OSX
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#define FREEGLUT_STATIC
#include "GL/glut.h"
#endif
#include "llqtwebkit.h"

#ifdef _WINDOWS
	#define PATH_SEPARATOR "\\"
#else
	#define PATH_SEPARATOR "/"
#endif


////////////////////////////////////////////////////////////////////////////////
//
std::string chooseFileName()
{
#ifdef _WINDOWS
	OPENFILENAMEA ofn ;
	static char szFile[_MAX_PATH] ;

    ZeroMemory( &ofn , sizeof( ofn) );
	ofn.lStructSize = sizeof ( ofn );
	ofn.hwndOwner = NULL  ;
	ofn.lpstrFile = szFile ;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof( szFile );
	ofn.lpstrFilter = "All\0*.*\0Images\0*.jpg;*.png\0";
	ofn.nFilterIndex =1;
	ofn.lpstrFileTitle = NULL ;
	ofn.nMaxFileTitle = 0 ;
	ofn.lpstrInitialDir=NULL ;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;

	GetOpenFileNameA( &ofn );

	return ofn.lpstrFile;
#else
	return "";
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of the test app - implemented as a class and derrives from
// the observer so we can catch events emitted by LLQtWebKit
//
class testGL :
	public LLEmbeddedBrowserWindowObserver
{
	public:
		testGL() :
			mAppWindowWidth( 800 ),						// dimensions of the app window - can be anything
			mAppWindowHeight( 900 ),
			mBrowserWindowWidth( mAppWindowWidth ),		// dimensions of the embedded browser - can be anything
			mBrowserWindowHeight( mAppWindowHeight ),	// but looks best when it's the same as the app window
			mAppTextureWidth( -1 ),						// dimensions of the texture that the browser is rendered into
			mAppTextureHeight( -1 ),					// calculated at initialization
			mAppTexture( 0 ),
			mBrowserWindowId( 0 ),
			mAppWindowName( "testGL" ),
			mCwd(),
			mHomeUrl(),
			mNeedsUpdate( true )						// flag to indicate if browser texture needs an update
		{
#ifdef _WINDOWS	// to remove warning on Windows
			mCwd = _getcwd(NULL, 1024);
#else
            mCwd = getcwd(NULL, 1024);
#endif
            mHomeUrl = "http://callum-linden.s3.amazonaws.com/browsertest.html";
            std::cout << "LLQtWebKit version: " << LLQtWebKit::getInstance()->getVersion() << std::endl;

            std::cout << "Current working directory is " << mCwd << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void init( const std::string argv0, const std::string argv1 )
		{
			// OpenGL initialization
			glClearColor( 0.0f, 0.0f, 0.0f, 0.5f);
			glEnable( GL_COLOR_MATERIAL );
			glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
			glEnable( GL_TEXTURE_2D );
			glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
			glEnable( GL_CULL_FACE );

			// calculate texture size required (next power of two above browser window size
			for ( mAppTextureWidth = 1; mAppTextureWidth < mBrowserWindowWidth; mAppTextureWidth <<= 1 )
			{
			};

			for ( mAppTextureHeight = 1; mAppTextureHeight < mBrowserWindowHeight; mAppTextureHeight <<= 1 )
			{
			};

			// create the texture used to display the browser data
			glGenTextures( 1, &mAppTexture );
			glBindTexture( GL_TEXTURE_2D, mAppTexture );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexImage2D( GL_TEXTURE_2D, 0,
				GL_RGB,
					mAppTextureWidth, mAppTextureHeight,
						0, GL_RGB, GL_UNSIGNED_BYTE, 0 );

			// create a single browser window and set things up.
			mProfileDir = mCwd + PATH_SEPARATOR + "testGL_profile";
			std::cout << "Profiles dir location is " << mProfileDir << std::endl;

			mCookiePath = mProfileDir + PATH_SEPARATOR + "cookies.txt";
			std::cout << "Cookies.txt file location is " << mCookiePath << std::endl;

			LLQtWebKit::getInstance()->init( mApplicationDir, mApplicationDir, mProfileDir, getNativeWindowHandle() );

			LLQtWebKit::getInstance()->enableQtMessageHandler( false );

			// set host language test (in reality, string will be language code passed into client)
			// IMPORTANT: must be called before createBrowserWindow(...)
			LLQtWebKit::getInstance()->setHostLanguage( "EN-AB-CD-EF" );

			// set up features
			LLQtWebKit::getInstance()->enableJavaScript( true );
			LLQtWebKit::getInstance()->enableCookies( true );
			LLQtWebKit::getInstance()->enablePlugins( true );

			// make a browser window
			mBrowserWindowId = LLQtWebKit::getInstance()->createBrowserWindow( mBrowserWindowWidth, mBrowserWindowHeight );

			// tell LLQtWebKit about the size of the browser window
			LLQtWebKit::getInstance()->setSize( mBrowserWindowId, mBrowserWindowWidth, mBrowserWindowHeight );

			// observer events that LLQtWebKit emits
			LLQtWebKit::getInstance()->addObserver( mBrowserWindowId, this );

			// append details to agent string
			LLQtWebKit::getInstance()->setBrowserAgentId( mAppWindowName );

			// don't flip bitmap
			LLQtWebKit::getInstance()->flipWindow( mBrowserWindowId, false );

			// only "trust" pages whose host match this regex
			LLQtWebKit::getInstance()->setWhiteListRegex( mBrowserWindowId, "^([^.]+\\.)*amazonaws\\.com$" );

			LLQtWebKit::getInstance()->enableLoadingOverlay( mBrowserWindowId, true );

			// Attempt to read cookies from the cookie file and send them to llqtwebkit.
			{
				std::ifstream cookie_file(mCookiePath.c_str(), std::ios_base::in);
				std::string cookies;

				while(cookie_file.good() && !cookie_file.eof())
				{
					std::string tmp;
					std::getline(cookie_file, tmp);
					cookies += tmp;
					cookies += "\n";
				}

				if(!cookies.empty())
				{
					LLQtWebKit::getInstance()->setCookies(cookies);
				}
			}

			#if 0
			const std::vector<std::string> before=LLQtWebKit::getInstance()->getInstalledCertsList();
			std::cout << "Certs before CA.pem load: " << before.size() << " items" << std::endl;
			for(int i=0;i<before.size();++i)
			{
				std::cout << "    " << before[i] << std::endl;
			}
			std::cout << "---- end of list ----" << std::endl;
			#endif

			// Tell llqtwebkit to look for a CA file in the application directory.
			// If it can't find or parse the file, this should have no effect.
			std::string ca_pem_file_loc = mCwd + PATH_SEPARATOR + "CA.pem";


			LLQtWebKit::getInstance()->setCAFile( ca_pem_file_loc.c_str() );
			std::cout << "Expected CA.pem file location is " << ca_pem_file_loc << std::endl;

			#if 0
			const std::vector<std::string> after=LLQtWebKit::getInstance()->getInstalledCertsList();
			std::cout << "Certs after CA.pem load: " << after.size() << " items" << std::endl;
			for(int i=0;i<after.size();++i)
			{
				std::cout << "    " << after[i] << std::endl;
			}
			std::cout << "---- end of list ----" << std::endl;
			#endif

			// test Second Life viewer specific functions
			LLQtWebKit::getInstance()->setSLObjectEnabled( true );				// true means the feature is turned on
			LLQtWebKit::getInstance()->setAgentLanguage( "tst-en" );			// viewer language selected by agent
			LLQtWebKit::getInstance()->setAgentRegion( "TestGL region" );		// name of region where agent is located
			LLQtWebKit::getInstance()->setAgentLocation( 9.8, 7.6, 5.4 );		// agent's x,y,z location within a region
			LLQtWebKit::getInstance()->setAgentGlobalLocation( 1234.5, 6789.0, 3456.7 );	// agent's x,y,z location within a region
			LLQtWebKit::getInstance()->setAgentOrientation( 175.69 );	// direction (0..359) agent is facing
			LLQtWebKit::getInstance()->setAgentMaturity( "Very immature" );		// selected maturity level of agent

			// go to the "home page" or URL passed in via command line
			if ( ! argv1.empty() )
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, argv1 );
			else
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, mHomeUrl );
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void reset( void )
		{
			// Get cookies from this instance
			std::string cookies = LLQtWebKit::getInstance()->getAllCookies();

			// Dump cookies to stdout
//			std::cout << "Cookies:" << std::endl;
//			std::cout << cookies;

			// and save them to cookies.txt in the profile directory
			{
				std::ofstream cookie_file(mCookiePath.c_str(), std::ios_base::out|std::ios_base::trunc);

				if(cookie_file.good())
				{
					cookie_file << cookies;
				}

				cookie_file.close();
			}

			// unhook observer
			LLQtWebKit::getInstance()->remObserver( mBrowserWindowId, this );

			// clean up
			LLQtWebKit::getInstance()->reset();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void reshape( int widthIn, int heightIn )
		{
			if ( heightIn == 0 )
				heightIn = 1;

			LLQtWebKit::getInstance()->setSize(mBrowserWindowId, widthIn, heightIn );
			mNeedsUpdate = true;

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();

			glViewport( 0, 0, widthIn, heightIn );
			glOrtho( 0.0f, widthIn, heightIn, 0.0f, -1.0f, 1.0f );

			// we use these elsewhere so save
			mAppWindowWidth = widthIn;
			mAppWindowHeight = heightIn;
                        mBrowserWindowWidth = widthIn;
                        mBrowserWindowHeight = heightIn;

			glMatrixMode( GL_MODELVIEW );
			glLoadIdentity();

			mNeedsUpdate = true;
			idle();

			glutPostRedisplay();
		};

		void updateSLvariables()
		{
			if ( rand() % 2 )
				LLQtWebKit::getInstance()->setAgentRegion( "Region Wibble" );
			else
				LLQtWebKit::getInstance()->setAgentRegion( "Region Flasm" );
			LLQtWebKit::getInstance()->setAgentLocation( (rand()%25600)/100.0f, (rand()%25600)/100.0f, (rand()%25600)/100.0f );
			LLQtWebKit::getInstance()->setAgentGlobalLocation( (rand()%25600)/10.0f, (rand()%25600)/10.0f, (rand()%25600)/10.0f );
			LLQtWebKit::getInstance()->setAgentOrientation( (rand()%3600)/10.0f );
			LLQtWebKit::getInstance()->emitLocation();

			if ( rand() % 2 )
				LLQtWebKit::getInstance()->setAgentLanguage( "One language" );
			else
				LLQtWebKit::getInstance()->setAgentLanguage( "Another language" );
			LLQtWebKit::getInstance()->emitLanguage();

			if ( rand() % 2 )
				LLQtWebKit::getInstance()->setAgentMaturity( "Adults only" );
			else
				LLQtWebKit::getInstance()->setAgentMaturity( "Children only" );
			LLQtWebKit::getInstance()->emitMaturity();
		}

		////////////////////////////////////////////////////////////////////////////////
		//
		void idle()
		{
			static time_t starttime = time( NULL );
			if ( time( NULL ) - starttime )
			{
				updateSLvariables();
				time( &starttime );
			};

			LLQtWebKit::getInstance()->pump(100);

			// onPageChanged event sets this
			if ( mNeedsUpdate )
				// grab a page but don't reset 'needs update' flag until we've written it to the texture in display()
				LLQtWebKit::getInstance()->grabBrowserWindow( mBrowserWindowId );

			// lots of updates for smooth motion
			glutPostRedisplay();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void display()
		{
			// clear screen
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			glLoadIdentity();

			// use the browser texture
			glBindTexture( GL_TEXTURE_2D, mAppTexture );

			// valid window ?
			if ( mBrowserWindowId )
			{
				// needs to be updated?
				if ( mNeedsUpdate )
				{
					// grab the page
					const unsigned char* pixels = LLQtWebKit::getInstance()->getBrowserWindowPixels( mBrowserWindowId );
					if ( pixels )
					{
						// write them into the texture
						glTexSubImage2D( GL_TEXTURE_2D, 0,
							0, 0,
								// because sometimes the rowspan != width * bytes per pixel (mBrowserWindowWidth)
								LLQtWebKit::getInstance()->getBrowserRowSpan( mBrowserWindowId ) / LLQtWebKit::getInstance()->getBrowserDepth( mBrowserWindowId ),
									mBrowserWindowHeight,
#ifdef _WINDOWS
                                    LLQtWebKit::getInstance()->getBrowserDepth(mBrowserWindowId ) == 3 ? GL_RGBA : GL_RGBA,
#elif defined(__APPLE__)
                                    GL_RGBA,
#elif defined(LL_LINUX)
                                    GL_RGBA,
#endif
											GL_UNSIGNED_BYTE,
												pixels );
					};

					// flag as already updated
					mNeedsUpdate = false;
				};
			};

			// scale the texture so that it fits the screen
			GLfloat textureScaleX = ( GLfloat )mBrowserWindowWidth / ( GLfloat )mAppTextureWidth;
			GLfloat textureScaleY = ( GLfloat )mBrowserWindowHeight / ( GLfloat )mAppTextureHeight;

			// draw the single quad full screen (orthographic)
			glMatrixMode( GL_TEXTURE );
			glPushMatrix();
			glScalef( textureScaleX, textureScaleY, 1.0f );

			glEnable( GL_TEXTURE_2D );
			glColor3f( 1.0f, 1.0f, 1.0f );
			glBegin( GL_QUADS );
				glTexCoord2f( 1.0f, 0.0f );
				glVertex2d( mAppWindowWidth, 0 );

				glTexCoord2f( 0.0f, 0.0f );
				glVertex2d( 0, 0 );

				glTexCoord2f( 0.0f, 1.0f );
				glVertex2d( 0, mAppWindowHeight );

				glTexCoord2f( 1.0f, 1.0f );
				glVertex2d( mAppWindowWidth, mAppWindowHeight );
			glEnd();

			glMatrixMode( GL_TEXTURE );
			glPopMatrix();

			glutSwapBuffers();
		};

		////////////////////////////////////////////////////////////////////////////////
		// convert a GLUT keyboard modifier to an LLQtWebKit one
		// (only valid in mouse and keyboard callbacks
		LLQtWebKit::EKeyboardModifier getLLQtWebKitKeyboardModifierCode()
		{
			int result = LLQtWebKit::KM_MODIFIER_NONE;

			int modifiers = glutGetModifiers();

			if ( GLUT_ACTIVE_SHIFT & modifiers )
			{
				result |= LLQtWebKit::KM_MODIFIER_SHIFT;
			}

			if ( GLUT_ACTIVE_CTRL & modifiers )
				result |= LLQtWebKit::KM_MODIFIER_CONTROL;

			if ( GLUT_ACTIVE_ALT & modifiers )
				result |= LLQtWebKit::KM_MODIFIER_ALT;

			return (LLQtWebKit::EKeyboardModifier)result;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void mouseButton( int button, int state, int xIn, int yIn )
		{
			// texture is scaled to fit the screen so we scale mouse coords in the same way
			xIn = ( xIn * mBrowserWindowWidth ) / mAppWindowWidth;
			yIn = ( yIn * mBrowserWindowHeight ) / mAppWindowHeight;

			if ( button == GLUT_LEFT_BUTTON )
			{
				if ( state == GLUT_DOWN )
				{
					// send event to LLQtWebKit
					LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
															LLQtWebKit::ME_MOUSE_DOWN,
																LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
																	xIn, yIn,
																		getLLQtWebKitKeyboardModifierCode() );
				}
				else
				if ( state == GLUT_UP )
				{
					// send event to LLQtWebKit
					LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
															LLQtWebKit::ME_MOUSE_UP,
																LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
																	xIn, yIn,
																		getLLQtWebKitKeyboardModifierCode() );


					// this seems better than sending focus on mouse down (still need to improve this)
					LLQtWebKit::getInstance()->focusBrowser( mBrowserWindowId, true );
				};
			};

			// force a GLUT  update
			glutPostRedisplay();
		}

		////////////////////////////////////////////////////////////////////////////////
		//
		void mouseMove( int xIn , int yIn )
		{
			// texture is scaled to fit the screen so we scale mouse coords in the same way
			xIn = ( xIn * mBrowserWindowWidth ) / mAppWindowWidth;
			yIn = ( yIn * mBrowserWindowHeight ) / mAppWindowHeight;

			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
													LLQtWebKit::ME_MOUSE_MOVE,
														LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
															xIn, yIn,
																LLQtWebKit::KM_MODIFIER_NONE );


			// force a GLUT  update
			glutPostRedisplay();
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void keyboard( unsigned char keyIn, bool isDown)
		{
			// ESC key exits
			if ( keyIn == 27 )
			{
				reset();

				exit( 0 );
			};

			// Translate some keys
			switch(keyIn)
			{
				case 127:
					// Turn delete char into backspace
					keyIn = LLQtWebKit::KEY_BACKSPACE;
				break;
				case '\r':
				case '\n':
					// Turn CR and NL into enter key
					keyIn = LLQtWebKit::KEY_RETURN;
				break;

				case '\t':
					keyIn = LLQtWebKit::KEY_TAB;
				break;

				default:
				break;
			}

			// control-H goes home
			if ( keyIn == 8 )
			{
				LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, mHomeUrl );
			}
			// control-B navigates back
			else if ( keyIn == 2 )
			{
				LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_BACK);
			}
			// control-F navigates forward
			else if ( keyIn == 6 )
			{
				LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_FORWARD);
			}
			// control-R reloads
			else if ( keyIn == 18 )
			{
				LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_RELOAD );
			}
			// control-I toggles inspector
			else if ( keyIn == 23 )
			{
				LLQtWebKit::getInstance()->showWebInspector( true );
			}
			else if ( keyIn == '1' )
			{
				if ( getLLQtWebKitKeyboardModifierCode() == LLQtWebKit::KM_MODIFIER_CONTROL )
				{
					LLQtWebKit::getInstance()->setPageZoomFactor( 1.0 );
				}
			}
			else if ( keyIn == '2' )
			{
				if ( getLLQtWebKitKeyboardModifierCode() == LLQtWebKit::KM_MODIFIER_CONTROL )
				{
					LLQtWebKit::getInstance()->setPageZoomFactor( 2.0 );
				}
			}

			char text[2];
			if(keyIn < 0x80)
			{
				text[0] = (char)keyIn;
			}
			else
			{
				text[0] = 0;
			}

			text[1] = 0;

			std::cerr << "key " << (isDown?"down ":"up ") << (int)keyIn << ", modifiers = " << (int)getLLQtWebKitKeyboardModifierCode() << std::endl;

			// send event to LLQtWebKit
			LLQtWebKit::getInstance()->keyboardEvent(mBrowserWindowId, isDown?LLQtWebKit::KE_KEY_DOWN:LLQtWebKit::KE_KEY_UP, keyIn, text, getLLQtWebKitKeyboardModifierCode() );
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void keyboardSpecial( int specialIn, bool isDown)
		{
			uint32_t key = LLQtWebKit::KEY_NONE;

			switch(specialIn)
			{
				case GLUT_KEY_F1:			key = LLQtWebKit::KEY_F1;		break;
				case GLUT_KEY_F2:			key = LLQtWebKit::KEY_F2;		break;
				case GLUT_KEY_F3:			key = LLQtWebKit::KEY_F3;		break;
				case GLUT_KEY_F4:			key = LLQtWebKit::KEY_F4;		break;
				case GLUT_KEY_F5:			key = LLQtWebKit::KEY_F5;		break;
				case GLUT_KEY_F6:			key = LLQtWebKit::KEY_F6;		break;
				case GLUT_KEY_F7:			key = LLQtWebKit::KEY_F7;		break;
				case GLUT_KEY_F8:			key = LLQtWebKit::KEY_F8;		break;
				case GLUT_KEY_F9:			key = LLQtWebKit::KEY_F9;		break;
				case GLUT_KEY_F10:			key = LLQtWebKit::KEY_F10;		break;
				case GLUT_KEY_F11:			key = LLQtWebKit::KEY_F11;		break;
				case GLUT_KEY_F12:			key = LLQtWebKit::KEY_F12;		break;
				case GLUT_KEY_LEFT:			key = LLQtWebKit::KEY_LEFT;		break;
				case GLUT_KEY_UP:			key = LLQtWebKit::KEY_UP;		break;
				case GLUT_KEY_RIGHT:		key = LLQtWebKit::KEY_RIGHT;	break;
				case GLUT_KEY_DOWN:			key = LLQtWebKit::KEY_DOWN;		break;
				case GLUT_KEY_PAGE_UP:		key = LLQtWebKit::KEY_PAGE_UP;	break;
				case GLUT_KEY_PAGE_DOWN:	key = LLQtWebKit::KEY_PAGE_DOWN;break;
				case GLUT_KEY_HOME:			key = LLQtWebKit::KEY_HOME;		break;
				case GLUT_KEY_END:			key = LLQtWebKit::KEY_END;		break;
				case GLUT_KEY_INSERT:		key = LLQtWebKit::KEY_INSERT;	break;

				default:
				break;
			}

			if(key != LLQtWebKit::KEY_NONE)
			{
				keyboard(key, isDown);
			}
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onPageChanged( const EventType& /*eventIn*/ )
		{
			// flag that an update is required - page grab happens in idle() so we don't stall
			mNeedsUpdate = true;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onNavigateBegin( const EventType& eventIn )
		{
			std::cout << "Event: begin navigation to " << eventIn.getEventUri() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onNavigateComplete( const EventType& eventIn )
		{
			std::cout << "Event: end navigation to " << eventIn.getEventUri() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onUpdateProgress( const EventType& eventIn )
		{
			std::cout << "Event: progress value updated to " << eventIn.getIntValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onStatusTextChange( const EventType& eventIn )
		{
			std::cout << "Event: status updated to " << eventIn.getStringValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onTitleChange( const EventType& eventIn )
		{
			std::cout << "Event: title changed to  " << eventIn.getStringValue() << std::endl;
			glutSetWindowTitle( eventIn.getStringValue().c_str() );
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onLocationChange( const EventType& eventIn )
		{
			std::cout << "Event: location changed to " << eventIn.getStringValue() << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onClickLinkHref( const EventType& eventIn )
		{
			std::string uuid = eventIn.getStringValue2();

			std::cout << "Event: clicked on link:" << std::endl;
			std::cout << "  URL:" << eventIn.getEventUri() << std::endl;
			std::cout << "  target:" << eventIn.getStringValue() << std::endl;
			std::cout << "  UUID:" << uuid << std::endl;
			std::cout << std::endl;

			// Since we never actually open the window, send a "proxy window closed" back to webkit to keep it from leaking.
			LLQtWebKit::getInstance()->proxyWindowClosed(mBrowserWindowId, uuid);
		};

		// virtual
		void onClickLinkNoFollow(const EventType& eventIn)
		{
			std::cout << "Clink link no-follow --" << std::endl;
			std::cout << "      URL:" << eventIn.getEventUri() << std::endl;
			std::cout << "     type:" << eventIn.getNavigationType() << std::endl;
			std::cout << "  trusted:" << eventIn.getTrustedHost() << std::endl;
		}


		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onCookieChanged( const EventType& eventIn )
		{
			int dead = eventIn.getIntValue();
			std::cout << (dead?"deleting cookie: ":"setting cookie: ") << eventIn.getStringValue() << std::endl;
		}

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		std::string onRequestFilePicker( const EventType& )
		{
			std::string fn = chooseFileName();
			return fn;
		}

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		bool onAuthRequest(const std::string &in_url, const std::string &in_realm, std::string &out_username, std::string &out_password)
		{
			std::cout << "Auth request, url = " << in_url << ", realm = " << in_realm << std::endl;
			out_username = "";	// replace these temporarily with site username/password as required.
			out_password = "";
			return false;
		}

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		bool onCertError(const std::string &in_url, const std::string &in_msg)
		{
			std::cout << "Cert error, url = " << in_url << ", message = " << in_msg << std::endl;
			return false; // cancel (return true to ignore errors and continue)
		}

		virtual void onQtDebugMessage( const std::string& msg, const std::string& msg_type)
		{
			std::cout << "QtDebugMsg [" << msg_type << "]> " << msg << std::endl;
		}

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onLinkHovered( const EventType& eventIn )
		{
			std::cout
				<< "Link hovered, link = " << eventIn.getEventUri()
				<< ", title = " << eventIn.getStringValue()
				<< ", text = " << eventIn.getStringValue2()
			<< std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onWindowCloseRequested( const EventType& )
		{
			std::cout << "Event: window close requested" << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onNavigateErrorPage( const EventType& event )
		{
			std::cout << "Error page hit with code of " << event.getIntValue() << " - navigating to another URL" << std::endl;
			LLQtWebKit::getInstance()->navigateTo( mBrowserWindowId, "http://bestbuy.com" );
		};

		////////////////////////////////////////////////////////////////////////////////
		// virtual
		void onWindowGeometryChangeRequested( const EventType& eventIn)
		{
			int x, y, width, height;
			eventIn.getRectValue(x, y, width, height);

			std::cout << "Event: window geometry change requested" << std::endl;
			std::cout << "  uuid: " << eventIn.getStringValue() << std::endl;
			std::cout << "  location: (" << x << ", " << y << ")" << std::endl;
			std::cout << "  size: (" << width << ", " << height << ")" << std::endl;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		int getAppWindowWidth()
		{
			return mAppWindowWidth;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		int getAppWindowHeight()
		{
			return mAppWindowHeight;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		std::string getAppWindowName()
		{
			return mAppWindowName;
		};

		////////////////////////////////////////////////////////////////////////////////
		//
		void* getNativeWindowHandle()
		{
			// My implementation of the embedded browser needs a native window handle
			// Can't get this via GLUT so had to use this hack
	    #ifdef _WINDOWS
			return FindWindow( NULL, (LPCWSTR)mAppWindowName.c_str() );
		#else
                #ifdef LL_OSX
                        // not needed on osx
                        return 0;
                #else
                        //#error "You will need an implementation of this method"
                        return 0;
                #endif
            #endif
		};

	private:
		int mAppWindowWidth;
		int mAppWindowHeight;
		int mBrowserWindowWidth;
		int mBrowserWindowHeight;
		int mAppTextureWidth;
		int mAppTextureHeight;
		GLuint mAppTexture;
		int mBrowserWindowId;
		std::string mAppWindowName;
		std::string mHomeUrl;
		std::string mCwd;
		bool mNeedsUpdate;
		std::string mApplicationDir;
		std::string mProfileDir;
		std::string mCookiePath;
};

testGL* theApp;

////////////////////////////////////////////////////////////////////////////////
//
void glutReshape( int widthIn, int heightIn )
{
	if ( theApp )
		theApp->reshape( widthIn, heightIn );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutDisplay()
{
	if ( theApp )
		theApp->display();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutIdle()
{
	if ( theApp )
		theApp->idle();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboard( unsigned char keyIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboard( keyIn, true );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboardUp( unsigned char keyIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboard( keyIn, false );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutSpecial( int specialIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboardSpecial( specialIn, true );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutSpecialUp( int specialIn, int /*xIn*/, int /*yIn*/ )
{
	if ( theApp )
	{
		theApp->keyboardSpecial( specialIn, false );
	}
};

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseMove( int xIn , int yIn )
{
	if ( theApp )
		theApp->mouseMove( xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseButton( int buttonIn, int stateIn, int xIn, int yIn )
{
	if ( theApp )
		theApp->mouseButton( buttonIn, stateIn, xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
int main( int argc, char* argv[] )
{
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );

	// implementation in a class so we can observer events
	// means we need this painful GLUT <--> class shim...
	theApp = new testGL;

	if ( theApp )
	{
		glutInitWindowPosition( 80, 0 );
		glutInitWindowSize( theApp->getAppWindowWidth(), theApp->getAppWindowHeight() );

		glutCreateWindow( theApp->getAppWindowName().c_str() );

		std::string url = "";
		if ( 2 == argc )
			url = std::string( argv[ 1 ] );

		theApp->init( std::string( argv[ 0 ] ), url );

		glutKeyboardFunc( glutKeyboard );
		glutKeyboardUpFunc( glutKeyboardUp );
		glutSpecialFunc( glutSpecial );
		glutSpecialUpFunc( glutSpecialUp );

		glutMouseFunc( glutMouseButton );
		glutPassiveMotionFunc( glutMouseMove );
		glutMotionFunc( glutMouseMove );

		glutDisplayFunc( glutDisplay );
		glutReshapeFunc( glutReshape );

		glutIdleFunc( glutIdle );

		glutMainLoop();

		std::cout << "glutMainLoop returned" << std::endl;

		delete theApp;
	};

	return 0;
}
