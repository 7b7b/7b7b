//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012-2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LDesktop.h"
#include "../LSession.h"
#include <QClipboard>
#include <QMimeData>
#include <QRandomGenerator>

#include <LuminaOS.h>
#include <LuminaX11.h>
#include "../LWinInfo.h"

#include <QScreen>

#define DEBUG 1

LDesktop::LDesktop(int deskNum, bool setdefault) : QObject() {
    screenID = QApplication::screens().at(deskNum)->name();
    DPREFIX = "desktop-"+screenID+"/";
    i_dlg_folder = true;
    inputDLG = 0;
    defaultdesktop = setdefault; //(desktop->screenGeometry(desktopnumber).x()==0);
    //desktoplocked = true;
    issyncing = bgupdating = false;
    usewinmenu=false;
    desktopFolderActionMenu = 0;
    //Setup the internal variables
    settings = new QSettings(QSettings::UserScope, "lumina-desktop","desktopsettings", this);
    //qDebug() << " - Desktop Settings File:" << settings->fileName();
    if(!QFile::exists(settings->fileName())) {
        settings->setValue(DPREFIX+"background/filelist",QStringList()<<"default");
        settings->sync();
    }
    //bgWindow = 0;
    bgDesktop = 0;
    QTimer::singleShot(1,this, SLOT(InitDesktop()) );

}

LDesktop::~LDesktop() {
    delete deskMenu;
    delete winMenu;
    delete desktopFolderActionMenu;
    //delete bgWindow;
    //delete workspacelabel;
    //delete wkspaceact;
}

int LDesktop::Screen() {
    QList<QScreen*> scrns = QApplication::screens();
    for(int i=0; i<scrns.length(); i++) {
        if(scrns[i]->name()==screenID) {
            return i;
        }
    }
    return -1;
}

void LDesktop::show() {
    //if(bgWindow!=0){ bgWindow->show(); }
    if(bgDesktop!=0) {
        bgDesktop->show();
    }
    for(int i=0; i<PANELS.length(); i++) {
        PANELS[i]->show();
    }
}

void LDesktop::hide() {
    //if(bgWindow!=0){ bgWindow->hide(); }
    if(bgDesktop!=0) {
        bgDesktop->hide();
    }
    for(int i=0; i<PANELS.length(); i++) {
        PANELS[i]->hide();
    }
}

void LDesktop::prepareToClose() {
    //Get any panels ready to close
    issyncing = true; //Stop handling any watcher events
    for(int i=0; i<PANELS.length(); i++) {
        PANELS[i]->prepareToClose();
        PANELS.takeAt(i)->deleteLater();
        i--;
    }
    //Now close down any desktop plugins
    //desktoplocked = true; //make sure that plugin settings are preserved during removal
    //Remove all the current containers
    bgDesktop->cleanup();
}

WId LDesktop::backgroundID() {
    if(bgDesktop!=0) {
        return bgDesktop->winId();
    }
    else {
        return QX11Info::appRootWindow();
    }
}

QRect LDesktop::availableScreenGeom() {
    //Return a QRect containing the (global) screen area that is available (not under any panels)
    if(bgDesktop!=0) {
        return globalWorkRect; //saved from previous calculations
    } else {
        return LSession::handle()->screenGeom( Screen() );
    }
}

void LDesktop::UpdateGeometry() {
    //First make sure there is something different about the geometry
    //if(desktop->screenGeometry(Screen())==bgWindow->geometry()){ return; }
    //Now update the screen
    // NOTE: This functionality is highly event-driven based on X changes - so we need to keep things in order (no signals/slots)
    //qDebug() << "Changing Desktop Geom:" << Screen();
    //bgWindow->setGeometry(desktop->screenGeometry(Screen()));
    /*for(int i=0; i<PANELS.length(); i++){
      PANELS[i]->UpdatePanel(true); //geom only updates - do this before adjusting the background
    }*/
    //qDebug() << " - Update Desktop Plugin Area";
    //UpdateDesktopPluginArea();
    //qDebug() << " - Done With Desktop Geom Updates";
    QTimer::singleShot(0, this, SLOT(UpdateBackground()));
    QTimer::singleShot(0, this, SLOT(UpdatePanels()));
}

void LDesktop::SystemLock() {
    QTimer::singleShot(30,LSession::handle(), SLOT(LockScreen()) );
    //QProcess::startDetached("xscreensaver-command -lock");
}

void LDesktop::SystemLogout() {
    LSession::handle()->systemWindow();
}
void LDesktop::SystemPreferences() {
    LSession::LaunchApplication("lumina-config");
}

