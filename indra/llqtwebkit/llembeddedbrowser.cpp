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

#include "llembeddedbrowser.h"

#include "llembeddedbrowser_p.h"
#include "llembeddedbrowserwindow.h"
#include "llnetworkaccessmanager.h"
#include "llstyle.h"

#include <qvariant.h>
#include <qwebsettings.h>
#include <qnetworkproxy.h>
#include <qfile.h>
#include <qsslconfiguration.h>
#include <qsslsocket.h>
#include <qdesktopservices.h>
#include <qdatetime.h>
#include <iostream>

// singleton pattern - initialization
LLEmbeddedBrowser* LLEmbeddedBrowser::sInstance = 0;

LLEmbeddedBrowserPrivate::LLEmbeddedBrowserPrivate()
    : mErrorNum(0)
    , mNativeWindowHandle(0)
    , mNetworkAccessManager(0)
    , mApplication(0)
#if QT_VERSION >= 0x040500
    , mDiskCache(0)
#endif
    , mNetworkCookieJar(0)
    , mHostLanguage( "en" )
	, mIgnoreSSLCertErrors(false)
{
    if (!qApp)
    {
        static int argc = 0;
        static const char* argv[] = {""};
		QApplication::setAttribute(Qt::AA_MacPluginApplication);
		QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

        mApplication = new QApplication(argc, (char **)argv);
        mApplication->addLibraryPath(qApp->applicationDirPath());
    }
    qApp->setStyle(new LLStyle());
    mNetworkAccessManager = new LLNetworkAccessManager(this);
#if LL_DARWIN
	// HACK: Qt installs CarbonEvent handlers that steal events from our main event loop.
	// This uninstalls them.
	// It's not clear whether calling this internal function is really a good idea.  It's probably not.
	// It does, however, seem to fix at least one problem ( https://jira.secondlife.com/browse/MOZ-12 ).
	extern void qt_release_app_proc_handler();
	qt_release_app_proc_handler();

	// This is defined and exported from qwidget_mac.mm.
	// Calling it with false should prevent qwidget from bringing its process to the foreground, such as when bringing up a popup menu.
	extern void qt_mac_set_raise_process(bool b);
	qt_mac_set_raise_process(false);
#endif
}

LLEmbeddedBrowserPrivate::~LLEmbeddedBrowserPrivate()
{
    delete mApplication;
    delete mNetworkAccessManager;
    delete mNetworkCookieJar;
}



LLEmbeddedBrowser::LLEmbeddedBrowser()
    : d(new LLEmbeddedBrowserPrivate)
    , mPluginsEnabled( false )
	, mJavaScriptEnabled( false )
	, mCookiesEnabled( false )
{
}

LLEmbeddedBrowser::~LLEmbeddedBrowser()
{
	if(d->mNetworkCookieJar)
	{
		d->mNetworkCookieJar->mBrowser = NULL;
	}

    delete d;
}

LLEmbeddedBrowser* LLEmbeddedBrowser::getInstance()
{
    if (!sInstance)
        sInstance = new LLEmbeddedBrowser;
    return sInstance;
}

void LLEmbeddedBrowser::setLastError(int error_number)
{
    d->mErrorNum = error_number;
}

void LLEmbeddedBrowser::clearLastError()
{
    d->mErrorNum = 0x0000;
}

int LLEmbeddedBrowser::getLastError()
{
    return d->mErrorNum;
}

std::string LLEmbeddedBrowser::getGREVersion()
{
    // take the string directly from Qt
    return std::string(QT_VERSION_STR);
}

bool LLEmbeddedBrowser::init(std::string application_directory,
                             std::string component_directory,
                             std::string profile_directory,
                             void* native_window_handle)
{
    Q_UNUSED(application_directory);
    Q_UNUSED(component_directory);
    Q_UNUSED(native_window_handle);
    d->mStorageDirectory = QString::fromStdString(profile_directory);
    QWebSettings::setIconDatabasePath(d->mStorageDirectory);
	// The gif and jpeg libraries should be installed in component_directory/imageformats/
	QCoreApplication::addLibraryPath(QString::fromStdString(component_directory));

	// turn on plugins by default
	enablePlugins( true );

    // Until QtWebkit defaults to 16
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFontSize, 16);
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFixedFontSize, 16);


 	QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    QWebSettings::globalSettings()->setOfflineStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

	// use default text encoding - not sure how this helps right now so commenting out until we
	// understand how to use it a little better.
    //QWebSettings::globalSettings()->setDefaultTextEncoding ( "" );

    return reset();
}

