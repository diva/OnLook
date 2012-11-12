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

#include "llwebpage.h"

#include <qnetworkrequest.h>
#include <qwebframe.h>
#include <qgraphicswebview.h>
#include <qevent.h>
#include <qdebug.h>
#include <qmessagebox.h>
#include <qwebelement.h>
#include <qgraphicsproxywidget.h>

#include <ctime>

#include "llqtwebkit.h"
#include "llembeddedbrowser.h"
#include "llembeddedbrowserwindow.h"
#include "llembeddedbrowserwindow_p.h"
#include "lljsobject.h"

LLWebPage::LLWebPage(QObject *parent)
    : QWebPage(parent)
    , window(0)
    , mHostLanguage( "en" )
    , mWhiteListRegex( "" )
{
	mJsObject = new LLJsObject( parent );

    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(loadProgressSlot(int)));
    connect(this, SIGNAL(linkHovered(const QString &, const QString &, const QString &)),
            this, SLOT(linkHoveredSlot(const QString &, const QString &, const QString &)));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(statusBarMessageSlot(const QString &)));
    connect(mainFrame(), SIGNAL(urlChanged(const QUrl&)),
            this, SLOT(urlChangedSlot(const QUrl&)));
    connect(this, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));
    connect(this, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished(bool)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(windowCloseRequested()));
    connect(this, SIGNAL(geometryChangeRequested(const QRect&)),
            this, SLOT(geometryChangeRequested(const QRect&)));
    connect(mainFrame(), SIGNAL(titleChanged(const QString&)),
            this, SLOT(titleChangedSlot(const QString&)));
    connect(mainFrame(), SIGNAL(javaScriptWindowObjectCleared()),
            this, SLOT(extendNavigatorObject()));
}

LLWebPage::~LLWebPage()
{
	delete mJsObject;
}

void LLWebPage::loadProgressSlot(int progress)
{
    if (!window)
        return;
    window->d->mPercentComplete = progress;
    LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
	event.setIntValue(progress);
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onUpdateProgress, event);

    if ( progress >= 100 )
		window->d->mShowLoadingOverlay = false;

	window->d->mDirty = true;
	window->grabWindow(0,0,webView->boundingRect().width(),webView->boundingRect().height());

	window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onPageChanged, event);
}

void LLWebPage::linkHoveredSlot(const QString &link, const QString &title, const QString &textContent)
{
    if (!window)
        return;
    LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(llToStdString(link));
	event.setStringValue(llToStdString(title));
	event.setStringValue2(llToStdString(textContent));
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onLinkHovered, event);
}

void LLWebPage::statusBarMessageSlot(const QString& text)
{
    if (!window)
        return;
    window->d->mStatusText = llToStdString(text);
    LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
	event.setStringValue(window->d->mStatusText);
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onStatusTextChange, event);
}

void LLWebPage::titleChangedSlot(const QString& text)
{
    if (!window)
        return;
    window->d->mTitle = llToStdString(text);
	LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
	event.setStringValue(window->d->mTitle);
	window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onTitleChange, event);
}

// set the regex used to determine if a page is trusted or not
void LLWebPage::setWhiteListRegex( const std::string& regex )
{
	mWhiteListRegex = regex;
}

void LLWebPage::configureTrustedPage( bool is_trusted )
{
	// action happens in browser window parent
	LLEmbeddedBrowser* parent_browser = 0;
	if ( window && window->d && window->d->mParent )
	{
		parent_browser = window->d->mParent;
		if ( parent_browser )
		{
			if ( is_trusted )
			{
				//qDebug() << "Whitelist passed - turning on";

				// trusted so turn everything on
				parent_browser->enableJavaScriptTransient( true );
				parent_browser->enableCookiesTransient( true );
				parent_browser->enablePluginsTransient( true );
			}
			else
			{
				//qDebug() << "Whitelist failed - reverting to default state";

				// restore default state set by client
				parent_browser->enableJavaScript( parent_browser->isJavaScriptEnabled() );
				parent_browser->enableCookies( parent_browser->areCookiesEnabled() );
				parent_browser->enablePlugins( parent_browser->arePluginsEnabled() );
			}
		}
	}
}

