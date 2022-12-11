//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012-2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LSession.h"
#include <LuminaOS.h>

#include <QTime>
#include <QScreen>
#include <QtConcurrent>
#include <QMimeData>
#include "LXcbEventFilter.h"

#include <LuminaXDG.h>

//LibLumina X11 class
#include <LuminaX11.h>
#include <LUtils.h>
#include <ExternalProcess.h>
#include <LIconCache.h>

#include <unistd.h> //for usleep() usage
#define DEBUG 0

XCBEventFilter *evFilter = 0;
LIconCache *ICONS = 0;

LSession::LSession(int &argc, char ** argv) : LSingleApplication(argc, argv, "lumina-desktop") {
    xchange = false;
    if(this->isPrimaryProcess()) {
        connect(this, SIGNAL(InputsAvailable(QStringList)), this, SLOT(NewCommunication(QStringList)) );
        this->setApplicationName("Lumina Desktop Environment");
        this->setApplicationVersion( LDesktopUtils::LuminaDesktopVersion() );
        this->setOrganizationName("LuminaDesktopEnvironment");
        this->setQuitOnLastWindowClosed(false); //since the LDesktop's are not necessarily "window"s
        //Enabled a few of the simple effects by default
        this->setEffectEnabled( Qt::UI_AnimateMenu, true);
        this->setEffectEnabled( Qt::UI_AnimateCombo, true);
        this->setEffectEnabled( Qt::UI_AnimateTooltip, true);
        SystemTrayID = 0;
        VisualTrayID = 0;
        sysWindow = 0;
        TrayDmgEvent = 0;
        TrayDmgError = 0;
        lastActiveWin = 0;
        cleansession = true;
        TrayStopping = false;
        xchange = false;
        ICONS = new LIconCache(this);
        screenTimer = new QTimer(this);
        screenTimer->setSingleShot(true);
        screenTimer->setInterval(50);
        connect(screenTimer, SIGNAL(timeout()), this, SLOT(updateDesktops()) );
        for(int i=1; i<argc; i++) {
            if( QString::fromLocal8Bit(argv[i]) == "--noclean" ) {
                cleansession = false;
                break;
            }
        }
        XCB = new LXCB(); //need access to XCB data/functions right away
        //initialize the empty internal pointers to 0
        appmenu = 0;
        sessionsettings=0;
        //Setup the event filter for Qt5
        evFilter =  new XCBEventFilter(this);
        this->installNativeEventFilter( evFilter );
        connect(this, SIGNAL(screenAdded(QScreen*)), this, SLOT(screensChanged()) );
        connect(this, SIGNAL(screenRemoved(QScreen*)), this, SLOT(screensChanged()) );
        connect(this, SIGNAL(primaryScreenChanged(QScreen*)), this, SLOT(screensChanged()) );
        // Clipboard
        ignoreClipboard = false;
        qRegisterMetaType<QClipboard::Mode>("QClipboard::Mode");
        connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), this, SLOT(handleClipboard(QClipboard::Mode)));
    } //end check for primary process
}

LSession::~LSession() {
    if(this->isPrimaryProcess()) {
        for(int i=0; i<DESKTOPS.length(); i++) {
            DESKTOPS[i]->deleteLater();
        }
        appmenu->deleteLater();
    }
}