bool LLEmbeddedBrowser::reset()
{
    foreach(LLEmbeddedBrowserWindow *window, d->windows)
        delete window;
    d->windows.clear();
    delete d->mNetworkAccessManager;
    d->mNetworkAccessManager = new LLNetworkAccessManager(d);
#if QT_VERSION >= 0x040500
    d->mDiskCache = new QNetworkDiskCache(d->mNetworkAccessManager);
    d->mDiskCache->setCacheDirectory(d->mStorageDirectory + "/cache");
    if (QLatin1String(qVersion()) != QLatin1String("4.5.1"))
        d->mNetworkAccessManager->setCache(d->mDiskCache);
#endif
    d->mNetworkCookieJar = new LLNetworkCookieJar(d->mNetworkAccessManager, this);
    d->mNetworkAccessManager->setCookieJar(d->mNetworkCookieJar);
    clearLastError();
    return true;
}

bool LLEmbeddedBrowser::clearCache()
{
#if QT_VERSION >= 0x040500
    if (d->mDiskCache)
    {
        d->mDiskCache->clear();
        return true;
    }
#endif
    return false;
}

bool LLEmbeddedBrowser::enableProxy(bool enabled, std::string host_name, int port)
{
    QNetworkProxy proxy;
    if (enabled)
    {
        proxy.setType(QNetworkProxy::HttpProxy);
        QString q_host_name = QString::fromStdString(host_name);
        proxy.setHostName(q_host_name);
        proxy.setPort(port);
    }
    d->mNetworkAccessManager->setProxy(proxy);
    return true;
}

bool LLEmbeddedBrowser::clearAllCookies()
{
    if (!d->mNetworkCookieJar)
        return false;
    d->mNetworkCookieJar->clear();
    return true;
}

void LLEmbeddedBrowser::setCookies(const std::string &cookies)
{
    if (d->mNetworkCookieJar)
	{
	    d->mNetworkCookieJar->setCookiesFromRawForm(cookies);
	}
}

std::string LLEmbeddedBrowser::getAllCookies()
{
	std::string result;

    if (d->mNetworkCookieJar)
	{
		result = d->mNetworkCookieJar->getAllCookiesInRawForm();
	}

	return result;
}

void LLEmbeddedBrowser::enableCookies( bool enabled )
{
	mCookiesEnabled = enabled;
	enableCookiesTransient( mCookiesEnabled );
}

void LLEmbeddedBrowser::enableCookiesTransient( bool enabled )
{
    if ( d->mNetworkCookieJar )
    {
	    d->mNetworkCookieJar->mAllowCookies = enabled;
	}
}

bool LLEmbeddedBrowser::areCookiesEnabled()
{
	return mCookiesEnabled;
}

void LLEmbeddedBrowser::enablePlugins( bool enabled )
{
	mPluginsEnabled = enabled;	// record state
	enablePluginsTransient( mPluginsEnabled );
}

void LLEmbeddedBrowser::enablePluginsTransient( bool enabled )
{
    QWebSettings* default_settings = QWebSettings::globalSettings();
    default_settings->setAttribute( QWebSettings::PluginsEnabled, enabled );
}

bool LLEmbeddedBrowser::arePluginsEnabled()
{
	return mPluginsEnabled;
}

void LLEmbeddedBrowser::enableJavaScript( bool enabled )
{
	mJavaScriptEnabled = enabled;	// record state
	enableJavaScriptTransient( mJavaScriptEnabled );
}

void LLEmbeddedBrowser::enableJavaScriptTransient( bool enabled )
{
    QWebSettings* default_settings = QWebSettings::globalSettings();
    default_settings->setAttribute( QWebSettings::JavascriptEnabled, enabled );
    default_settings->setAttribute( QWebSettings::JavascriptCanOpenWindows, enabled );
}

bool LLEmbeddedBrowser::isJavaScriptEnabled()
{
	return mJavaScriptEnabled;
}

bool LLEmbeddedBrowser::showWebInspector(bool show)
{
	QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, show);
	foreach (LLEmbeddedBrowserWindow* window, d->windows)
	{
		window->showWebInspector(show);
	}
	return true;
}