void LDesktop::SystemTerminal() {
    LSession::handle()->sessionSettings()->sync(); //make sure it is up to date
    QString term = LXDG::findDefaultAppForMime("application/terminal"); //LSession::handle()->sessionSettings()->value("default-terminal","xterm").toString();
    if(term.isEmpty() ||(!term.endsWith(".desktop") && !LUtils::isValidBinary(term)) ) {
        term = "xterm";
    }
    LSession::LaunchApplication("lumina-open \""+term+"\"");
}

void LDesktop::SystemFileManager() {
    //Just open the home directory
    QString fm =  "lumina-open \""+QDir::homePath()+"\"";
    LSession::LaunchApplication(fm);
}

void LDesktop::SystemApplication(QAction* act) {
    if(!act->whatsThis().isEmpty() && act->parent()==deskMenu) {
        LSession::LaunchApplication("lumina-open \""+act->whatsThis()+"\"");
    }
}

void LDesktop::checkResolution() {
    //Compare the current screen resolution with the last one used/saved and adjust config values *only*
    //NOTE: This is only run the first time this desktop is created (before loading all the interface) - not on each desktop change
    int oldWidth = settings->value(DPREFIX+"screen/lastWidth",-1).toInt();
    int oldHeight = settings->value(DPREFIX+"screen/lastHeight",-1).toInt();
    QRect scrn = LSession::handle()->screenGeom( Screen() );
    if(scrn.isNull()) {
        return;
    }
    issyncing = true;
    settings->setValue(DPREFIX+"screen/lastWidth",scrn.width());
    settings->setValue(DPREFIX+"screen/lastHeight",scrn.height());

    if(oldWidth<1 || oldHeight<1 || scrn.width()<1 || scrn.height()<1) {
        //nothing to do - something invalid
    } else if(scrn.width()==oldWidth && scrn.height()==oldHeight) {
        //nothing to do - same as before
    } else {
        //Calculate the scale factor between the old/new sizes in each dimension
        //  and forward that on to all the interface elements
        double xscale = scrn.width()/((double) oldWidth);
        double yscale = scrn.height()/((double) oldHeight);
        if(DEBUG) {
            qDebug() << "Screen Resolution Changed:" << screenID;
            qDebug() << " - Old:" << QString::number(oldWidth)+"x"+QString::number(oldHeight);
            qDebug() << " - New:" << QString::number(scrn.width())+"x"+QString::number(scrn.height());
            qDebug() << " - Scale Factors:" << xscale << yscale;
        }
        //Update any panels in the config file
        for(int i=0; i<4; i++) {
            QString PPREFIX = "panel"+QString::number(Screen())+"."+QString::number(i)+"/";
            int ht = settings->value(PPREFIX+"height",-1).toInt();
            if(ht<1) {
                continue;    //no panel height defined
            }
            QString loc = settings->value(PPREFIX+"location","top").toString().toLower();
            if(loc=="top" || loc=="bottom") {
                settings->setValue(PPREFIX+"height", (int) ht*yscale); //vertical dimension
            } else {
                settings->setValue(PPREFIX+"height", (int) ht*xscale); //horizontal dimension
            }
        }
        //Update any desktop plugins
        QStringList plugs = settings->value(DPREFIX+"pluginlist").toStringList();
        QFileInfoList files = LSession::handle()->DesktopFiles();
        for(int i=0; i<files.length(); i++) {
            plugs << "applauncher::"+files[i].absoluteFilePath()+"---"+DPREFIX;
        }
        //QString pspath = QDir::homePath()+"/.lumina/desktop-plugins/%1.conf";
        QSettings *DP = LSession::handle()->DesktopPluginSettings();
        QStringList keys = DP->allKeys();
        for(int i=0; i<plugs.length(); i++) {
            QStringList filter = keys.filter(plugs[i]);
            for(int j=0; j<filter.length(); j++) {
                //Has existing settings - need to adjust it
                if(filter[j].endsWith("location/height")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*yscale) );
                }
                if(filter[j].endsWith("location/width")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*xscale) );
                }
                if(filter[j].endsWith("location/x")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*xscale) );
                }
                if(filter[j].endsWith("location/y")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*yscale) );
                }
                if(filter[j].endsWith("IconSize")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*yscale) );
                }
                if(filter[j].endsWith("iconsize")) {
                    DP->setValue( filter[j], qRound(DP->value(filter[j]).toInt()*yscale) );
                }
            }
        }
        DP->sync(); //make sure it gets saved to disk right away

    }
    issyncing = false;
}