void LSession::setupSession() {    
    //Seed random number generator (if needed)
    QRandomGenerator( QTime::currentTime().msec() );

    qDebug() << "Initializing Session";
    if(QFile::exists("/tmp/.luminastopping")) {
        QFile::remove("/tmp/.luminastopping");
    }
    QElapsedTimer* timer = 0;
    if(DEBUG) {
        timer = new QElapsedTimer();
        timer->start();
        qDebug() << " - Init srand:" << timer->elapsed();
    }

    sessionsettings = new QSettings("lumina-desktop", "sessionsettings");
    DPlugSettings = new QSettings("lumina-desktop","pluginsettings/desktopsettings");
    //Load the proper translation files
    if(sessionsettings->value("ForceInitialLocale",false).toBool()) {
        //Some system locale override it in place - change the env first
        LUtils::setLocaleEnv( sessionsettings->value("InitLocale/LANG","").toString(), \
                              sessionsettings->value("InitLocale/LC_MESSAGES","").toString(), \
                              sessionsettings->value("InitLocale/LC_TIME","").toString(), \
                              sessionsettings->value("InitLocale/LC_NUMERIC","").toString(), \
                              sessionsettings->value("InitLocale/LC_MONETARY","").toString(), \
                              sessionsettings->value("InitLocale/LC_COLLATE","").toString(), \
                              sessionsettings->value("InitLocale/LC_CTYPE","").toString() );
    }
    checkUserFiles();
	
	// Window Manager
	LaunchApplicationDetached(sessionsettings->value("WindowManager", "").toString());
	
    //Initialize the internal variables
    DESKTOPS.clear();

    startSystemTray();

    //Initialize the global menus
    qDebug() << " - Initialize system menus";

    appmenu = new AppMenu();

    sysWindow = new SystemWindow();

    //Initialize the desktops

    desktopFiles = QDir(LUtils::standardDirectory(LUtils::Desktop)).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);
    updateDesktops();
    //if(DEBUG){ qDebug() << " - Process Events (6x):" << timer->elapsed();}

    //Now setup the system watcher for changes
    //qDebug() << " - Initialize file system watcher";

    if(DEBUG) {
        qDebug() << " - Init QFileSystemWatcher:" << timer->elapsed();
    }
    watcher = new QFileSystemWatcher(this);
    QString confdir = sessionsettings->fileName().section("/",0,-2);
    watcherChange(sessionsettings->fileName() );
    watcherChange( confdir+"/desktopsettings.conf" );
    watcherChange( confdir+"/favorites.list" );
    //Try to watch the localized desktop folder too
    watcherChange( LUtils::standardDirectory(LUtils::Desktop) );
    //And watch the /media directory, and /run/media/USERNAME directory
    if(QFile::exists("/media")) {
        watcherChange("/media");
    }
    QString userMedia = QString("/run/media/%1").arg(QDir::homePath().split("/").takeLast());
    if (QFile::exists(userMedia)) {
        watcherChange(userMedia);
    }
    if(!QFile::exists("/tmp/.autofs_change")) {
        system("touch /tmp/.autofs_change");
    }
    watcherChange("/tmp/.autofs_change");
    //connect internal signals/slots
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(watcherChange(QString)) );
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(watcherChange(QString)) );
    connect(this, SIGNAL(aboutToQuit()), this, SLOT(SessionEnding()) );
    //if(DEBUG){ qDebug() << " - Process Events (4x):" << timer->elapsed();}
    if(DEBUG) {
        qDebug() << " - Launch Startup Apps:" << timer->elapsed();
    }

    QTimer::singleShot(500, this, SLOT(launchStartupApps()) );
    if(DEBUG) {
        qDebug() << " - Close Splashscreen:" << timer->elapsed();
    }
    if(DEBUG) {
        qDebug() << " - Init Finished:" << timer->elapsed();
        delete timer;
    }
}

void LSession::CleanupSession() {
    //Close any running applications and tray utilities (Make sure to keep the UI interactive)
    LSession::processEvents();
    //Create a temporary flag to prevent crash dialogs from opening during cleanup
    LUtils::writeFile("/tmp/.luminastopping",QStringList() << "yes", true);

    //Stop the background system tray (detaching/closing apps as necessary)
    stopSystemTray(!cleansession);
    //Now perform any other cleanup
    if(cleansession) {
        //Close any open windows
        //qDebug() << " - Closing any open windows";
        QList<WId> WL = XCB->WindowList(true);
        for(int i=0; i<WL.length(); i++) {
            qDebug() << " - Closing window:" << XCB->WindowClass(WL[i]) << WL[i];
            XCB->CloseWindow(WL[i]);
            LSession::processEvents();
        }
        //Now wait a moment for things to close down before quitting
        for(int i=0; i<20; i++) {
            LSession::processEvents();    //1/2 second pause
            usleep(25);
        }
        //Kill any remaining windows
        WL = XCB->WindowList(true); //all workspaces
        for(int i=0; i<WL.length(); i++) {
            qDebug() << " - Window did not close, killing application:" << XCB->WindowClass(WL[i]) << WL[i];
            XCB->KillClient(WL[i]);
            LSession::processEvents();
        }
    }
    evFilter->StopEventHandling();
    //Stop the window manager
    //qDebug() << " - Stopping the window manager";
    //Now close down the desktop
    qDebug() << " - Closing down the desktop elements";
    for(int i=0; i<DESKTOPS.length(); i++) {
        DESKTOPS[i]->prepareToClose();
        //don't actually close them yet - that will happen when the session exits
        // this will leave the wallpapers up for a few moments (preventing black screens)
    }
    for(int i=0; i<20; i++) {
        LSession::processEvents();    //1/2 second pause
        usleep(25000);
    }
    //Clean up the temporary flag
    if(QFile::exists("/tmp/.luminastopping")) {
        QFile::remove("/tmp/.luminastopping");
    }
}

