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

#include <QtGui/QtGui>
#include <llqtwebkit.h>

class WebPage : public QWidget, LLEmbeddedBrowserWindowObserver
{
    Q_OBJECT

signals:
    void locationChanged(const QString &);
    void canGoBack(bool);
    void canGoForward(bool);

public:
    WebPage(QWidget *parent = 0);
    ~WebPage();

    void onCursorChanged(const EventType& event);
    void onPageChanged(const EventType& event);
    void onNavigateBegin(const EventType& event);
    void onNavigateComplete(const EventType& event);
    void onUpdateProgress(const EventType& event);
    void onStatusTextChange(const EventType& event);
    void onLocationChange(const EventType& event);
    void onClickLinkHref(const EventType& event);
    void onClickLinkNoFollow(const EventType& event);

public slots:
    void goBack();
    void goForward();
    void reload();
    void loadUrl(const QString &);

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
private:
	void updateSLvariables();
    int mBrowserWindowId;
};

WebPage::WebPage(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    std::string applicationDir = std::string();
    std::string componentDir = applicationDir;
    std::string profileDir = applicationDir + "\\" + "testGL_profile";
    LLQtWebKit::getInstance()->init(applicationDir, componentDir, profileDir, 0);

    mBrowserWindowId = LLQtWebKit::getInstance()->createBrowserWindow(width(), height());

    // observer events that LLQtWebKit emits
    LLQtWebKit::getInstance()->addObserver( mBrowserWindowId, this );

    // append details to agent string
    LLQtWebKit::getInstance()->setBrowserAgentId("testqtapp");

    // don't flip bitmap
    LLQtWebKit::getInstance()->flipWindow(mBrowserWindowId, false);
    
	// test Second Life viewer specific functions
	LLQtWebKit::getInstance()->setSLObjectEnabled( true );				// true means feature is turned on
	LLQtWebKit::getInstance()->setAgentLanguage( "tst-en" );			// viewer language selected by agent
	LLQtWebKit::getInstance()->setAgentRegion( "QtTestAppRegion" );		// name of region where agent is located
	LLQtWebKit::getInstance()->setAgentLocation( 9.8, 7.6, 5.4 );		// agent's x,y,z location within a region
	LLQtWebKit::getInstance()->setAgentGlobalLocation( 119.8, 227.6, 335.4 );		// agent's x,y,z location within the grid
	LLQtWebKit::getInstance()->setAgentMaturity( "Very immature" );		// selected maturity level of agent
	LLQtWebKit::getInstance()->setAgentOrientation( (rand()%36000)/100.0f ); // direction avatar is facing
	LLQtWebKit::getInstance()->emitLocation();
	LLQtWebKit::getInstance()->emitLanguage();
	LLQtWebKit::getInstance()->emitMaturity();

    // go to the "home page"
    LLQtWebKit::getInstance()->navigateTo(mBrowserWindowId, "http://callum-linden.s3.amazonaws.com/browsertest.html");
}

WebPage::~WebPage()
{
    // unhook observer
    LLQtWebKit::getInstance()->remObserver( mBrowserWindowId, this );

    // clean up
    LLQtWebKit::getInstance()->reset();
}

void WebPage::updateSLvariables()
{
	// randomly update SL values to test
	LLQtWebKit::getInstance()->setAgentOrientation( (rand()%36000)/100.0f );
	LLQtWebKit::getInstance()->setAgentLocation( (rand()%25600)/100.0f, (rand()%25600)/100.0f, (rand()%25600)/100.0f );
	LLQtWebKit::getInstance()->setAgentGlobalLocation( (rand()%25600)/100.0f, (rand()%25600)/100.0f, (rand()%25600)/100.0f );

	if ( rand() % 2 )
		LLQtWebKit::getInstance()->setAgentLanguage( "One language" );
	else
		LLQtWebKit::getInstance()->setAgentLanguage( "Another language" );
	
	if ( rand() % 2 )
		LLQtWebKit::getInstance()->setAgentRegion( "Region Wibble" );
	else
		LLQtWebKit::getInstance()->setAgentRegion( "Region Flasm" );

	if ( rand() % 2 )
		LLQtWebKit::getInstance()->setAgentMaturity( "Adults only" );
	else
		LLQtWebKit::getInstance()->setAgentMaturity( "Children only" );
		
	LLQtWebKit::getInstance()->emitLocation();
	LLQtWebKit::getInstance()->emitLanguage();
	LLQtWebKit::getInstance()->emitMaturity();
}

void WebPage::onCursorChanged(const EventType& event)
{
    //qDebug() << __FUNCTION__ << QString::fromStdString(event.getEventUri());
    switch (event.getIntValue()) {
    case LLQtWebKit::C_ARROW: setCursor(QCursor(Qt::ArrowCursor)); break;
    case LLQtWebKit::C_IBEAM: setCursor(QCursor(Qt::IBeamCursor)); break;
    case LLQtWebKit::C_SPLITV: setCursor(QCursor(Qt::SplitHCursor)); break;
    case LLQtWebKit::C_SPLITH: setCursor(QCursor(Qt::SplitVCursor)); break;
    case LLQtWebKit::C_POINTINGHAND: setCursor(QCursor(Qt::PointingHandCursor)); break;
    default: break;
    }
}