// =====================
//     PRIVATE SLOTS
// =====================
void LDesktop::InitDesktop() {
    //This is called *once* during the main initialization routines
    checkResolution(); //Adjust the desktop config file first (if necessary)
    if(DEBUG) {
        qDebug() << "Init Desktop:" << Screen();
    }
    //connect(desktop, SIGNAL(resized(int)), this, SLOT(UpdateGeometry(int)));
    if(DEBUG) {
        qDebug() << "Desktop #"<<Screen()<<" -> "<< QGuiApplication::screens().at(Screen())->availableGeometry() << LSession::handle()->screenGeom(Screen());
    }
    deskMenu = new QMenu(0);
    connect(deskMenu, SIGNAL(triggered(QAction*)), this, SLOT(SystemApplication(QAction*)) );
    winMenu = new QMenu(0);
    winMenu->setTitle(tr("Window List"));
    winMenu->setIcon( LXDG::findIcon("preferences-system-windows","") );
    connect(winMenu, SIGNAL(triggered(QAction*)), this, SLOT(winClicked(QAction*)) );
    //workspacelabel = new QLabel(0);
    //  workspacelabel->setAlignment(Qt::AlignCenter);
    //wkspaceact = new QWidgetAction(0);
    //  wkspaceact->setDefaultWidget(workspacelabel);
    bgtimer = new QTimer(this);
    bgtimer->setSingleShot(true);
    connect(bgtimer, SIGNAL(timeout()), this, SLOT(UpdateBackground()) );

    connect(QApplication::instance(), SIGNAL(DesktopConfigChanged()), this, SLOT(SettingsChanged()) );
    connect(QApplication::instance(), SIGNAL(DesktopFilesChanged()), this, SLOT(UpdateDesktop()) );
    connect(QApplication::instance(), SIGNAL(MediaFilesChanged()), this, SLOT(UpdateDesktop()) );
    connect(QApplication::instance(), SIGNAL(LocaleChanged()), this, SLOT(LocaleChanged()) );
    connect(QApplication::instance(), SIGNAL(WorkspaceChanged()), this, SLOT(UpdateBackground()) );
    //if(DEBUG){ qDebug() << "Create bgWindow"; }
    /*bgWindow = new QWidget(); //LDesktopBackground();
    bgWindow->setObjectName("bgWindow");
    bgWindow->setContextMenuPolicy(Qt::CustomContextMenu);
    bgWindow->setFocusPolicy(Qt::StrongFocus);
    	bgWindow->setWindowFlags(Qt::WindowStaysOnBottomHint | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    LSession::handle()->XCB->SetAsDesktop(bgWindow->winId());
    bgWindow->setGeometry(LSession::handle()->screenGeom(Screen()));
          bgWindow->setWindowOpacity(0.0);
    connect(bgWindow, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowMenu()) );*/
    if(DEBUG) {
        qDebug() << "Create bgDesktop";
    }
    bgDesktop = new LDesktopPluginSpace();
    int grid = settings->value(DPREFIX+"GridSize",-1).toInt();
    if(grid<0 && QGuiApplication::screens().at(Screen())->availableGeometry().height() > 2000) {
        grid = 200;
    }
    else if(grid<0) {
        grid = 100;
    }
    bgDesktop->SetIconSize( grid );
    bgDesktop->setContextMenuPolicy(Qt::CustomContextMenu);
    //LSession::handle()->XCB->SetAsDesktop(bgDesktop->winId());
    connect(bgDesktop, SIGNAL(PluginRemovedByUser(QString)), this, SLOT(RemoveDeskPlugin(QString)) );
    connect(bgDesktop, SIGNAL(IncreaseIcons()), this, SLOT(IncreaseDesktopPluginIcons()) );
    connect(bgDesktop, SIGNAL(DecreaseIcons()), this, SLOT(DecreaseDesktopPluginIcons()) );
    connect(bgDesktop, SIGNAL(HideDesktopMenu()), deskMenu, SLOT(hide()));
    connect(bgDesktop, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowMenu()) );

    desktopFolderActionMenu = new QMenu(0);
    desktopFolderActionMenu->setTitle(tr("Actions"));
    desktopFolderActionMenu->setIcon(LXDG::findIcon("user-desktop",""));
    desktopFolderActionMenu->addAction(LXDG::findIcon("folder-new",""), tr("New Folder"), this, SLOT(NewDesktopFolder()) );
    desktopFolderActionMenu->addAction(LXDG::findIcon("document-new",""), tr("New File"), this, SLOT(NewDesktopFile()) );
    //desktopFolderActionMenu->addAction(LXDG::findIcon("edit-paste",""), tr("Paste"), this, SLOT(PasteInDesktop()) );
    if(DEBUG) {
        qDebug() << " - Desktop Init Done:" << screenID;
    }

    //Start the update processes
    QTimer::singleShot(10,this, SLOT(UpdateMenu()) );
    QTimer::singleShot(0,this, SLOT(UpdateBackground()) );
    QTimer::singleShot(1,this, SLOT(UpdateDesktop()) );
    QTimer::singleShot(2,this, SLOT(UpdatePanels()) );
}