void LSession::NewCommunication(QStringList list) {
    if(DEBUG) {
        qDebug() << "New Communications:" << list;
    }
    for(int i=0; i<list.length(); i++) {
        if(list[i]=="--check-geoms") {
            screensChanged();
        } else if(list[i]=="--show-start") {
            emit StartButtonActivated();
        } else if(list[i]=="--logout") {
            QTimer::singleShot(1000, this, SLOT(StartLogout()));
        }
    }
}

void LSession::launchStartupApps() {
    //First start any system-defined startups, then do user defined
    qDebug() << "Launching startup applications";

    QList<XDGDesktop*> xdgapps = LXDG::findAutoStartFiles();
    for(int i=0; i<xdgapps.length(); i++) {
        //Generate command and clean up any stray "Exec" field codes (should not be any here)
        QString cmd = xdgapps[i]->getDesktopExec();
        if(cmd.contains("%")) {
            cmd = cmd.remove("%U").remove("%u").remove("%F").remove("%f").remove("%i").remove("%c").remove("%k").simplified();
        }
        //Now run the command
        if(!cmd.isEmpty()) {
            if(DEBUG) qDebug() << " - Auto-Starting File:" << xdgapps[i]->filePath;
            QStringList args = cmd.split(" ");
            args.removeFirst();
            QProcess::startDetached(cmd.split(" ")[0], args);
        }
    }
    //make sure we clean up all the xdgapps structures
    for(int i=0;  i<xdgapps.length(); i++) {
        xdgapps[i]->deleteLater();
    }
}

void LSession::StartLogout() {
    CleanupSession();
    QCoreApplication::exit(0);
}

void LSession::StartShutdown(bool skipupdates) {
    CleanupSession();

	QString cmd = sessionSettings()->value("ShutdownCmd", "").toString();

    QCoreApplication::exit(0);
}

void LSession::StartReboot(bool skipupdates) {
    CleanupSession();

	LaunchApplicationDetached(sessionSettings()->value("RestartCmd", "").toString());
    QCoreApplication::exit(0);
}

void LSession::LockScreen() {
	LaunchApplicationDetached(sessionSettings()->value("LockCmd", "").toString());
}

void LSession::watcherChange(QString changed) {
    if(DEBUG) {
        qDebug() << "Session Watcher Change:" << changed;
    }
    if(changed.endsWith("sessionsettings.conf") ) {
        sessionsettings->sync();
        //qDebug() << "Session Settings Changed";
        emit SessionConfigChanged();
    } else if(changed.endsWith("desktopsettings.conf") ) {
        emit DesktopConfigChanged();
    }
    else if(changed == LUtils::standardDirectory(LUtils::Desktop) ) {
        desktopFiles = QDir(changed).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs,QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);
        if(DEBUG) {
            qDebug() << "New Desktop Files:" << desktopFiles.length();
        }
        emit DesktopFilesChanged();
    } else if(changed.toLower() == "/media" || changed.toLower().startsWith("/run/media/") || changed == "/tmp/.autofs_change" ) {
        emit MediaFilesChanged();
    } else if(changed.endsWith("favorites.list")) {
        emit FavoritesChanged();
    }
    //Now ensure this file was not removed from the watcher
    if(!watcher->files().contains(changed) && !watcher->directories().contains(changed)) {
        if(!QFile::exists(changed)) {
            //Create the file really quick to ensure it can be watched
            //TODO
        }
        watcher->addPath(changed);
    }
}