bool LLWebPage::checkRegex( const QUrl& url )
{
	QRegExp reg_exp( QString::fromStdString( mWhiteListRegex ) );
	reg_exp.setCaseSensitivity( Qt::CaseInsensitive );
	reg_exp.setMinimal( true );

	if ( reg_exp.exactMatch( url.host() ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void LLWebPage::checkWhiteList( const QUrl& url )
{
	if ( mWhiteListRegex.length() )
	{
		if ( checkRegex( url ) )
		{
			configureTrustedPage( true );	// page is "trusted" - go ahead and configure it as such
		}
		else
		{
			configureTrustedPage( false ); // page is "NOT trusted" - go ahead and configure it as such
		}
	}
	else
	// no regex specified, don't do anything (i.e. don't change trust state)
	{
	}
}

void LLWebPage::urlChangedSlot(const QUrl& url)
{
    if (!window)
        return;

	checkWhiteList( url );

	LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onLocationChange, event);
}

bool LLWebPage::event(QEvent *event)
{
    bool result = QWebPage::event(event);

    if (event->type() == QEvent::GraphicsSceneMousePress)
		currentPoint = ((QGraphicsSceneMouseEvent*)event)->pos().toPoint();
    else if(event->type() == QEvent::GraphicsSceneMouseRelease)
        currentPoint = QPoint();

    return result;
}

bool LLWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
	Q_UNUSED( frame );

    if (!window)
        return false;

    if (request.url().scheme() == window->d->mNoFollowScheme)
    {
        QString encodedUrl = request.url().toEncoded();
        // QUrl is turning foo:///home/bar into foo:/home/bar for some reason while Firefox does not
        // http://bugs.webkit.org/show_bug.cgi?id=24695
        if (!encodedUrl.startsWith(window->d->mNoFollowScheme + "://")) {
            encodedUrl = encodedUrl.mid(window->d->mNoFollowScheme.length() + 1);
            encodedUrl = window->d->mNoFollowScheme + "://" + encodedUrl;
        }
        std::string rawUri = llToStdString(encodedUrl);
		LLEmbeddedBrowserWindowEvent event(window->getWindowId());
		event.setEventUri(rawUri);

		// pass over the navigation type as per this page: http://apidocs.meego.com/1.1/core/html/qt4/qwebpage.html#NavigationType-enum
		// pass as strings because telling everyone who needs to know about enums is too invasive.
		std::string nav_type("unknown");
		if  (type == QWebPage::NavigationTypeLinkClicked) nav_type="clicked";
		else
		if  (type == QWebPage::NavigationTypeFormSubmitted) nav_type="form_submited";
		else
		if  (type == QWebPage::NavigationTypeBackOrForward) nav_type="back_forward";
		else
		if  (type == QWebPage::NavigationTypeReload) nav_type="reloaded";
		else
		if  (type == QWebPage::NavigationTypeFormResubmitted) nav_type="form_resubmited";
		event.setNavigationType(nav_type);

		if ( mWhiteListRegex.length() )
		{
			if ( frame )
			{
				if ( checkRegex( frame->url() ) )
				{
					event.setTrustedHost( true );
				}
				else
				{
					event.setTrustedHost( false );
				}
			}
			else
			// no frame - no trust (TODO: when can this happen?)
			{
				event.setTrustedHost( false );
			}
		}
		else
		// no regex is like switching it off and indicating everything is trusted
		{
			event.setTrustedHost( true );
		}

		window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onClickLinkNoFollow, event);

		//qDebug() << "LLWebPage::acceptNavigationRequest: sending onClickLinkNoFollow, NavigationType is " << type << ", url is " << QString::fromStdString(rawUri) ;
		return false;
    }


    return true;
}


void LLWebPage::loadStarted()
{
	if (!window)
		return;

	QUrl url( QString::fromStdString( window->getCurrentUri() ) );
	checkWhiteList( url );

	window->d->mShowLoadingOverlay = true;

	window->d->mTimeLoadStarted=time(NULL);

	window->d->mDirty = true;
	window->grabWindow(0,0,webView->boundingRect().width(),webView->boundingRect().height());

	LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
	window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onNavigateBegin, event);
}

void LLWebPage::loadFinished(bool)
{
	if (!window)
		return;

	window->d->mShowLoadingOverlay = false;

	window->d->mDirty = true;
	window->grabWindow(0,0,webView->boundingRect().width(),webView->boundingRect().height());

	LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	event.setEventUri(window->getCurrentUri());
	window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onPageChanged, event);

	window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onNavigateComplete, event);
}

void LLWebPage::windowCloseRequested()
{
    if (!window)
        return;
    LLEmbeddedBrowserWindowEvent event(window->getWindowId());
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onWindowCloseRequested, event);
}

void LLWebPage::geometryChangeRequested(const QRect& geom)
{
    if (!window)
        return;

	LLEmbeddedBrowserWindowEvent event(window->getWindowId());
	// empty UUID indicates this is targeting the main window
//	event.setStringValue(window->getUUID());
	event.setRectValue(geom.x(), geom.y(), geom.width(), geom.height());
    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onWindowGeometryChangeRequested, event);
}

QString LLWebPage::chooseFile(QWebFrame* parentFrame, const QString& suggestedFile)
{
    Q_UNUSED(parentFrame);
    Q_UNUSED(suggestedFile);

    return QString::fromStdString( window->requestFilePicker() );
}