void LDesktop::SettingsChanged() {
    if(issyncing) {
        return;    //don't refresh for internal modifications to the
    }
    issyncing = true;
    qDebug() << "Found Settings Change:" << screenID;
    settings->sync(); //make sure to sync with external settings changes
    UpdateBackground();
    UpdateDesktop();
    UpdatePanels();
    UpdateMenu();
    issyncing = false;
    QTimer::singleShot(50, this, SLOT(UnlockSettings()) ); //give it a few moments to settle before performing another sync
}

void LDesktop::LocaleChanged() {
    //Update any elements which require a re-translation
    UpdateMenu(false); //do the full menu refresh
}

void LDesktop::UpdateMenu(bool fast) {
    if(DEBUG) {
        qDebug() << " - Update Menu:" << screenID;
    }
    //Put a label at the top
    int num = LSession::handle()->XCB->CurrentWorkspace(); //LX11::GetCurrentDesktop();
    if(DEBUG) {
        qDebug() << "Found workspace number:" << num;
    }
    /*if(num < 0){ workspacelabel->setText( "<b>"+tr("Lumina Desktop")+"</b>"); }
    else{ workspacelabel->setText( "<b>"+QString(tr("Workspace %1")).arg(QString::number(num+1))+"</b>"); }*/
    if(fast && usewinmenu) {
        UpdateWinMenu();
    }
    if(fast) {
        return;    //already done
    }
    deskMenu->clear(); //clear it for refresh
    //deskMenu->addAction(wkspaceact);

    //Now add the desktop folder options (if desktop is icon-enabled)
    if(settings->value(DPREFIX+"generateDesktopIcons",false).toBool()) {
        deskMenu->addMenu(desktopFolderActionMenu);
    }

    deskMenu->addAction(LXDG::findIcon("edit-paste",""), tr("Paste"), this, SLOT(PasteInDesktop()) );
    deskMenu->addSeparator();
    //Now load the user's menu setup and fill the menu
    QStringList items = settings->value("menu/itemlist", QStringList()<< "terminal" << "filemanager" <<"applications" << "line" << "settings" ).toStringList();
    usewinmenu=false;
    for(int i=0; i<items.length(); i++) {
        if(items[i]=="terminal") {
            deskMenu->addAction(LXDG::findIcon("utilities-terminal",""), tr("Terminal"), this, SLOT(SystemTerminal()) );
        }
        else if(items[i]=="lockdesktop") {
            deskMenu->addAction(LXDG::findIcon("system-lock-screen",""), tr("Lock Session"), this, SLOT(SystemLock()) );
        }
        else if(items[i]=="filemanager") {
            deskMenu->addAction( LXDG::findIcon("user-home",""), tr("Browse Files"), this, SLOT(SystemFileManager()) );
        }
        else if(items[i]=="applications") {
            deskMenu->addMenu( LSession::handle()->applicationMenu() );
        }
        else if(items[i]=="line") {
            deskMenu->addSeparator();
        }
        else if(items[i]=="settings") {
            deskMenu->addAction( LXDG::findIcon("configure",""), tr("Preferences"), this, SLOT(SystemPreferences()) );
        }
        else if(items[i]=="windowlist") {
            deskMenu->addMenu( winMenu);
            usewinmenu=true;
        }
        else if(items[i].startsWith("app::::") && items[i].endsWith(".desktop")) {
            //Custom *.desktop application
            QString file = items[i].section("::::",1,1).simplified();
            XDGDesktop xdgf(file);// = LXDG::loadDesktopFile(file, ok);
            if(xdgf.type!=XDGDesktop::BAD) {
                deskMenu->addAction( LXDG::findIcon(xdgf.icon,""), xdgf.name)->setWhatsThis(file);
            } else {
                qDebug() << "Could not load application file:" << file;
            }
        }
    }
    //Now add the system quit options
    deskMenu->addSeparator();
    deskMenu->addAction(LXDG::findIcon("system-log-out",""), tr("Leave"), this, SLOT(SystemLogout()) );
}

void LDesktop::UpdateWinMenu() {
    winMenu->clear();
    //Get the current list of windows
    QList<WId> wins = LSession::handle()->XCB->WindowList();
    //Now add them to the menu
    for(int i=0; i<wins.length(); i++) {
        LWinInfo info(wins[i]);
        bool junk;
        QAction *act = winMenu->addAction( info.icon(junk), info.text() );
        act->setData( QString::number(wins[i]) );
    }
}

void LDesktop::winClicked(QAction* act) {
    LSession::handle()->XCB->ActivateWindow( act->data().toString().toULong() );
}

