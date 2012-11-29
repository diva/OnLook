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

#include "llwebpageopenshim.h"

#include <qnetworkrequest.h>
#include <qwebframe.h>
#include <qdebug.h>
#include <quuid.h>

#include "llqtwebkit.h"
#include "llembeddedbrowserwindow.h"
#include "llembeddedbrowserwindow_p.h"

LLWebPageOpenShim::LLWebPageOpenShim(LLEmbeddedBrowserWindow *in_window, QObject *parent)
    : QWebPage(parent)
    , window(in_window)
	, mOpeningSelf(false)
	, mGeometryChangeRequested(false)
	, mHasSentUUID(false)
{
//	qDebug() << "LLWebPageOpenShim created";

    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(windowCloseRequested()));
    connect(this, SIGNAL(geometryChangeRequested(const QRect&)),
            this, SLOT(geometryChangeRequested(const QRect&)));

	// Create a unique UUID for this proxy
	mUUID = llToStdString(QUuid::createUuid().toString());

	// mTarget starts out as the empty string, which is what we want.
}

LLWebPageOpenShim::~LLWebPageOpenShim()
{
//	qDebug() << "LLWebPageOpenShim destroyed";
}

void LLWebPageOpenShim::windowCloseRequested()
{
//	qDebug() << "LLWebPageOpenShim::windowCloseRequested";
	if(window)
	{
		LLEmbeddedBrowserWindowEvent event(window->getWindowId());
		event.setStringValue(mUUID);
	    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onWindowCloseRequested, event);
	}
}

void LLWebPageOpenShim::geometryChangeRequested(const QRect& geom)
{
//	qDebug() << "LLWebPageOpenShim::geometryChangeRequested: " << geom ;

	// This seems to happen before acceptNavigationRequest is called.  If this is the case, delay sending the message until afterwards.

	if(window && mHasSentUUID)
	{
		LLEmbeddedBrowserWindowEvent event(window->getWindowId());
		event.setStringValue(mUUID);
		event.setRectValue(geom.x(), geom.y(), geom.width(), geom.height());
	    window->d->mEventEmitter.update(&LLEmbeddedBrowserWindowObserver::onWindowGeometryChangeRequested, event);
	}
	else
	{
		mGeometry = geom;
		mGeometryChangeRequested = true;
	}

}

bool LLWebPageOpenShim::matchesTarget(const std::string target)
{
	return (target == mTarget);
}

bool LLWebPageOpenShim::matchesUUID(const std::string uuid)
{
	return (uuid == mUUID);
}

void LLWebPageOpenShim::setProxy(const std::string &target, const std::string &uuid)
{
	mTarget = target;
	mUUID = uuid;

	mHasSentUUID = false;

	mOpeningSelf = true;

    mainFrame()->evaluateJavaScript(QString("window.open("", \"%1\");").arg( QString::fromStdString(target) ));
}

bool LLWebPageOpenShim::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
	Q_UNUSED(type);
    if (!window)
	{
        return false;
	}

	if(mOpeningSelf)
	{
		qDebug() << "LLWebPageOpenShim::acceptNavigationRequest: reopening self to set target name.";
		return true;
	}

#if 0
	qDebug() << "LLWebPageOpenShim::acceptNavigationRequest called, NavigationType is " << type
		<< ", web frame is " << frame
		<< ", frame->page is " << frame->page()
		<< ", url is " << request.url()
		<< ", frame name is " << frame->frameName()
		;
#endif

    if (request.url().scheme() == QString("file"))
	{
		// For some reason, I'm seeing a spurious call to this function with a file:/// URL that points to the current working directory.
		// Ignoring file:/// URLs here isn't a perfect solution (since it could potentially break content in local HTML files),
		// but it's the best I could come up with for now.

		return false;
	}

	// The name of the incoming frame has been set to the link target that was used when opening this window.
	std::string click_href = llToStdString(request.url());
	mTarget = llToStdString(frame->frameName());

	// build event based on the data we have and emit it
	LLEmbeddedBrowserWindowEvent event( window->getWindowId());
	event.setEventUri(click_href);
	event.setStringValue(mTarget);
	event.setStringValue2(mUUID);

	window->d->mEventEmitter.update( &LLEmbeddedBrowserWindowObserver::onClickLinkHref, event );

	mHasSentUUID = true;

	if(mGeometryChangeRequested)
	{
		geometryChangeRequested(mGeometry);
		mGeometryChangeRequested = false;
	}

    return false;
}

QWebPage *LLWebPageOpenShim::createWindow(WebWindowType type)
{
    Q_UNUSED(type);

	return this;
}
