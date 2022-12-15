//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012-2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LPanel.h"
#include "LSession.h"
#include <QScreen>
#include <QColor>

#include "panel-plugins/systemtray/LSysTray.h"

#define DEBUG 0

LPanel::LPanel(QSettings *file, QString scr, int num, QWidget *parent, bool reservespace) : QWidget() {
    //Take care of inputs
    reserveloc = reservespace;
    this->setMouseTracking(true);
    if(DEBUG) {
        qDebug() << " - Creating Panel:" << scr << num;
    }
    bgWindow = parent; //save for later
    //Setup the widget overlay for the entire panel to provide transparency effects
    panelArea = new QWidget(this);
    //panelArea->setAttribute(Qt::WA_TranslucentBackground);
    QBoxLayout *tmp = new QBoxLayout(QBoxLayout::LeftToRight);
    tmp->setContentsMargins(0,0,0,0);
    this->setLayout(tmp);
    tmp->addWidget(panelArea);
    settings = file;
    screenID = scr;
    panelnum = num; //save for later

	panel_csstyle  = "QWidget#LuminaPanelColor{ background: %1; }";

    QString lscreenID = QApplication::screens().at(Screen())->name();
    PPREFIX = "panel_"+lscreenID+"."+QString::number(num)+"/";
    if(DEBUG) {
        qDebug() << "Panel Prefix:" << PPREFIX;
    }
    defaultpanel = (LSession::handle()->screenGeom(Screen()).x()==0 && num==0);
    horizontal=true; //use this by default initially
    hidden = false; //use this by default
    //Setup the panel
    if(DEBUG) {
        qDebug() << " -- Setup Panel";
    }
    this->setContentsMargins(0,0,0,0);
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    this->setAttribute(Qt::WA_X11NetWmWindowTypeDock);
    this->setAttribute(Qt::WA_AlwaysShowToolTips);
    this->setAttribute(Qt::WA_TranslucentBackground);

    this->setAutoFillBackground(false);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);

    this->setWindowTitle("LuminaPanel");
    this->setObjectName("LuminaPanelBackgroundWidget");
    this->setStyleSheet("QToolButton::menu-indicator{ image: none; } QWidget#LuminaPanelBackgroundWidget{ background: transparent; }");
    panelArea->setObjectName("LuminaPanelColor");
    layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(1);

    panelArea->setLayout(layout);

    //Set special window flags on the panel for proper usage
    if(DEBUG) {
        qDebug() << "About to Show Panel";
    }
    this->show();

    LSession::handle()->XCB->SetAsPanel(this->winId());

    LSession::handle()->XCB->SetAsSticky(this->winId());
    QTimer::singleShot(1,this, SLOT(UpdatePanel()) );

    this->startTimer(10);
    this->timerEvent(nullptr);
}

LPanel::~LPanel() {

}

// A fix for gtk2 style plugins
void LPanel::timerEvent(QTimerEvent *) {
    if(!settings->value(PPREFIX+"customColor", false).toBool()) {
        panelArea->setPalette(this->style()->standardPalette());
        QRgb rgb = panelArea->palette().base().color().rgb();
        QString color = "rgba("+ QString::number(qRed(rgb)) + ", " + QString::number(qGreen(rgb)) +", "+ QString::number(qBlue(rgb)) +", 255)";
        if (styleCLR != color) {
            styleCLR = color;
            panelArea->setStyleSheet(panel_csstyle.arg(color));
        }
    }
}

int LPanel::Screen() {
    // Find the screen number associated with this ID
    QList<QScreen*> scrns = QApplication::screens();
    for(int i=0; i<scrns.length(); i++) {
        if(scrns[i]->name() == screenID) {
            return i;
        }
    }
    return -1;
}

void LPanel::prepareToClose() {
    //Go through and remove all the plugins
    for(int i=0; i<PLUGINS.length(); i++) {
        PLUGINS[i]->AboutToClose(); //any last cleanup for this plugin
        layout->takeAt(i); //remove from the layout
        PLUGINS.takeAt(i)->deleteLater(); //delete the actual widget
        LSession::processEvents();
        i--; //need to back up one space to not miss another plugin
    }
    this->hide();
}