void LLWebPage::javaScriptAlert(QWebFrame* frame, const QString& msg)
{
    Q_UNUSED(frame);
    QMessageBox *msgBox = new QMessageBox;
    msgBox->setWindowTitle(tr("JavaScript Alert - %1").arg(mainFrame()->url().host()));
    msgBox->setText(msg);
    msgBox->addButton(QMessageBox::Ok);

    QGraphicsProxyWidget *proxy = webView->scene()->addWidget(msgBox);
    proxy->setWindowFlags(Qt::Window); // this makes the item a panel (and will make it get a window 'frame')
    proxy->setPanelModality(QGraphicsItem::SceneModal);
    proxy->setPos((webView->boundingRect().width() - msgBox->sizeHint().width())/2,
                  (webView->boundingRect().height() - msgBox->sizeHint().height())/2);
    proxy->setActive(true); // make it the active item

    connect(msgBox, SIGNAL(finished(int)), proxy, SLOT(deleteLater()));
    msgBox->show();

    webView->scene()->setFocusItem(proxy);
}

bool LLWebPage::javaScriptConfirm(QWebFrame* frame, const QString& msg)
{
    Q_UNUSED(frame);
    Q_UNUSED(msg);
    qWarning() << "LLWebPage::" << __FUNCTION__ << "not implemented" << msg << "returning true";
    return true;
}

bool LLWebPage::javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result)
{
    Q_UNUSED(frame);
    Q_UNUSED(msg);
    Q_UNUSED(defaultValue);
    Q_UNUSED(result);
    qWarning() << "LLWebPage::" << __FUNCTION__ << "not implemented" << msg << defaultValue << "returning false";
    return false;
}

void LLWebPage::extendNavigatorObject()
{
	// legacy - will go away in the future
	QString q_host_language = QString::fromStdString( mHostLanguage );
    mainFrame()->evaluateJavaScript(QString("navigator.hostLanguage=\"%1\"").arg( q_host_language ));

    // the new way
   	if ( mJsObject )
   	{
		bool enabled = mJsObject->getSLObjectEnabled();
		if ( enabled )
		{
			mainFrame()->addToJavaScriptWindowObject("slviewer", mJsObject );
		};
	};
}

QWebPage *LLWebPage::createWindow(WebWindowType type)
{
    Q_UNUSED(type);
	QWebPage *result = NULL;

	if(window)
	{
		result = window->createWindow();
	}

	return result;
}

void LLWebPage::setHostLanguage(const std::string& host_language)
{
	mHostLanguage = host_language;
}

bool LLWebPage::supportsExtension(QWebPage::Extension extension) const
{
    if (extension == QWebPage::ErrorPageExtension)
        return true;
    return false;
}

bool LLWebPage::extension(Extension, const ExtensionOption* option, ExtensionReturn* output)
{
    const QWebPage::ErrorPageExtensionOption* info = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
    QWebPage::ErrorPageExtensionReturn* errorPage = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

    errorPage->content = QString("<html><head><title>Failed loading page</title></head><body bgcolor=\"#ffe0e0\" text=\"#000000\"><h3><tt>%1</tt></h3></body></html>")
        .arg(info->errorString).toUtf8();

    return true;
}

// Second Life viewer specific functions
void LLWebPage::setSLObjectEnabled( bool enabled )
{
	if ( mJsObject )
		mJsObject->setSLObjectEnabled( enabled );
}

void LLWebPage::setAgentLanguage( const std::string& agent_language )
{
	if ( mJsObject )
		mJsObject->setAgentLanguage( QString::fromStdString( agent_language ) );
}

void LLWebPage::setAgentRegion( const std::string& agent_region )
{
	if ( mJsObject )
		mJsObject->setAgentRegion( QString::fromStdString( agent_region ) );
}

void LLWebPage::setAgentLocation( double x, double y, double z )
{
	if ( mJsObject )
	{
		QVariantMap location;
		location["x"] = x;
		location["y"] = y;
		location["z"] = z;
		mJsObject->setAgentLocation( location );
	}
}

void LLWebPage::setAgentGlobalLocation( double x, double y, double z )
{
	if ( mJsObject )
	{
		QVariantMap global_location;
		global_location["x"] = x;
		global_location["y"] = y;
		global_location["z"] = z;
		mJsObject->setAgentGlobalLocation( global_location );
	}
}

void LLWebPage::setAgentOrientation( double angle )
{
	if ( mJsObject )
	{
		mJsObject->setAgentOrientation( angle );
	}
}

void LLWebPage::setAgentMaturity( const std::string& agent_maturity )
{
	if ( mJsObject )
		mJsObject->setAgentMaturity( QString::fromStdString( agent_maturity ) );
}

void LLWebPage::emitLocation()
{
	if ( mJsObject )
		mJsObject->emitLocation();
}

void LLWebPage::emitMaturity()
{
	if ( mJsObject )
		mJsObject->emitMaturity();
}

void LLWebPage::emitLanguage()
{
	if ( mJsObject )
		mJsObject->emitLanguage();
}

void LLWebPage::setPageZoomFactor( double factor )
{
	if ( webView )
	{
		webView->setZoomFactor( factor );
	}
}