/*
	Sets a string that should be addded to the user agent to identify the application
*/
void LLEmbeddedBrowser::setBrowserAgentId(std::string id)
{
    QCoreApplication::setApplicationName(QString::fromStdString(id));
}

// updates value of 'hostLanguage' in JavaScript 'Navigator' obect that
// embedded pages can query to see what language the host app is set to
// IMPORTANT: call this before any windows are created - only gets passed
//            to LLWebPage when new window is created
void LLEmbeddedBrowser::setHostLanguage( const std::string& host_language )
{
	d->mHostLanguage = host_language;
}

LLEmbeddedBrowserWindow* LLEmbeddedBrowser::createBrowserWindow(int width, int height, const std::string target)
{
    LLEmbeddedBrowserWindow *newWin = new LLEmbeddedBrowserWindow();
    if (newWin)
    {
        newWin->setSize(width, height);
        newWin->setParent(this);
        newWin->setHostLanguage(d->mHostLanguage);
        clearLastError();
        d->windows.append(newWin);

		if(!target.empty() && (target != "_blank"))
		{
			newWin->setTarget(target);
		}

        return newWin;
    }
    return 0;
}

bool LLEmbeddedBrowser::destroyBrowserWindow(LLEmbeddedBrowserWindow* browser_window)
{
    // check if exists in windows list
    if (d->windows.removeOne(browser_window))
    {
        delete browser_window;
        clearLastError();
        return true;
    }
    return false;
}

int LLEmbeddedBrowser::getWindowCount() const
{
    return d->windows.size();
}

void LLEmbeddedBrowser::pump(int max_milliseconds)
{
#if 0
	// This USED to be necessary on the mac, but with Qt 4.6 it seems to cause trouble loading some pages,
	// and using processEvents() seems to work properly now.
	// Leaving this here in case these issues ever come back.

	// On the Mac, calling processEvents hangs the viewer.
	// I'm not entirely sure this does everything we need, but it seems to work better, and allows things like animated gifs to work.
	qApp->sendPostedEvents();
	qApp->sendPostedEvents(0, QEvent::DeferredDelete);
#else
	qApp->processEvents(QEventLoop::AllEvents, max_milliseconds);
#endif
}

