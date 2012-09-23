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

#ifndef LLWEBPAGEOPENSHIM_H
#define LLWEBPAGEOPENSHIM_H

#include <qwebpage.h>

class LLEmbeddedBrowserWindow;
class LLWebPageOpenShim : public QWebPage
{
    Q_OBJECT

    public:
        LLWebPageOpenShim(LLEmbeddedBrowserWindow *in_window, QObject *parent = 0);
        ~LLWebPageOpenShim();
        LLEmbeddedBrowserWindow *window;
		bool matchesTarget(const std::string target);
		bool matchesUUID(const std::string uuid);
		void setProxy(const std::string &target, const std::string &uuid);

	public slots:
	    void windowCloseRequested();
		void geometryChangeRequested(const QRect& geom);
	
    protected:
        bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);
        QWebPage *createWindow(WebWindowType type);

    private:
		std::string mUUID;
		std::string mTarget;
		bool mOpeningSelf;
		bool mGeometryChangeRequested;
		bool mHasSentUUID;
		QRect mGeometry;
		
};

#endif