void LSession::screensChanged() {
    qDebug() << "Screen Number Changed";
    if(screenTimer->isActive()) {
        screenTimer->stop();
    }
    screenTimer->start();
    xchange = true;
}

void LSession::checkWindowGeoms() {
    //Only do one window per run (this will be called once per new window - with time delays between)
    if(checkWin.isEmpty()) {
        return;
    }
    WId win = checkWin.takeFirst();
    if(RunningApps.contains(win) ) { //just to make sure it did not close during the delay
        adjustWindowGeom( win );
    }
}

bool LSession::checkUserFiles() {
    //internal version conversion examples:
    //  [1.0.0 -> 1000000], [1.2.3 -> 1002003], [0.6.1 -> 6001]
    qDebug() << "Check User Files";
    //char tmp[] = "junk\0";
    //int tmpN = 0;
    //QApplication A(tmpN, (char **)&tmp);
    QSettings sset("lumina-desktop", "sessionsettings");
    QString OVS = sset.value("DesktopVersion","0").toString(); //Old Version String
    qDebug() << " - Old Version:" << OVS;
    qDebug() << " - Current Version:" << LDesktopUtils::LuminaDesktopVersion();
    bool changed = LDesktopUtils::checkUserFiles(OVS, LDesktopUtils::LuminaDesktopVersion());
    qDebug() << " - Made Changes:" << changed;
    if(changed) {
        //Save the current version of the session to the settings file (for next time)
        sset.setValue("DesktopVersion", LDesktopUtils::LuminaDesktopVersion());
    }
    qDebug() << "Finished with user files check";
    //delete A;
    return changed;
}

void LSession::updateDesktops() {
    qDebug() << " - Update Desktops";
    QList<QScreen*> screens = QGuiApplication::screens();
    int sC = screens.count();
    qDebug() << "  Screen Count:" << sC;
    qDebug() << "  DESKTOPS Length:" << DESKTOPS.length();
    if(sC<1) {
        return;    //stop here - no screens available temporarily (displayport/4K issue)
    }
    screenRect = QRect(); //clear it
    QList<QScreen*>::const_iterator it;
    for(it = screens.constBegin(); it != screens.constEnd(); ++it) {
        screenRect = screenRect.united((*it)->geometry());
    }

    bool firstrun = (DESKTOPS.length()==0);
    QSettings dset("lumina-desktop", "desktopsettings");
    if(firstrun && sC==1) {
        //Sanity check - ensure the monitor ID did not change between sessions for single-monitor setups
        QString name = QApplication::screens().at(0)->name();
        if(!dset.contains("desktop-"+name+"/screen/lastHeight")) {
            //Empty Screen - find the previous one and migrate the settings over
            QStringList old = dset.allKeys().filter("desktop-").filter("/screen/lastHeight");
            QStringList lastused = dset.value("last_used_screens").toStringList();
            QString oldname;
            for(int i=0; i<old.length(); i++) {
                QString tmp = old[i].section("/",0,0).section("-",1,-1); //old desktop ID
                if(lastused.contains(tmp)) {
                    oldname = tmp;
                    break; //use the first screen that was last used
                }
            }
            if(!oldname.isEmpty()) {
                LDesktopUtils::MigrateDesktopSettings(&dset, oldname, name);
            }
        }
    }

    // If the screen count is changing on us
    if ( sC != QGuiApplication::screens().count() ) {
        qDebug() << "Screen Count changed while running";
        return;
    }

    //First clean up any current desktops
    QList<int> dnums; //keep track of which screens are already managed
    QList<QRect> geoms;
    for(int i=0; i<DESKTOPS.length(); i++) {
        if ( DESKTOPS[i]->Screen() < 0 || DESKTOPS[i]->Screen() >= sC || geoms.contains(screens.at(i)->geometry())) {
            //qDebug() << " - Close desktop:" << i;
            qDebug() << " - Close desktop on screen:" << DESKTOPS[i]->Screen();
            DESKTOPS[i]->prepareToClose();
            DESKTOPS.takeAt(i)->deleteLater();
            i--;
        } else {
            //qDebug() << " - Show desktop:" << i;
            qDebug() << " - Show desktop on screen:" << DESKTOPS[i]->Screen();
            DESKTOPS[i]->UpdateGeometry();
            DESKTOPS[i]->show();
            dnums << DESKTOPS[i]->Screen();
            geoms << screens.at(i)->geometry();
        }
    }

    //Now add any new desktops
    QStringList allNames;
    QList<QScreen*> scrns = QApplication::screens();
    for(int i=0; i<sC; i++) {
        allNames << scrns.at(i)->name();
        if(!dnums.contains(i) && !geoms.contains(screens.at(i)->geometry()) ) {
            //Start the desktop on this screen
            qDebug() << " - Start desktop on screen:" << i;
            DESKTOPS << new LDesktop(i);
            geoms << screens.at(i)->geometry();
        }
    }
    dset.setValue("last_used_screens", allNames);
    //Make sure fluxbox also gets prompted to re-load screen config if the number of screens changes in the middle of a session
    if(!firstrun && xchange) {
        qDebug() << "Update WM";
        xchange = false;
    }

    //Make sure all the background windows are registered on the system as virtual roots
    QTimer::singleShot(100,this, SLOT(registerDesktopWindows()));
}