void LLEmbeddedBrowser::cookieChanged(const std::string &cookie, const std::string &url, bool dead)
{
	foreach (LLEmbeddedBrowserWindow* window, d->windows)
	{
		window->cookieChanged(cookie, url, dead);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLEmbeddedBrowser::setCAFile(const std::string &ca_file)
{
	bool result = false;
	//qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "attempting to read certs from file: " << QString::fromStdString(ca_file);

	// Extract the list of certificates from the specified file
	QList<QSslCertificate> certs = QSslCertificate::fromPath(QString::fromStdString(ca_file));

	if(!certs.isEmpty())
	{
		//qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "certs read: " << certs;

		// Set the default CA cert for Qt's SSL implementation.
		QSslConfiguration config = QSslConfiguration::defaultConfiguration();
		config.setCaCertificates(certs);
		QSslConfiguration::setDefaultConfiguration(config);
		result = true;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLEmbeddedBrowser::addCAFile(const std::string &ca_file)
{
	// Enabling this can help diagnose certificate verification issues.
	const bool cert_debugging_on = false;

	if ( cert_debugging_on )
	{
		//qDebug() << "\n\nLLEmbeddedBrowser::" << __FUNCTION__ << " ------------------- (Before add)";
		QSslCertificate cert;
		foreach(cert, QSslSocket::defaultCaCertificates())
		{
			//qDebug()  << cert.issuerInfo(QSslCertificate::CommonName) << " --- " << cert.subjectInfo(QSslCertificate::CommonName);
		}
	}

	bool result = false;
	//qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "attempting to read certs from file: " << QString::fromStdString(ca_file);

	if ( cert_debugging_on )
	{
		//qDebug() << "\n\nLLEmbeddedBrowser::" << __FUNCTION__ << " ------------------- (From CA.pem)";
		QList<QSslCertificate> certs = QSslCertificate::fromPath(QString::fromStdString(ca_file));
		QSslCertificate cert;
		foreach(cert, certs)
		{
			//qDebug()  << cert.issuerInfo(QSslCertificate::CommonName) << " --- " << cert.subjectInfo(QSslCertificate::CommonName);
		}
	}

    result = QSslSocket::addDefaultCaCertificates(QString::fromStdString(ca_file));

	if ( cert_debugging_on )
	{
		//qDebug() << "\n\nLLEmbeddedBrowser::" << __FUNCTION__ << " ------------------- (After add)";
		QSslCertificate cert;
		foreach(cert, QSslSocket::defaultCaCertificates())
		{
			//qDebug()  << cert.issuerInfo(QSslCertificate::CommonName) << " --- " << cert.subjectInfo(QSslCertificate::CommonName);
		}
	}

	return result;
}

void LLEmbeddedBrowser::setIgnoreSSLCertErrors(bool ignore)
{
	d->mIgnoreSSLCertErrors = ignore;
}

bool LLEmbeddedBrowser::getIgnoreSSLCertErrors()
{
	return d->mIgnoreSSLCertErrors;
}

const std::vector< std::string > LLEmbeddedBrowser::getInstalledCertsList()
{
	std::vector< std::string > cert_list;

	QSslCertificate cert;
	foreach(cert, QSslSocket::defaultCaCertificates())
	{
		QString cert_info="";

		QString issuer_info="";
		issuer_info+="C=";
		issuer_info+=cert.issuerInfo(QSslCertificate::CountryName);
		issuer_info+=", ST=";
		issuer_info+=cert.issuerInfo(QSslCertificate::StateOrProvinceName);
		issuer_info+=", L=";
		issuer_info+=cert.issuerInfo(QSslCertificate::LocalityName);
		issuer_info+=", O=";
		issuer_info+=cert.issuerInfo(QSslCertificate::Organization);
		issuer_info+=", OU=";
		issuer_info+=cert.issuerInfo(QSslCertificate::OrganizationalUnitName);
		issuer_info+=", CN=";
		issuer_info+=cert.issuerInfo(QSslCertificate::CommonName);
		cert_info+=issuer_info;
		cert_info+="\n";

		QString subject_info="";
		subject_info+="C=";
		subject_info+=cert.subjectInfo(QSslCertificate::CountryName);
		subject_info+=", ST=";
		subject_info+=cert.subjectInfo(QSslCertificate::StateOrProvinceName);
		subject_info+=", L=";
		subject_info+=cert.subjectInfo(QSslCertificate::LocalityName);
		subject_info+=", O=";
		subject_info+=cert.subjectInfo(QSslCertificate::Organization);
		subject_info+=", OU=";
		subject_info+=cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
		subject_info+=", CN=";
		subject_info+=cert.subjectInfo(QSslCertificate::CommonName);
		cert_info+=subject_info;
		cert_info+="\n";

		cert_info+="Not valid before: ";
		cert_info+=cert.effectiveDate().toString();
		cert_info+="\n";
		cert_info+="Not valid after: ";
		cert_info+=cert.expiryDate().toString();
		cert_info+="\n";

		cert_list.push_back( llToStdString(cert_info) );
	}
	return cert_list;
}

// Second Life viewer specific functions
void LLEmbeddedBrowser::setSLObjectEnabled( bool enabled )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setSLObjectEnabled( enabled );
	}
}

void LLEmbeddedBrowser::setAgentLanguage( const std::string& agent_language )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentLanguage( agent_language );
	}
}

void LLEmbeddedBrowser::setAgentRegion( const std::string& agent_region )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentRegion( agent_region );
	}
}

void LLEmbeddedBrowser::setAgentLocation( double x, double y, double z )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentLocation( x, y, z );
	}
}

void LLEmbeddedBrowser::setAgentGlobalLocation( double x, double y, double z )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentGlobalLocation( x, y, z );
	}
}

void LLEmbeddedBrowser::setAgentOrientation( double angle )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentOrientation( angle );
	}
}

void LLEmbeddedBrowser::setAgentMaturity( const std::string& agent_maturity )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setAgentMaturity( agent_maturity );
	}
}

void LLEmbeddedBrowser::emitLocation()
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->emitLocation();
	}
}

void LLEmbeddedBrowser::emitMaturity()
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->emitMaturity();
	}
}

void LLEmbeddedBrowser::emitLanguage()
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->emitLanguage();
	}
}

void LLEmbeddedBrowser::setPageZoomFactor( double factor )
{
	foreach ( LLEmbeddedBrowserWindow* window, d->windows )
	{
		window->setPageZoomFactor( factor );
	}
}

