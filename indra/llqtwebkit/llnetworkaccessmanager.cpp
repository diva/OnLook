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
#include "llnetworkaccessmanager.h"

#include <qauthenticator.h>
#include <qnetworkreply.h>
#include <qtextdocument.h>
#include <qgraphicsview.h>
#include <qgraphicsscene.h>
#include <qdatetime.h>
#include <qgraphicsproxywidget.h>
#include <qdebug.h>
#include <qsslconfiguration.h>

#include "llembeddedbrowserwindow.h"
#include "llembeddedbrowser_p.h"

#include "ui_passworddialog.h"


LLNetworkAccessManager::LLNetworkAccessManager(LLEmbeddedBrowserPrivate* browser,QObject* parent)
    : QNetworkAccessManager(parent)
    , mBrowser(browser)
{
    connect(this, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(finishLoading(QNetworkReply*)));
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            this, SLOT(authenticationRequiredSlot(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(sslErrors( QNetworkReply *, const QList<QSslError> &)),
            this, SLOT(sslErrorsSlot( QNetworkReply *, const QList<QSslError> &  )));
}

QNetworkReply *LLNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request,
                                         QIODevice *outgoingData)
{

	// Create a local copy of the request we can modify.
	QNetworkRequest mutable_request(request);

	// Set an Accept-Language header in the request, based on what the host has set through setHostLanguage.
	mutable_request.setRawHeader(QByteArray("Accept-Language"), QByteArray(mBrowser->mHostLanguage.c_str()));

	// this is undefine'd in 4.7.1 and leads to caching issues - setting it here explicitly
	mutable_request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork);

	if(op == GetOperation)
	{
		// GET requests should not have a Content-Type header, but it seems somebody somewhere is adding one.
		// This removes it.
		mutable_request.setRawHeader("Content-Type", QByteArray());
	}

//	qDebug() << "headers for request:" << mutable_request.rawHeaderList();

	// and pass this through to the parent implementation
	return QNetworkAccessManager::createRequest(op, mutable_request, outgoingData);
}

void LLNetworkAccessManager::sslErrorsSlot(QNetworkReply* reply, const QList<QSslError>& errors)
{
	// Enabling this can help diagnose certificate verification issues.
	const bool ssl_debugging_on = false;

	// flag that indicates if the error that brought us here is one we care about or not
	bool valid_ssl_error = false;

	foreach( const QSslError &error, errors )
	{
		if ( ssl_debugging_on )
		{
			qDebug() << "SSL error details are (" << (int)(error.error()) << ") - " << error.error();
		}

		// SSL "error" codes we don't care about - if we get one of these, we want to continue
		if ( error.error() != QSslError::NoError
			 // many more in src/network/ssl/qsslerror.h
			)
		{
			if ( ssl_debugging_on )
			{
				qDebug() << "Found valid SSL error - will not ignore";
			}

			valid_ssl_error = true;
		}
		else
		{
			if ( ssl_debugging_on )
			{
				qDebug() << "Found invalid SSL error - will ignore and continue";
			}
		}
	}

	if ( ssl_debugging_on )
	{
		qDebug() << "LLNetworkAccessManager" << __FUNCTION__ << "errors: " << errors
			<< ", peer certificate chain: ";

		QSslCertificate cert;
		foreach(cert, reply->sslConfiguration().peerCertificateChain())
		{
			qDebug() << "    cert: " << cert
				<< ", issuer = " << cert.issuerInfo(QSslCertificate::CommonName)
				<< ", subject = " << cert.subjectInfo(QSslCertificate::CommonName);
		}
	}

	if ( valid_ssl_error )
	{
		std::string url = llToStdString(reply->url());
		QString err_msg="";
		foreach( const QSslError &error, errors )
		{
			err_msg+=error.errorString();
			err_msg+="\n";

			QSslCertificate cert = error.certificate();

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
			err_msg+=issuer_info;
			err_msg+="\n";

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
			err_msg+=subject_info;
			err_msg+="\n";

			err_msg+="Not valid before: ";
			err_msg+=cert.effectiveDate().toString();
			err_msg+="\n";
			err_msg+="Not valid after: ";
			err_msg+=cert.expiryDate().toString();
			err_msg+="\n";
			err_msg+="----------\n";
		}

		if(mBrowser->certError(url, llToStdString(err_msg)))
		{
			// signal we should ignore and continue processing
			reply->ignoreSslErrors();
		}
		else
		{
			// The user canceled, don't return yet so we can test ignore variable
		}
	}

	// we the SSL error is invalid (in our opinion) or we explicitly ignore all SSL errors
	if ( valid_ssl_error == false || ( mBrowser && mBrowser->mIgnoreSSLCertErrors ) )
	{
		// signal we should ignore and continue processing
		reply->ignoreSslErrors();
	};
}

void LLNetworkAccessManager::finishLoading(QNetworkReply* reply)
{
	QVariant val = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
	int http_status_code = val.toInt();
	if ( http_status_code >=400 && http_status_code <=499 )
	{
        if (mBrowser)
        {
            std::string current_url = llToStdString(reply->url());
            foreach (LLEmbeddedBrowserWindow *window, mBrowser->windows)
            {
                if (window->getCurrentUri() == current_url)
                {
                    window->navigateErrorPage( http_status_code );
				}
            }
        }
    }

	// tests if navigation request resulted in a cache hit - useful for testing so leaving here for the moment.
	//QVariant from_cache = reply->attribute( QNetworkRequest::SourceIsFromCacheAttribute );
    //QString url = QString(reply->url().toEncoded());
    //qDebug() << url << " --- from cache?" << fromCache.toBool() << "\n";
}

void LLNetworkAccessManager:: authenticationRequiredSlot(QNetworkReply *reply, QAuthenticator *authenticator)
{
	std::string username;
	std::string password;
	std::string url = llToStdString(reply->url());
	std::string realm = llToStdString(authenticator->realm());

	if(mBrowser->authRequest(url, realm, username, password))
	{
		// Got credentials to try, attempt auth with them.
		authenticator->setUser(QString::fromStdString(username));
		authenticator->setPassword(QString::fromStdString(password));
	}
	else
	{
		// The user cancelled, don't attempt auth.
	}
}