void LDesktop::UpdateDesktop() {
    if(DEBUG) {
        qDebug() << " - Update Desktop Plugins for screen:" << screenID;
    }
    QStringList plugins = settings->value(DPREFIX+"pluginlist", QStringList()).toStringList();
    if(defaultdesktop && plugins.isEmpty()) {
        //plugins << "sample" << "sample" << "sample";
    }
    bool changed=false; //in case the plugin list needs to be changed
    //First make sure all the plugin names are unique
    for(int i=0; i<plugins.length(); i++) {
        if(!plugins[i].contains("---") ) {
            int num=1;
            while( plugins.contains(plugins[i]+"---"+QString::number(Screen())+"."+QString::number(num)) ) {
                num++;
            }
            plugins[i] = plugins[i]+"---"+screenID+"."+QString::number(num);
            //plugins[i] = plugins[i]+"---"+QString::number(Screen())+"."+QString::number(num);
            changed=true;
        }
    }
    if(changed) {
        //save the modified plugin list to file (so per-plugin settings are preserved)
        issyncing=true; //don't let the change cause a refresh
        settings->setValue(DPREFIX+"pluginlist", plugins);
        settings->sync();
        QTimer::singleShot(200, this, SLOT(UnlockSettings()) );
    }
    //If generating desktop file launchers, add those in
    QStringList filelist;
    if(settings->value(DPREFIX+"generateDesktopIcons",false).toBool()) {
        QFileInfoList files = LSession::handle()->DesktopFiles();
        for(int i=0; i<files.length(); i++) {
            filelist << files[i].absoluteFilePath();
        }
    }
    //Also show anything available in the /media directory, and /run/media/USERNAME directory
    if(settings->value(DPREFIX+"generateMediaIcons",true).toBool()) {
        QDir media("/media");
        QStringList mediadirs = media.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for(int i=0; i<mediadirs.length(); i++) {
            filelist << media.absoluteFilePath(mediadirs[i]);
        }
        QDir userMedia(QString("/run/media/%1").arg(QDir::homePath().split("/").takeLast()));
        QStringList userMediadirs = userMedia.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for(int i=0; i<userMediadirs.length(); i++) {
            filelist << userMedia.absoluteFilePath(userMediadirs[i]);
        }
        //qDebug() << "Found media Dirs:" << mediadirs << userMediadirs;
    }
    UpdateDesktopPluginArea();
    bgDesktop->LoadItems(plugins, filelist);
}

void LDesktop::RemoveDeskPlugin(QString ID) {
    //This is called after a plugin is manually removed by the user
    //	just need to ensure that the plugin is also removed from the settings file
    QStringList plugs = settings->value(DPREFIX+"pluginlist", QStringList()).toStringList();
    if(plugs.contains(ID)) {
        plugs.removeAll(ID);
        issyncing=true; //don't let the change cause a refresh
        settings->setValue(DPREFIX+"pluginlist", plugs);
        settings->sync();
        QTimer::singleShot(200, this, SLOT(UnlockSettings()) );
    }
}

void LDesktop::IncreaseDesktopPluginIcons() {
    int cur = settings->value(DPREFIX+"GridSize",-1).toInt();
    if(cur<0 && QGuiApplication::screens().at(Screen())->availableGeometry().height() > 2000) {
        cur = 200;
    }
    else if(cur<0) {
        cur = 100;
    }
    cur+=16;
    issyncing=true; //don't let the change cause a refresh
    settings->setValue(DPREFIX+"GridSize",cur);
    settings->sync();
    QTimer::singleShot(200, this, SLOT(UnlockSettings()) );
    bgDesktop->SetIconSize(cur);
}

void LDesktop::DecreaseDesktopPluginIcons() {
    int cur = settings->value(DPREFIX+"GridSize",-1).toInt();
    if(cur<0 && QGuiApplication::screens().at(Screen())->availableGeometry().height() > 2000) {
        cur = 200;
    }
    else if(cur<0) {
        cur = 100;
    }
    if(cur<32) {
        return;    //cannot get smaller than 16x16
    }
    cur-=16;
    issyncing=true; //don't let the change cause a refresh
    settings->setValue(DPREFIX+"GridSize",cur);
    settings->sync();
    QTimer::singleShot(200, this, SLOT(UnlockSettings()) );
    bgDesktop->SetIconSize(cur);
}

