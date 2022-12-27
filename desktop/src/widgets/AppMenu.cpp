//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "AppMenu.h"
#include "LSession.h"
#include <LIconCache.h>

extern LIconCache *ICONS;

AppMenu::AppMenu(QWidget* parent) : QMenu(parent) {
    sysApps = new XDGDesktopList(this, true); //have this one automatically keep in sync
    APPS.clear();
    start(); //do the initial run during session init so things are responsive immediately.
    connect(QApplication::instance(), SIGNAL(LocaleChanged()), this, SLOT(watcherUpdate()) );
    connect(QApplication::instance(), SIGNAL(IconThemeChanged()), this, SLOT(watcherUpdate()) );
}

AppMenu::~AppMenu() {

}

QHash<QString, QList<XDGDesktop*> >* AppMenu::currentAppHash() {
    return &APPS;
}

//===========
//  PRIVATE
//===========
void AppMenu::updateAppList() {
    //watcher->removePaths(watcher->directories());
    //Make sure the title/icon are updated as well (in case of locale/icon change)
    this->setTitle(tr("Applications"));
    this->setIcon( LXDG::findIcon("system-run","") );
    //Now update the lists
    this->clear();
    APPS.clear(); //NOTE: Don't delete these pointers - the pointers are managed by the sysApps class and these are just references to them
    //qDebug() << "New Apps List:";
    if(LSession::handle()->sessionSettings()->value("AutomaticDesktopAppLinks",true).toBool() && !lastHashUpdate.isNull() ) {
        QString desktop = LUtils::standardDirectory(LUtils::Desktop);
        desktop.append("/");
        //qDebug() << "Update Desktop Folder:" << desktop << sysApps->removedApps << sysApps->newApps;
        QStringList tmp = sysApps->removedApps;
        for(int i=0; i<tmp.length() && !desktop.isEmpty(); i++) {
            //Remove any old symlinks first
            QString filename = tmp[i].section("/",-1);
            //qDebug() << "Check for symlink:" << filename;
            if( QFileInfo(desktop+filename).isSymLink() ) {
                QFile::remove(desktop+filename);
            }
        }
        tmp = sysApps->newApps;
        for(int i=0; i<tmp.length() && !desktop.isEmpty(); i++) {
            XDGDesktop *desk = sysApps->files.value(tmp[i]);
            if(desk->isHidden || !desk->isValid(false) ) {
                continue;    //skip this one
            }
            //qDebug() << "New App: " << tmp[i] << desk.filePath << "Hidden:" << desk.isHidden;
            //Create a new symlink for this file if one does not exist
            QString filename = tmp[i].section("/",-1);
            //qDebug() << "Check for symlink:" << filename;
            if(!QFile::exists(desktop+filename) ) {
                QFile::link(tmp[i], desktop+filename);
            }
        }
    }
    QList<XDGDesktop*> allfiles = sysApps->apps(false,false); //only valid, non-hidden apps
    APPS = LXDG::sortDesktopCats(allfiles);
    APPS.insert("All", LXDG::sortDesktopNames(allfiles));
    lastHashUpdate = QDateTime::currentDateTime();
    //Now fill the menu
    this->addSeparator();
    //--Now create the sub-menus
    QStringList cats = APPS.keys();
    cats.sort(); //make sure they are alphabetical
    for(int i=0; i<cats.length(); i++) {
        //Make sure they are translated and have the right icons
        QString name, icon;

        QStringList submenus = QStringList() << "All" << "Game" << "Network" << "Settings" << "Utility" << "Wine" << "Multimedia" << "Development" << "Education" << "Graphics" << "Office" << "Science" << "System";
        switch (submenus.indexOf(cats[i])) {
        case 0:
            continue;
            break;
        case 1:
            name = tr("Games");
            icon = "applications-games";
            break;
        case 2:
            name = tr("Network");
            icon = "applications-internet";
            break;
        case 3:
            name = tr("Settings");
            icon = "applications-system";
            break;
        case 4:
            name = tr("Utility");
            icon = "applications-utilities";
            break;
        case 5:
            name = tr("Wine");
            icon = "wine";
            break;
        case 6 ... 12:
            name = tr(cats[i].toUtf8().constData());
            icon = "applications-" + cats[i].toLower();
            break;
        default:
            name = tr("Unsorted");
            icon = "applications-other";
            break;
        }

        QMenu *menu = new QMenu(name, this);
        menu->setIcon( ICONS->loadIcon(icon) );
        //menu->setIcon(LXDG::findIcon(icon,""));
        connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(launchApp(QAction*)) );
        QList<XDGDesktop*> appL = APPS.value(cats[i]);
        for( int a=0; a<appL.length(); a++) {
            if(appL[a]->actions.isEmpty()) {
                //Just a single entry point - no extra actions
                QAction *act = new QAction(appL[a]->name, this);
                ICONS->loadIcon(act, appL[a]->icon);
                act->setToolTip(appL[a]->comment);
                act->setWhatsThis(appL[a]->filePath);
                menu->addAction(act);
            } else {
                //This app has additional actions - make this a sub menu
                // - first the main menu/action
                QMenu *submenu = new QMenu(appL[a]->name, this);
                submenu->setIcon( ICONS->loadIcon(appL[a]->icon ));
                //This is the normal behavior - not a special sub-action (although it needs to be at the top of the new menu)
                QAction *act = new QAction(appL[a]->name, this);
                ICONS->loadIcon(act, appL[a]->icon);
                act->setToolTip(appL[a]->comment);
                act->setWhatsThis(appL[a]->filePath);
                submenu->addAction(act);
                //Now add entries for every sub-action listed
                for(int sa=0; sa<appL[a]->actions.length(); sa++) {
                    QAction *sact = new QAction( appL[a]->actions[sa].name, this);
                    if(ICONS->exists(appL[a]->actions[sa].icon)) {
                        ICONS->loadIcon(sact, appL[a]->actions[sa].icon);
                    }
                    else {
                        ICONS->loadIcon(sact, appL[a]->icon);
                    }
                    sact->setToolTip(appL[a]->comment);
                    sact->setWhatsThis("-action "+appL[a]->actions[sa].ID+" "+appL[a]->filePath);
                    submenu->addAction(sact);
                }
                menu->addMenu(submenu);
            }
        }
        this->addMenu(menu);
    }
    emit AppMenuUpdated();
}

//=================
//  PRIVATE SLOTS
//=================
void AppMenu::start() {
    //Setup the watcher
    connect(sysApps, SIGNAL(appsUpdated()), this, SLOT(watcherUpdate()) );
    sysApps->updateList();
    //Now fill the menu the first time
    updateAppList();
}

void AppMenu::watcherUpdate() {
    updateAppList(); //Update the menu listings
}

void AppMenu::launchApp(QAction *act) {
    QString appFile = act->whatsThis();
    LSession::LaunchApplication("7b7b-open "+appFile);
}