//===========
// PUBLIC SLOTS
//===========
void LPanel::UpdatePanel(bool geomonly) {
    //Create/Update the panel as designated in the Settings file
    settings->sync(); //make sure to catch external settings changes
    //First set the geometry of the panel and send the EWMH message to reserve that space
    if(DEBUG) {
        qDebug() << "Update Panel: Geometry only=" << geomonly << "Screen Size:" << LSession::handle()->screenGeom(Screen());
    }
    hidden = settings->value(PPREFIX+"hidepanel",false).toBool();
    QString loc = settings->value(PPREFIX+"location","").toString().toLower();
    if(loc.isEmpty() && defaultpanel) {
        loc="top";
    }
    if(loc=="top" || loc=="bottom") {
        horizontal=true;
        layout->setAlignment(Qt::AlignLeft);
        layout->setDirection(QBoxLayout::LeftToRight);
    } else {
        horizontal=false;
        layout->setAlignment(Qt::AlignTop);
        layout->setDirection(QBoxLayout::TopToBottom);
    }
    int ht = qRound(settings->value(PPREFIX+"height", 30).toDouble()); //this is technically the distance into the screen from the edge
    fullwidth = ht; //save this for later
    if(ht<=1) {
        ht = 30;    //some kind of error in the saved height - use the default value
    }
    int hidesize = qRound(ht*0.01); //use 1% of the panel size
    if(hidesize<2) {
        hidesize=2;    //minimum of 2 pixels (need space for the mouse to go over it)
    }
    if(hidden) {
        viswidth = hidesize;
    }
    else {
        viswidth = ht;
    }
    if(DEBUG) {
        qDebug() << "Hidden Panel size:" << hidesize << "pixels";
    }
    //qDebug() << " - set Geometry";
    int xwid = LSession::handle()->screenGeom(Screen()).width();
    int xhi = LSession::handle()->screenGeom(Screen()).height();
    int xloc = LSession::handle()->screenGeom(Screen()).x();
    int yloc = LSession::handle()->screenGeom(Screen()).y();
    double panelPercent = settings->value(PPREFIX+"lengthPercent",100).toInt();
    if(panelPercent<1 || panelPercent>100) {
        panelPercent = 100;
    }
    panelPercent = panelPercent/100.0;
    QString panelPinLoc = settings->value(PPREFIX+"pinLocation","center").toString().toLower(); //[left/right/center] possible values (assume center otherwise)
    if(DEBUG) {
        qDebug() << " - Panel settings:" << QString::number(panelPercent)+QString("%") << panelPinLoc << loc;
    }
    //xloc=xoffset;
    if(loc=="top") { //top of screen
        QSize sz = QSize(xwid*panelPercent, ht);
        if(panelPinLoc=="left") {} // no change to xloc
        else if(panelPinLoc=="right") {
            xloc = xloc+xwid-sz.width();
        }
        else {
            xloc = xloc+((xwid-sz.width())/2) ;    //centered
        }
        //qDebug() << " - Panel Sizing:" << xloc << sz;
        this->setMinimumSize(sz);
        this->setMaximumSize(sz);
        this->setGeometry(xloc,yloc,sz.width(), sz.height());
        //qDebug() << " - Reserve Panel Localation";
        if(!hidden) {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), this->geometry(), "top");
            }
        } else {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), QRect(xloc, yloc, this->width(), hidesize), "top");
            }
            hidepoint = QPoint(xloc, yloc);
            showpoint = QPoint(xloc, yloc);
            this->move(hidepoint);
            this->resize( this->width(), viswidth);
        }
    } else if(loc=="bottom") { //bottom of screen
        QSize sz = QSize(xwid*panelPercent, ht);
        if(panelPinLoc=="left") {} // no change to xloc
        else if(panelPinLoc=="right") {
            xloc = xloc+xwid-sz.width();
        }
        else {
            xloc = xloc+((xwid-sz.width())/2) ;    //centered
        }
        this->setMinimumSize(sz);
        this->setMaximumSize(sz);
        this->setGeometry(xloc,yloc+xhi-ht,sz.width(), ht );
        if(!hidden) {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), this->geometry(), "bottom");
            }
        } else {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), QRect(xloc,yloc+ xhi-hidesize, this->width(), hidesize), "bottom");
            }
            hidepoint = QPoint(xloc, yloc+xhi-hidesize);
            showpoint = QPoint(xloc, yloc+xhi-ht);
            this->move(hidepoint); //Could bleed over onto the screen below
            this->resize( this->width(), viswidth);
        }
    } else if(loc=="left") { //left side of screen
        QSize sz = QSize(ht, xhi*panelPercent);
        if(panelPinLoc=="left") {} //this is actually the top (left of center in length dimension)
        else if(panelPinLoc=="right") {
            yloc = yloc+xhi-sz.height();
        }
        else {
            yloc = yloc+((xhi-sz.height())/2) ;    //centered
        }
        this->setMinimumSize(sz);
        this->setMaximumSize(sz);
        this->setGeometry(xloc,yloc, ht, sz.height());
        if(!hidden) {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), this->geometry(), "left");
            }
        } else {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), QRect(xloc, yloc, hidesize, sz.height()), "left");
            }
            hidepoint = QPoint(xloc, yloc);
            showpoint = QPoint(xloc, yloc);
            this->move(hidepoint); //Could bleed over onto the screen left
            this->resize( viswidth, this->height());
        }
    } else { //right side of screen
        QSize sz = QSize(ht, xhi*panelPercent);
        if(panelPinLoc=="left") {} //this is actually the top (left of center in length dimension)
        else if(panelPinLoc=="right") {
            yloc = yloc+xhi-sz.height();
        }
        else {
            yloc = yloc+((xhi-sz.height())/2) ;    //centered
        }
        this->setMinimumSize(sz);
        this->setMaximumSize(sz);
        this->setGeometry(xloc+xwid-ht,yloc,ht, sz.height());
        if(!hidden) {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), this->geometry(), "right");
            }
        } else {
            if(reserveloc) {
                LSession::handle()->XCB->ReserveLocation(this->winId(), QRect(xloc+xwid-hidesize, yloc, hidesize, sz.height()), "right");
            }
            hidepoint = QPoint(xloc+xwid-hidesize, yloc);
            showpoint = QPoint(xloc+xwid-ht, yloc);
            this->move(hidepoint); //Could bleed over onto the screen right
            this->resize( viswidth, this->height());
        }
    }
    if(DEBUG) {
        qDebug() << " - Done with panel geometry" << this->geometry();
    }
    //Double check that the "sticky" bit is set on the window state
    bool  needsticky = (reserveloc ? !LSession::handle()->XCB->WM_Get_Window_States(this->winId()).contains(LXCB::S_STICKY) : true);
    if(needsticky) {
        LSession::handle()->XCB->SetAsSticky(this->winId());
    }
    if(geomonly) {
        return;
    }
    //Now update the appearance of the toolbar
    if(settings->value(PPREFIX+"customColor", false).toBool()) {
        QString color = settings->value(PPREFIX+"color", "rgba(255,255,255,160)").toString();
		panelArea->setStyleSheet(panel_csstyle.arg(color));
    } else {
        panelArea->setPalette(this->style()->standardPalette());
        QRgb rgb = panelArea->palette().base().color().rgb();
        QString color = "rgba("+ QString::number(qRed(rgb)) + ", " + QString::number(qGreen(rgb)) +", "+ QString::number(qBlue(rgb)) +", 255)";
        styleCLR = color;
        panelArea->setStyleSheet(panel_csstyle.arg(color));
    }

    //Then go through the plugins and create them as necessary
    QStringList plugins = settings->value(PPREFIX+"pluginlist", QStringList()).toStringList();
    if(DEBUG) {
        qDebug() << " - Initialize Plugins: " << plugins;
    }
    for(int i=0; i<plugins.length(); i++) {
        //Ensure this plugin has a unique ID (NOTE: this numbering does not persist between sessions)
        if(!plugins[i].contains("---")) {
            int num=1;
            while( plugins.contains(plugins[i]+"---"+QString::number(Screen())+"."+QString::number(this->number())+"."+QString::number(num)) ) {
                num++;
            }

            plugins[i] = plugins[i]+"---"+QString::number(Screen())+"."+QString::number(this->number())+"."+QString::number(num);
            //qDebug() << "Adjust Plugin ID:" << plugins[i];
        }
        //See if this plugin is already there or in a different spot
        bool found = false;
        for(int p=0; p<PLUGINS.length(); p++) {
            if(PLUGINS[p]->type()==plugins[i]) {
                found = true; //already exists
                //Make sure the plugin layout has the correct orientation
                if(horizontal) {
                    PLUGINS[p]->layout()->setDirection(QBoxLayout::LeftToRight);
                }
                else {
                    PLUGINS[p]->layout()->setDirection(QBoxLayout::TopToBottom);
                }
                PLUGINS[p]->OrientationChange();
                //Now check the location of the plugin in the panel
                if(p!=i) { //wrong place in the panel
                    layout->takeAt(p); //remove the item from the current location
                    layout->insertWidget(i, PLUGINS[p]); //add the item into the correct location
                    PLUGINS.move(p,i); //move the identifier in the list to match
                }
                break;
            }
        }
        if(!found) {
            //New Plugin
            if(DEBUG) {
                qDebug() << " -- New Plugin:" << plugins[i] << i;
            }
            LPPlugin *plug = NewPP::createPlugin(plugins[i], panelArea, horizontal);
            if(plug != 0) {
                PLUGINS.insert(i, plug);
                layout->insertWidget(i, PLUGINS[i]);
                connect(plug, SIGNAL(MenuClosed()), this, SLOT(checkPanelFocus()));
            } else {
                //invalid plugin type
                plugins.removeAt(i); //remove this invalid plugin from the list
                i--; //make sure we don't miss the next item with the re-order
            }
        }
        //LSession::processEvents();
    }
    //Now remove any extra plugins from the end
    //qDebug() << "plugins:" << plugins;
    //qDebug() << "PLUGINS length:" << PLUGINS.length();
    for(int i=0; i<PLUGINS.length(); i++) {
        if(plugins.contains(PLUGINS[i]->type())) {
            continue;    //good plugin - skip it
        }
        if(DEBUG) {
            qDebug() << " -- Remove Plugin: " << PLUGINS[i]->type();
        }
        //If this is the system tray - stop it first
        if( PLUGINS[i]->type().startsWith("systemtray---") ) {
            static_cast<LSysTray*>(PLUGINS[i])->stop();
        }
        layout->takeAt(i); //remove from the layout
        PLUGINS.takeAt(i)->deleteLater(); //delete the actual widget
        //LSession::processEvents();
        i--; //need to back up one space to not miss another plugin
    }
    this->update();
    this->show(); //make sure the panel is visible now
    if(hidden) {
        this->move(hidepoint);
    }
    //Now go through and send the orientation update signal to each plugin
    for(int i=0; i<PLUGINS.length(); i++) {
        QTimer::singleShot(0,PLUGINS[i], SLOT(OrientationChange()));
    }
    checkPanelFocus();
    //LSession::processEvents();
}
// ===================
//     PRIVATE SLOTS
// ===================
void LPanel::checkPanelFocus() {
    //qDebug() << "Check Panel Focus:" << panelnum << viswidth << fullwidth << this->size();
    if( !this->geometry().contains(QCursor::pos()) ) {
        //Move the panel back to it's "hiding" spot
        if(hidden) {
            QSize sz(horizontal ? this->width() : viswidth, horizontal ? viswidth : this->height() );
            this->setMinimumSize(sz);
            this->setMaximumSize(sz);
            this->setGeometry( QRect(hidepoint, sz) );
        }
        //Re-active the old window
        if(LSession::handle()->activeWindow()!=0) {
            LSession::handle()->XCB->ActivateWindow(LSession::handle()->activeWindow());
        }
    } else if(hidden) {
        QSize sz(horizontal ? this->width() : fullwidth, horizontal ? fullwidth : this->height() );
        this->setMinimumSize(sz);
        this->setMaximumSize(sz);
        this->setGeometry( QRect(showpoint, sz) );
    }
    //qDebug() << " - done checking focus";
}