void LDesktop::UpdatePanels() {
    if(DEBUG) {
        qDebug() << " - Update Panels For Screen:" << Screen();
    }
    int panels = settings->value(DPREFIX+"panels", -1).toInt();
    //if(panels==-1 && defaultdesktop){ panels=1; } //need at least 1 panel on the primary desktop
    //Remove all extra panels
    for(int i=0; i<PANELS.length(); i++) {
        if(panels <= PANELS[i]->number()) {
            if(DEBUG) {
                qDebug() << " -- Remove Panel:" << PANELS[i]->number();
            }
            PANELS[i]->prepareToClose();
            PANELS.takeAt(i)->deleteLater();
            i--;
        }
    }
    for(int i=0; i<panels; i++) {
        //Check for a panel with this number
        bool found = false;
        for(int p=0; p<PANELS.length() && !found; p++) {
            if(PANELS[p]->number() == i) {
                found = true;
                if(DEBUG) {
                    qDebug() << " -- Update panel "<< i;
                }
                //panel already exists - just update it
                QTimer::singleShot(0, PANELS[p], SLOT(UpdatePanel()) );
            }
        }
        if(!found) {
            if(DEBUG) {
                qDebug() << " -- Create panel "<< i;
            }
            //New panel
            LPanel *pan = new LPanel(settings, screenID, i, bgDesktop);
            PANELS << pan;
            pan->show();
        }
    }
    //Give it a 1/2 second before ensuring that the visible desktop area is correct
    QTimer::singleShot(1500, this, SLOT(UpdateDesktopPluginArea()) );
}

void LDesktop::UpdateDesktopPluginArea() {
    QRegion visReg(QGuiApplication::screens().at(Screen())->availableGeometry() ); //visible region (not hidden behind a panel)
    QRect rawRect = visReg.boundingRect(); //initial value (screen size)
    //qDebug() << "Update Desktop Plugin Area:" << bgWindow->geometry();
    for(int i=0; i<PANELS.length(); i++) {
        QRegion shifted = visReg;
        QString loc = settings->value(PANELS[i]->prefix()+"location","top").toString().toLower();
        int vis = PANELS[i]->visibleWidth();
        if(loc=="top") {
            if(!shifted.contains(QRect(rawRect.x(), rawRect.y(), rawRect.width(), vis))) {
                continue;
            }
            shifted.translate(0, (rawRect.top()+vis)-shifted.boundingRect().top() );
        } else if(loc=="bottom") {
            if(!shifted.contains(QRect(rawRect.x(), rawRect.bottom()-vis, rawRect.width(), vis))) {
                continue;
            }
            shifted.translate(0, (rawRect.bottom()-vis)-shifted.boundingRect().bottom());
        } else if(loc=="left") {
            if( !shifted.contains(QRect(rawRect.x(), rawRect.y(), vis,rawRect.height())) ) {
                continue;
            }
            shifted.translate((rawRect.left()+vis)-shifted.boundingRect().left(),0);
        } else { //right
            if(!shifted.contains(QRect(rawRect.right()-vis, rawRect.y(), vis,rawRect.height())) ) {
                continue;
            }
            shifted.translate((rawRect.right()-vis)-shifted.boundingRect().right(),0);
        }
        visReg = visReg.intersected( shifted );
    }
    //Now make sure the desktop plugin area is only the visible area
    QRect rec = visReg.boundingRect();
//  QRect rec = LSession::desktop()->availableGeometry(Screen());
    //qDebug() << " - DPArea: Panel-Adjusted rectangle:" << rec;
    //qDebug() << " - DPArea: Screen Geometry:" << LSession::desktop()->screenGeometry(Screen());
    //qDebug() << " - DPArea: Current Geometry:" << bgDesktop->geometry();
    //LSession::handle()->XCB->SetScreenWorkArea((unsigned int) Screen(), rec);
    //Now remove the X offset to place it on the current screen (needs widget-coords, not global)
    globalWorkRect = rec; //save this for later
    rec.moveTopLeft( QPoint( rec.x()-QGuiApplication::screens().at(Screen())->availableGeometry().x(), rec.y()-QGuiApplication::screens().at(Screen())->availableGeometry().y() ) );
    //qDebug() << "DPlug Area:" << rec << bgDesktop->geometry() << LSession::handle()->desktop()->availableGeometry(bgDesktop);
    if(rec.size().isNull() ) {
        return;    //nothing changed
    } //|| rec == bgDesktop->geometry()){return; }
    //bgDesktop->show(); //make sure Fluxbox is aware of it *before* we start moving it

    bgDesktop->setGeometry( QGuiApplication::screens().at(Screen())->geometry());
    //bgDesktop->resize(LSession::desktop()->screenGeometry(Screen()).size());
    //bgDesktop->move(LSession::desktop()->screenGeometry(Screen()).topLeft());
    bgDesktop->setDesktopArea( rec );
    bgDesktop->UpdateGeom(); //just in case the plugin space itself needs to do anything
    QTimer::singleShot(10, this, SLOT(UpdateBackground()) );
    //Re-paint the panels (just in case a plugin was underneath it and the panel is transparent)
    //for(int i=0; i<PANELS.length(); i++){ PANELS[i]->update(); }
    //Make sure to re-disable any WM control flags and reset geometry again
    LSession::handle()->XCB->SetDisableWMActions(bgDesktop->winId());
    //bgDesktop->setGeometry( LSession::desktop()->screenGeometry(Screen()));
    //qDebug() << "Desktop Geom:" << bgDesktop->geometry();
    //qDebug() << "Screen Geom:" <<  LSession::desktop()->screenGeometry(Screen());
}