void LSession::registerDesktopWindows() {
    QList<WId> wins;
    for(int i=0; i<DESKTOPS.length(); i++) {
        wins << DESKTOPS[i]->backgroundID();
    }
    XCB->RegisterVirtualRoots(wins);
}

void LSession::adjustWindowGeom(WId win, bool maximize) {
    //return; //temporary disable
    if(DEBUG) {
        qDebug() << "AdjustWindowGeometry():" << win << maximize << XCB->WindowClass(win);
    }
    if(XCB->WindowIsFullscreen(win) >=0 ) {
        return;    //don't touch it
    }
    //Quick hack for making sure that new windows are not located underneath any panels
    // Get the window location
    QRect geom = XCB->WindowGeometry(win, false);
    //Get the frame size
    QList<int> frame = XCB->WindowFrameGeometry(win); //[top,bottom,left,right] sizes of the frame
    //Calculate the full geometry (window + frame)
    QRect fgeom = QRect(geom.x()-frame[2], geom.y()-frame[0], geom.width()+frame[2]+frame[3], geom.height()+frame[0]+frame[1]);
    if(DEBUG) {
        qDebug() << "Check Window Geometry:" << XCB->WindowClass(win) << !geom.isNull() << geom << fgeom;
    }
    if(geom.isNull()) {
        return;    //Could not get geometry for some reason
    }
    //Get the available geometry for the screen the window is on
    QRect desk;
    QList<QScreen *> screens = QGuiApplication::screens();
    for(int i=0; i<DESKTOPS.length(); i++) {
        if( screens.at(i)->geometry().contains(geom.center()) ) {
            //Window is on this screen
            if(DEBUG) {
                qDebug() << " - On Screen:" << DESKTOPS[i]->Screen();
            }
            desk = DESKTOPS[i]->availableScreenGeom();
            if(DEBUG) {
                qDebug() << " - Screen Geom:" << desk;
            }
            break;
        }
    }
    if(desk.isNull()) {
        return;    //Unable to determine screen
    }
    //Adjust the window location if necessary
    if(maximize) {
        if(DEBUG) {
            qDebug() << " - Maximizing New Window:" << desk.width() << desk.height();
        }
        geom = desk; //Use the full screen
        XCB->MoveResizeWindow(win, geom);
        XCB->MaximizeWindow(win, true); //directly set the appropriate "maximized" flags (bypassing WM)

    } else if(!desk.contains(fgeom) ) {
        //Adjust origin point for left/top margins
        if(fgeom.y() < desk.y()) {
            geom.moveTop(desk.y()+frame[0]);    //move down to the edge (top panel)
            fgeom.moveTop(desk.y());
        }
        if(fgeom.x() < desk.x()) {
            geom.moveLeft(desk.x()+frame[2]);    //move right to the edge (left panel)
            fgeom.moveLeft(desk.x());
        }
        //Adjust size for bottom margins (within reason, since window titles are on top normally)
        // if(geom.right() > desk.right() && (geom.width() > 100)){ geom.setRight(desk.right()); }
        if(fgeom.bottom() > desk.bottom() && geom.height() > 10) {
            if(DEBUG) {
                qDebug() << "Adjust Y:" << fgeom << geom << desk;
            }
            int diff = fgeom.bottom()-desk.bottom(); //amount of overlap
            if(DEBUG) {
                qDebug() << "Y-Diff:" << diff;
            }
            if(diff < 0) {
                diff = -diff;    //need a positive value
            }
            if( (fgeom.height()+ diff)< desk.height()) {
                //just move the window - there is room for it above
                geom.setBottom(desk.bottom()-frame[1]);
                fgeom.setBottom(desk.bottom());
            } else if(geom.height() > diff) { //window bigger than the difference
                //Need to resize the window - keeping the origin point the same
                geom.setHeight( geom.height()-diff-1 ); //shrink it by the difference (need an extra pixel somewhere)
                fgeom.setHeight( fgeom.height()-diff );
            }
        }
        //Now move/resize the window
        if(DEBUG) {
            qDebug() << " - New Geom:" << geom << fgeom;
        }
        XCB->WM_Request_MoveResize_Window(win, geom);
    }

}