//===========
// PROTECTED
//===========
void LPanel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    for(int i=0; i<PLUGINS.length(); i++) {
        PLUGINS[i]->OrientationChange();
    }
}

void LPanel::paintEvent(QPaintEvent *event) {
    if(reserveloc) {
        QPainter *painter = new QPainter(this);
        //qDebug() << "Paint Panel:" << PPREFIX;
        //Make sure the base background of the event rectangle is the associated rectangle from the BGWindow
        QRect rec = event->rect();//this->geometry(); //start with the global geometry of the panel
        rec.adjust(-1,-1,2,2); //add 1 more pixel on each side
        //Need to translate that rectangle to the background image coordinates
        //qDebug() << " - Rec:" << rec << hidden << this->geometry() << bgWindow->geometry();
        rec.moveTo( bgWindow->mapFromGlobal( this->mapToGlobal(rec.topLeft()) ) ); //(rec.x()-LSession::handle()->screenGeom(Screen()).x(), rec.y()-LSession::handle()->screenGeom(Screen()).y() );
        //qDebug() << " - Adjusted Window Rec:" << rec;
        painter->drawPixmap(event->rect().adjusted(-1,-1,2,2), bgWindow->grab(rec));
        //painter->drawPixmap(event->rect().adjusted(-1,-1,2,2), QApplication::screens().at(Screen())->grabWindow(QX11Info::appRootWindow(), rec.x(), rec.y(), rec.width(), rec.height()) );
        delete(painter);
    }
    QWidget::paintEvent(event); //now pass the event along to the normal painting event
}

void LPanel::leaveEvent(QEvent *event) {
    checkPanelFocus();
    QWidget::leaveEvent(event);
}