void WebPage::onPageChanged(const EventType& event)
{
    Q_UNUSED(event);
    LLQtWebKit::getInstance()->grabBrowserWindow( mBrowserWindowId );
    //qDebug() << __FUNCTION__ << QString::fromStdString(event.getEventUri());
    update();
}

void WebPage::onNavigateBegin(const EventType& event)
{
    Q_UNUSED(event);
    //qDebug() << __FUNCTION__ << QString::fromStdString(event.getEventUri());
}

void WebPage::onNavigateComplete(const EventType& event)
{
    Q_UNUSED(event);
    //qDebug() << __FUNCTION__ << QString::fromStdString(event.getEventUri());
}

void WebPage::onUpdateProgress(const EventType& event)
{
    Q_UNUSED(event);
}

void WebPage::onStatusTextChange(const EventType& event)
{
    Q_UNUSED(event);
}

void WebPage::onLocationChange(const EventType& event)
{
    //qDebug() << __FUNCTION__;
    emit locationChanged(QString::fromStdString(event.getEventUri()));
    //void canGoBack(bool);
    //void canGoForward(bool);
}

void WebPage::onClickLinkHref(const EventType& event)
{
    Q_UNUSED(event);
}

void WebPage::onClickLinkNoFollow(const EventType& event)
{
    Q_UNUSED(event);
}

void WebPage::resizeEvent(QResizeEvent *event)
{
    LLQtWebKit::getInstance()->setSize(mBrowserWindowId, event->size().width(), event->size().height());
    QWidget::resizeEvent(event);
}

void WebPage::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

    int width = LLQtWebKit::getInstance()->getBrowserWidth(mBrowserWindowId);
    int height = LLQtWebKit::getInstance()->getBrowserHeight(mBrowserWindowId);
    const unsigned char* pixels = LLQtWebKit::getInstance()->getBrowserWindowPixels(mBrowserWindowId);
    QImage image(pixels, width, height, QImage::Format_RGB32);
    image = image.rgbSwapped();
    QPainter painter(this);
    painter.drawImage(QPoint(0, 0), image);
}

void WebPage::mouseDoubleClickEvent(QMouseEvent *event)
{
	LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
											LLQtWebKit::ME_MOUSE_DOUBLE_CLICK,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													event->x(), event->y(),
														LLQtWebKit::KM_MODIFIER_NONE );
}

void WebPage::mouseMoveEvent(QMouseEvent *event)
{
	updateSLvariables();
	
	LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
											LLQtWebKit::ME_MOUSE_MOVE,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													event->x(), event->y(),
														LLQtWebKit::KM_MODIFIER_NONE );
}

void WebPage::mousePressEvent(QMouseEvent *event)
{
	LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
											LLQtWebKit::ME_MOUSE_DOWN,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													event->x(), event->y(),
														LLQtWebKit::KM_MODIFIER_NONE );
}

void WebPage::mouseReleaseEvent(QMouseEvent *event)
{
	LLQtWebKit::getInstance()->mouseEvent( mBrowserWindowId,
											LLQtWebKit::ME_MOUSE_UP,
												LLQtWebKit::MB_MOUSE_BUTTON_LEFT,
													event->x(), event->y(),
														LLQtWebKit::KM_MODIFIER_NONE );

    LLQtWebKit::getInstance()->focusBrowser(mBrowserWindowId, true);
}

void WebPage::keyPressEvent(QKeyEvent *event)
{
	Q_UNUSED(event);
}

void WebPage::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    //LLQtWebKit::getInstance()->unicodeInput(mBrowserWindowId, event->text().at(0).unicode(),LLQtWebKit::KM_MODIFIER_NONE);
}

void WebPage::goBack()
{
    LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_BACK);
}

void WebPage::goForward()
{
    LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_FORWARD);
}

void WebPage::reload()
{
    LLQtWebKit::getInstance()->userAction(mBrowserWindowId, LLQtWebKit::UA_NAVIGATE_RELOAD);
}

void WebPage::loadUrl(const QString &url)
{
    LLQtWebKit::getInstance()->navigateTo(mBrowserWindowId, url.toStdString());
}

#include "ui_window.h"

class Window : public QDialog, public Ui_Dialog
{
    Q_OBJECT
public:
    Window(QWidget *parent = 0);

public slots:
    void loadUrl();
};

Window::Window(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    connect(webpage, SIGNAL(locationChanged(const QString &)),
            location, SLOT(setText(const QString &)));
    connect(webpage, SIGNAL(canGoBack(bool)),
            backButton, SLOT(setEnabled(bool)));
    connect(webpage, SIGNAL(canGoForward(bool)),
            forwardButton, SLOT(setEnabled(bool)));
    connect(backButton, SIGNAL(clicked()),
            webpage, SLOT(goBack()));
    connect(forwardButton, SIGNAL(clicked()),
            webpage, SLOT(goForward()));
    connect(reloadButton, SIGNAL(clicked()),
            webpage, SLOT(reload()));
    connect(location, SIGNAL(returnPressed()),
            this, SLOT(loadUrl()));
}

void Window::loadUrl()
{
    webpage->loadUrl(location->text());
}


int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    Window window;
    window.show();
    return application.exec();
}

#include "main.moc"