void LSession::SessionEnding() {
    stopSystemTray(); //just in case it was not stopped properly earlier
}

void LSession::handleClipboard(QClipboard::Mode mode) {
    if ( !ignoreClipboard && mode == QClipboard::Clipboard ) { //only support Clipboard
        const QMimeData *mime = QApplication::clipboard()->mimeData(mode);
        if (mime==NULL) {
            return;
        }
        if (mime->hasText() && !QApplication::clipboard()->ownsClipboard()) {
            //preserve the entire mimeData set, not just the text
            //Note that even when we want to "save" the test, we should keep the whole thing
            //  this preserves formatting codes and more that apps might need
            QMimeData *copy = new QMimeData();
            QStringList fmts = mime->formats();
            for(int i=0; i<fmts.length(); i++) {
                copy->setData(fmts[i], mime->data(fmts[i]));
            }
            ignoreClipboard = true;
            QApplication::clipboard()->setMimeData(copy, mode);
            ignoreClipboard = false;
            //QMetaObject::invokeMethod(this, "storeClipboard", Qt::QueuedConnection, Q_ARG(QString, mime->text()), Q_ARG(QClipboard::Mode, mode));
        }
    }
}

//===============
//  SYSTEM ACCESS
//===============
void LSession::LaunchApplication(QString cmd) {
    ExternalProcess::launch(cmd, true);
}

void LSession::LaunchApplicationDetached(QString cmd) {
    QStringList args = cmd.split(" ");
	args.removeFirst();
	QProcess::startDetached(cmd.split(" ")[0], args);

}

QFileInfoList LSession::DesktopFiles() {
    return desktopFiles;
}

QRect LSession::screenGeom(int num) {
    QList<QScreen *> screens = QGuiApplication::screens();
    if(num < 0 || num >= screens.count() ) {
        return QRect();
    }
    QRect geom = screens.at(num)->geometry();
    return geom;
}

AppMenu* LSession::applicationMenu() {
    return appmenu;
}

QSettings* LSession::sessionSettings() {
    return sessionsettings;
}

QSettings* LSession::DesktopPluginSettings() {
    return DPlugSettings;
}

WId LSession::activeWindow() {
    //Check the last active window pointer first
    WId active = XCB->ActiveWindow();
    //qDebug() << "Check Active Window:" << active << lastActiveWin;
    if(RunningApps.contains(active)) {
        lastActiveWin = active;
    }
    else if(RunningApps.contains(lastActiveWin) && XCB->WindowState(lastActiveWin) >= LXCB::VISIBLE) {} //no change needed
    else if(RunningApps.contains(lastActiveWin) && RunningApps.length()>1) {
        int start = RunningApps.indexOf(lastActiveWin);
        if(start<1) {
            lastActiveWin = RunningApps.length()-1;    //wrap around to the last item
        }
        else {
            lastActiveWin = RunningApps[start-1];
        }
    } else {
        //Need to change the last active window - find the first one which is visible
        lastActiveWin = 0; //fallback value - nothing active
        for(int i=0; i<RunningApps.length(); i++) {
            if(XCB->WindowState(RunningApps[i]) >= LXCB::VISIBLE) {
                lastActiveWin = RunningApps[i];
                break;
            }
        }
        //qDebug() << " -- New Last Active Window:" << lastActiveWin;
    }
    return lastActiveWin;
}