void LLEmbeddedBrowser::qtMessageHandler(QtMsgType type, const char *msg)
{
	std::string msg_type("");
	switch (type)
	{
		case QtDebugMsg:
			msg_type="Debug";
			break;
		case QtWarningMsg:
			msg_type="Warning";
			break;
		case QtCriticalMsg:
			msg_type="Critical";
			break;
		case QtFatalMsg:
			msg_type="Fatal";
			break;
	};

	foreach ( LLEmbeddedBrowserWindow* window, sInstance->d->windows )
	{

		window->onQtDebugMessage( std::string( msg ), msg_type);
	}
}

void LLEmbeddedBrowser::enableQtMessageHandler( bool enable )
{
	if ( enable )
	{
		qInstallMsgHandler( qtMessageHandler );
	}
	else
	{
		// remove handler
		qInstallMsgHandler(0);
	};
}

LLNetworkCookieJar::LLNetworkCookieJar(QObject* parent, LLEmbeddedBrowser *browser)
    : NetworkCookieJar(parent)
    , mAllowCookies(true)
	, mBrowser(browser)
{
}

LLNetworkCookieJar::~LLNetworkCookieJar()
{
}

QList<QNetworkCookie> LLNetworkCookieJar::cookiesForUrl(const QUrl& url) const
{
    if (!mAllowCookies)
        return QList<QNetworkCookie>();
    return NetworkCookieJar::cookiesForUrl(url);
}

bool LLNetworkCookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookie_list, const QUrl& url)
{
    if (!mAllowCookies)
        return false;
    return NetworkCookieJar::setCookiesFromUrl(cookie_list, url);
}

void LLNetworkCookieJar::onCookieSetFromURL(const QNetworkCookie &cookie, const QUrl &url, bool already_dead)
{
//	qDebug() << "LLNetworkCookieJar::" << __FUNCTION__ << (already_dead?"set dead cookie":"set cookie ") << cookie;

	if(mBrowser)
	{
		QByteArray cookie_bytes = cookie.toRawForm(QNetworkCookie::Full);
		std::string cookie_string(cookie_bytes.data(), cookie_bytes.size());
		std::string url_string = llToStdString(url);
		mBrowser->cookieChanged(cookie_string, url_string, already_dead);
	}
}

void LLNetworkCookieJar::clear()
{
    clearCookies();
}

void LLNetworkCookieJar::setCookiesFromRawForm(const std::string &cookie_string)
{
	QByteArray cookie_bytearray(cookie_string.data(), cookie_string.size());
	QList<QNetworkCookie> cookie_list = QNetworkCookie::parseCookies(cookie_bytearray);
	setCookies(cookie_list);
}

std::string LLNetworkCookieJar::getAllCookiesInRawForm()
{
	std::string result;

	QList<QNetworkCookie> cookie_list = allCookies();

	foreach (const QNetworkCookie &cookie, cookie_list)
	{
		QByteArray raw_form = cookie.toRawForm(QNetworkCookie::Full);
		result.append(raw_form.data(), raw_form.size());
		result.append("\n");
	}

	return result;
}

#include "llembeddedbrowserwindow_p.h"
#include <qnetworkreply.h>

QGraphicsWebView *LLEmbeddedBrowserPrivate::findView(QNetworkReply *reply)
{
    for (int i = 0; i < windows.count(); ++i)
        if (windows[i]->d->mView->url() == reply->url())
            return windows[i]->d->mView;
    return windows[0]->d->mView;
}

bool LLEmbeddedBrowserPrivate::authRequest(const std::string &in_url, const std::string &in_realm, std::string &out_username, std::string &out_password)
{
	bool result = false;

//	qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "requesting auth for url " << QString::fromStdString(in_url) << ", realm " << QString::fromStdString(in_realm);
//
//	qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "window count is " << windows.count();

	if(windows.count() > 1)
	{
		qDebug() << "LLEmbeddedBrowser::" << __FUNCTION__ << "WARNING: authRequest called with more than one window, using the first one";
	}

	LLEmbeddedBrowserWindow* window = windows.first();

	if(window)
	{
		result = window->authRequest(in_url, in_realm, out_username, out_password);
	}

	return result;
}

bool LLEmbeddedBrowserPrivate::certError(const std::string &in_url, const std::string &in_msg)
{
	bool result = false;

	LLEmbeddedBrowserWindow* window = windows.first();
	if(window)
	{
		result = window->certError(in_url, in_msg);
	}

	return result;
}