void LDesktop::UpdateBackground() {
    //Get the current Background
    if(bgupdating || bgDesktop==0) {
        return;    //prevent multiple calls to this at the same time
    }
    bgupdating = true;
    if(DEBUG) {
        qDebug() << " - Update Desktop Background for screen:" << Screen();
    }
    //Get the list of background(s) to show
    QStringList bgL = settings->value(DPREFIX+"background/filelist-workspace-"+QString::number( LSession::handle()->XCB->CurrentWorkspace()), QStringList()).toStringList();
    if(bgL.isEmpty()) {
        bgL = settings->value(DPREFIX+"background/filelist", QStringList()).toStringList();
    }
    if(bgL.isEmpty()) {
        bgL << LOS::LuminaShare()+"../wallpapers/lumina-nature";    //Use this entire directory by default if nothing specified
    }
    //qDebug() << " - List:" << bgL << CBG;
    //Remove any invalid files
    for(int i=0; i<bgL.length(); i++) {
        if(bgL[i]=="default" || bgL[i].startsWith("rgb(") ) {
            continue;    //built-in definitions - treat them as valid
        }
        if(bgL[i].isEmpty()) {
            bgL.removeAt(i);
            i--;
        }
        if( !QFile::exists(bgL[i]) ) {
            //Quick Detect/replace for new path for Lumina wallpapers (change in 1.3.4)
            if(bgL[i].contains("/wallpapers/Lumina-DE/")) {
                bgL[i] = bgL[i].replace("/wallpapers/Lumina-DE/", "/wallpapers/lumina-desktop/");
                i--; //modify the path and re-check it
            } else {
                bgL.removeAt(i);
                i--;
            }
        }
    }
    if(bgL.isEmpty()) {
        bgL << "default";    //always fall back on the default
    }
    //Determine if the background needs to be changed
    //qDebug() << "BG List:" << bgL << oldBGL << CBG << bgtimer->isActive();
    if(bgL==oldBGL && !CBG.isEmpty() && bgtimer->isActive()) {
        //No background change scheduled - just update the widget
        bgDesktop->update();
        bgupdating=false;
        return;
    }
    oldBGL = bgL; //save this for later
    //Determine which background to use next
    QString bgFile;
    while(bgFile.isEmpty() || QFileInfo(bgFile).isDir()) {
        QString prefix;
        if(!bgFile.isEmpty()) {
            //Got a directory - update the list of files and re-randomize the selection
            QStringList imgs = LUtils::imageExtensions();
            for(int i=0; i<imgs.length(); i++) {
                imgs[i].prepend("*.");
            }
            QDir tdir(bgFile);
            prefix=bgFile+"/";
            bgL = tdir.entryList(imgs, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            //If directory no longer has any valid images - remove it from list and try again
            if(bgL.isEmpty()) {
                oldBGL.removeAll(bgFile); //invalid directory - remove it from the list for the moment
                bgL = oldBGL; //reset the list back to the original list (not within a directory)
            }
        }
        //Verify that there are files in the list - otherwise use the default
        if(bgL.isEmpty()) {
            bgFile="default";
            break;
        }
        int index = ( QRandomGenerator::global()->generate() % bgL.length() );
        if(index== bgL.indexOf(CBG)) { //if the current wallpaper was selected by the randomization again
            //Go to the next in the list
            if(index < 0 || index >= bgL.length()-1) {
                index = 0;    //if invalid or last item in the list - go to first
            }
            else {
                index++;    //go to next
            }
        }
        bgFile = prefix+bgL[index];
    }
    //Save this file as the current background
    CBG = bgFile;
    //qDebug() << " - Set Background to:" << CBG << index << bgL;
    if( (bgFile.toLower()=="default")) {
        bgFile = LOS::LuminaShare()+"desktop-background.jpg";
    }
    //Now set this file as the current background
    QString format = settings->value(DPREFIX+"background/format","stretch").toString();
    //bgWindow->setBackground(bgFile, format);
    QPixmap backPix = LDesktopBackground::setBackground(bgFile, format, LSession::handle()->screenGeom(Screen()));
    bgDesktop->setBackground(backPix);
    //Now reset the timer for the next change (if appropriate)
    if(bgtimer->isActive()) {
        bgtimer->stop();
    }
    if(bgL.length()>1 || oldBGL.length()>1) {
        //get the length of the timer (in minutes)
        int min = settings->value(DPREFIX+"background/minutesToChange",5).toInt();
        //restart the internal timer
        if(min > 0) {
            bgtimer->start(min*60000); //convert from minutes to milliseconds
        }
    }
    //Now update the panel backgrounds
    for(int i=0; i<PANELS.length(); i++) {
        PANELS[i]->update();
        PANELS[i]->show();
    }
    bgupdating=false;
}

//Desktop Folder Interactions
void LDesktop::i_dlg_finished(int ret) {
    if(inputDLG==0) {
        return;
    }
    QString name = inputDLG->textValue();
    inputDLG->deleteLater();
    inputDLG = 0;
    if(name.isEmpty() || ret!=QDialog::Accepted) {
        return;    //do nothing
    }
    if(i_dlg_folder) {
        NewDesktopFolder(name);
    }
    else {
        NewDesktopFile(name);
    }
}

void LDesktop::NewDesktopFolder(QString name) {
    if(name.isEmpty()) {
        i_dlg_folder = true; //creating a new folder
        if(inputDLG == 0) {
            inputDLG = new QInputDialog(0, Qt::Dialog | Qt::WindowStaysOnTopHint);
            inputDLG->setInputMode(QInputDialog::TextInput);
            inputDLG->setTextValue("");
            inputDLG->setTextEchoMode(QLineEdit::Normal);
            inputDLG->setLabelText( tr("New Folder") );
            connect(inputDLG, SIGNAL(finished(int)), this, SLOT(i_dlg_finished(int)) );
        }
        inputDLG->showNormal();
    } else {
        QDir desktop(QDir::homePath());
        if(desktop.exists(tr("Desktop"))) {
            desktop.cd(tr("Desktop"));    //translated folder
        }
        else {
            desktop.cd("Desktop");    //default/english folder
        }
        if(!desktop.exists(name)) {
            desktop.mkpath(name);
        }
    }
}

void LDesktop::NewDesktopFile(QString name) {
    if(name.isEmpty()) {
        i_dlg_folder = false; //creating a new file
        if(inputDLG == 0) {
            inputDLG = new QInputDialog(0, Qt::Dialog | Qt::WindowStaysOnTopHint);
            inputDLG->setInputMode(QInputDialog::TextInput);
            inputDLG->setTextValue("");
            inputDLG->setTextEchoMode(QLineEdit::Normal);
            inputDLG->setLabelText( tr("New File") );
            connect(inputDLG, SIGNAL(finished(int)), this, SLOT(i_dlg_finished(int)) );
        }
        inputDLG->showNormal();
    } else {
        QDir desktop(QDir::homePath());
        if(desktop.exists(tr("Desktop"))) {
            desktop.cd(tr("Desktop"));    //translated folder
        }
        else {
            desktop.cd("Desktop");    //default/english folder
        }
        if(!desktop.exists(name)) {
            QFile file(desktop.absoluteFilePath(name));
            if(file.open(QIODevice::WriteOnly) ) {
                file.close();
            }
        }
    }
}

void LDesktop::PasteInDesktop() {
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    QStringList files;
    if(mime->hasFormat("x-special/lumina-copied-files")) {
        files = QString(mime->data("x-special/lumina-copied-files")).split("\n");
    } else if(mime->hasUrls()) {
        QList<QUrl> urls = mime->urls();
        for(int i=0; i<urls.length(); i++) {
            files << QString("copy::::")+urls[i].toLocalFile();
        }
    }
    //Now go through and paste all the designated files
    QString desktop = LUtils::standardDirectory(LUtils::Desktop);
    for(int i=0; i<files.length(); i++) {
        QString path = files[i].section("::::",1,-1);
        if(!QFile::exists(path)) {
            continue;    //does not exist any more - move on to next
        }
        QString newpath = desktop+"/"+path.section("/",-1);
        if(QFile::exists(newpath)) {
            //Need to change the filename a bit to make it unique
            int n = 2;
            newpath = desktop+"/"+QString::number(n)+"_"+path.section("/",-1);
            while(QFile::exists(newpath)) {
                n++;
                newpath = desktop+"/"+QString::number(n)+"_"+path.section("/",-1);
            }
        }
        bool isdir = QFileInfo(path).isDir();
        if(files[i].section("::::",0,0)=="cut") {
            QFile::rename(path, newpath);
        } else { //copy
            if(isdir) {
                QFile::link(path, newpath);    //symlink for directories - never do a full copy
            }
            else {
                QFile::copy(path, newpath);
            }
        }
    }
}