void LSession::systemWindow() {
    if(sysWindow==0) {
        sysWindow = new SystemWindow();
    }
    else {
        sysWindow->updateWindow();
    }
    sysWindow->show();
    //LSession::processEvents();
}

// =======================
//  XCB EVENT FILTER FUNCTIONS
// =======================
void LSession::RootSizeChange() {
    if(DESKTOPS.isEmpty() || screenRect.isNull()) {
        return;    //Initial setup not run yet
    }

    QRect tmp;
    QList<QScreen*> screens = QGuiApplication::screens();
    QList<QScreen*>::const_iterator it;
    for(it = screens.constBegin(); it != screens.constEnd(); ++it) {
        tmp = tmp.united( (*it)->geometry() );
    }
    if(tmp == screenRect) {
        return;    //false event - session size did not change
    }
    qDebug() << "Got Root Size Change";
    xchange = true;
    screenTimer->start();
}

void LSession::WindowPropertyEvent() {
    if(DEBUG) {
        qDebug() << "Window Property Event";
    }
    QList<WId> newapps = XCB->WindowList();
    if(RunningApps.length() < newapps.length()) {
        //New Window found
        //qDebug() << "New window found";
        //LSession::restoreOverrideCursor(); //restore the mouse cursor back to normal (new window opened?)
        //Perform sanity checks on any new window geometries
        for(int i=0; i<newapps.length() && !TrayStopping; i++) {
            if(!RunningApps.contains(newapps[i])) {
                checkWin << newapps[i];
                XCB->SelectInput(newapps[i]); //make sure we get property/focus events for this window
                if(DEBUG) {
                    qDebug() << "New Window - check geom in a moment:" << XCB->WindowClass(newapps[i]);
                }
                QTimer::singleShot(50, this, SLOT(checkWindowGeoms()) );
            }
        }
    }

    //Now save the list and send out the event
    RunningApps = newapps;
    emit WindowListEvent();
}

void LSession::WindowPropertyEvent(WId win) {
    //Emit the single-app signal if the window in question is one used by the task manager
    if(RunningApps.contains(win)) {
        if(DEBUG) {
            qDebug() << "Single-window property event";
        }
        WindowPropertyEvent(); //Run through the entire routine for window checks
    } else if(RunningTrayApps.contains(win)) {
        emit TrayIconChanged(win);
    }
}

void LSession::SysTrayDockRequest(WId win) {
    if(TrayStopping) {
        return;
    }
    attachTrayWindow(win); //Check to see if the window is already registered
}

void LSession::WindowClosedEvent(WId win) {
    if(TrayStopping) {
        return;
    }
    removeTrayWindow(win); //Check to see if the window is a tray app
}

void LSession::WindowConfigureEvent(WId win) {
    if(TrayStopping) {
        return;
    }
    if(RunningTrayApps.contains(win)) {
        if(DEBUG) {
            qDebug() << "SysTray: Configure Event";
        }
        emit TrayIconChanged(win); //trigger a repaint event
    } else if(RunningApps.contains(win)) {
        WindowPropertyEvent();
    }
}

void LSession::WindowDamageEvent(WId win) {
    if(TrayStopping) {
        return;
    }
    if(RunningTrayApps.contains(win)) {
        if(DEBUG) {
            qDebug() << "SysTray: Damage Event";
        }
        emit TrayIconChanged(win); //trigger a repaint event
    }
}

void LSession::WindowSelectionClearEvent(WId win) {
    if(win==SystemTrayID && !TrayStopping) {
        qDebug() << "Stopping system tray";
        stopSystemTray(true); //make sure to detach all windows
    }
}


//======================
//   SYSTEM TRAY FUNCTIONS
//======================
bool LSession::registerVisualTray(WId visualTray) {
    //Only one visual tray can be registered at a time
    //  (limitation of how tray apps are embedded)
    if(TrayStopping) {
        return false;
    }
    else if(VisualTrayID==0) {
        VisualTrayID = visualTray;
        return true;
    }
    else if(VisualTrayID==visualTray) {
        return true;
    }
    else {
        return false;
    }
}

void LSession::unregisterVisualTray(WId visualTray) {
    if(VisualTrayID==visualTray) {
        qDebug() << "Unregistered Visual Tray";
        VisualTrayID=0;
        if(!TrayStopping) {
            emit VisualTrayAvailable();
        }
    }
}

QList<WId> LSession::currentTrayApps(WId visualTray) {
    if(visualTray==VisualTrayID) {
        //Check the validity of all the current tray apps (make sure nothing closed erratically)
        for(int i=0; i<RunningTrayApps.length(); i++) {
            if(XCB->WindowClass(RunningTrayApps[i]).isEmpty()) {
                RunningTrayApps.removeAt(i);
                i--;
            }
        }
        return RunningTrayApps;
    } else if( registerVisualTray(visualTray) ) {
        return RunningTrayApps;
    } else {
        return QList<WId>();
    }
}

void LSession::startSystemTray() {
    if(SystemTrayID!=0) {
        return;
    }
    RunningTrayApps.clear(); //nothing running yet
    SystemTrayID = XCB->startSystemTray(0);
    TrayStopping = false;
    if(SystemTrayID!=0) {
        XCB->SelectInput(SystemTrayID); //make sure TrayID events get forwarded here
        TrayDmgEvent = XCB->GenerateDamageID(SystemTrayID);
        evFilter->setTrayDamageFlag(TrayDmgEvent);
        qDebug() << "System Tray Started Successfully";
        if(DEBUG) {
            qDebug() << " - System Tray Flags:" << TrayDmgEvent << TrayDmgError;
        }
    }
}

void LSession::stopSystemTray(bool detachall) {
    if(TrayStopping) {
        return;    //already run
    }
    qDebug() << "Stopping system tray...";
    TrayStopping = true; //make sure the internal list does not modified during this
    //Close all the running Tray Apps
    QList<WId> tmpApps = RunningTrayApps;
    RunningTrayApps.clear(); //clear this ahead of time so tray's do not attempt to re-access the apps
    if(!detachall) {
        for(int i=0; i<tmpApps.length(); i++) {
            qDebug() << " - Stopping tray app:" << XCB->WindowClass(tmpApps[i]);
            //Tray apps are special and closing the window does not close the app
            XCB->KillClient(tmpApps[i]);
            LSession::processEvents();
        }
    }
    //Now close down the tray backend
    XCB->closeSystemTray(SystemTrayID);
    SystemTrayID = 0;
    TrayDmgEvent = 0;
    TrayDmgError = 0;
    evFilter->setTrayDamageFlag(0); //turn off tray event handling
    emit TrayListChanged();
    LSession::processEvents();
}

void LSession::attachTrayWindow(WId win) {
    //static int appnum = 0;
    if(TrayStopping) {
        return;
    }
    if(RunningTrayApps.contains(win)) {
        return;    //already managed
    }
    qDebug() << "Session Tray: Window Added";
    RunningTrayApps << win;
    //LSession::restoreOverrideCursor();
    if(DEBUG) {
        qDebug() << "Tray List Changed";
    }
    emit TrayListChanged();
}

void LSession::removeTrayWindow(WId win) {
    if(SystemTrayID==0) {
        return;
    }
    for(int i=0; i<RunningTrayApps.length(); i++) {
        if(win==RunningTrayApps[i]) {
            qDebug() << "Session Tray: Window Removed";
            RunningTrayApps.removeAt(i);
            emit TrayListChanged();
            break;
        }
    }
}
//=========================
//  START MENU FUNCTIONS
//=========================
bool LSession::registerStartButton(QString ID) {
    if(StartButtonID.isEmpty()) {
        StartButtonID = ID;
    }
    return (StartButtonID==ID);
}

void LSession::unregisterStartButton(QString ID) {
    if(StartButtonID == ID) {
        StartButtonID.clear();
        emit StartButtonAvailable();
    }
}
