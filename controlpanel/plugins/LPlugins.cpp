//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LPlugins.h"

#include <LUtils.h>

LPlugins::LPlugins() {
    LoadPanelPlugins();
    LoadDesktopPlugins();
    LoadMenuPlugins();
}

LPlugins::~LPlugins() {
}

//Plugin lists
QStringList LPlugins::panelPlugins() {
    QStringList pan = PANEL.keys();
    pan.sort();
    return pan;
}
QStringList LPlugins::desktopPlugins() {
    QStringList desk = DESKTOP.keys();
    desk.sort();
    return desk;
}
QStringList LPlugins::menuPlugins() {
    QStringList men = MENU.keys();
    men.sort();
    return men;
}
//Information on individual plugins
LPI LPlugins::panelPluginInfo(QString plug) {
    if(PANEL.contains(plug)) {
        return PANEL[plug];
    }
    else {
        return LPI();
    }
}
LPI LPlugins::desktopPluginInfo(QString plug) {
    if(DESKTOP.contains(plug)) {
        return DESKTOP[plug];
    }
    else {
        return LPI();
    }
}
LPI LPlugins::menuPluginInfo(QString plug) {
    if(MENU.contains(plug)) {
        return MENU[plug];
    }
    else {
        return LPI();
    }
}

//===================
//             PLUGINS
//===================
//   PANEL PLUGINS
void LPlugins::LoadPanelPlugins() {
    PANEL.clear();
    LPI info;
    //Application Menu
    info = LPI(); //clear it
    info.name = QObject::tr("Application Menu");
    info.description = QObject::tr("Start menu alternative which focuses on launching applications.");
    info.ID = "appmenu";
    info.icon = "format-list-unordered";
    PANEL.insert(info.ID, info);
    //Task Manager
    info = LPI(); //clear it
    info.name = QObject::tr("Task Manager");
    info.description = QObject::tr("View and control any running application windows (group similar windows under a single button).");
    info.ID = "taskmanager";
    info.icon = "preferences-system-windows";
    PANEL.insert(info.ID, info);
    //Spacer
    info = LPI(); //clear it
    info.name = QObject::tr("Spacer");
    info.description = QObject::tr("Invisible spacer to separate plugins.");
    info.ID = "spacer";
    info.icon = "transform-move";
    PANEL.insert(info.ID, info);
    //System Tray
    info = LPI(); //clear it
    info.name = QObject::tr("System Tray");
    info.description = QObject::tr("Display area for dockable system applications");
    info.ID = "systemtray";
    info.icon = "preferences-system-windows-actions";
    PANEL.insert(info.ID, info);
    //Clock
    info = LPI(); //clear it
    info.name = QObject::tr("Time/Date");
    info.description = QObject::tr("View the current time and date.");
    info.ID = "clock";
    info.icon = "preferences-system-time";
    PANEL.insert(info.ID, info);
    //Home Button
    info = LPI(); //clear it
    info.name = QObject::tr("Show Desktop");
    info.description = QObject::tr("Hide all open windows and show the desktop");
    info.ID = "homebutton";
    info.icon = "user-desktop";
    PANEL.insert(info.ID, info);
}

// DESKTOP PLUGINS
void LPlugins::LoadDesktopPlugins() {
    DESKTOP.clear();
    LPI info;
    //Application Launcher Plugin
    info = LPI(); //clear it
    info.name = QObject::tr("Application Launcher");
    info.description = QObject::tr("Desktop button for launching an application");
    info.ID = "applauncher";
    info.icon = "quickopen";
    DESKTOP.insert(info.ID, info);
    //Desktop View Plugin
    info = LPI(); //clear it
    info.name = QObject::tr("Desktop Icons View");
    info.description = QObject::tr("Configurable area for automatically showing desktop icons");
    info.ID = "desktopview";
    info.icon = "preferences-desktop-icons";
    DESKTOP.insert(info.ID, info);
}

//    MENU PLUGINS
void LPlugins::LoadMenuPlugins() {
    MENU.clear();
    //Terminal
    LPI info;
    info.name = QObject::tr("Terminal");
    info.description = QObject::tr("Start the default system terminal.");
    info.ID = "terminal";
    info.icon = "utilities-terminal";
    MENU.insert(info.ID, info);
    //File Manager
    info = LPI(); //clear it
    info.name = QObject::tr("Browse Files");
    info.description = QObject::tr("Browse the system with the default file manager.");
    info.ID = "filemanager";
    info.icon = "Insight-FileManager";
    MENU.insert(info.ID, info);
    //Applications
    info = LPI(); //clear it
    info.name = QObject::tr("Applications");
    info.description = QObject::tr("Show the system applications menu.");
    info.ID = "applications";
    info.icon = "system-run";
    MENU.insert(info.ID, info);
    //Line seperator
    info = LPI(); //clear it
    info.name = QObject::tr("Separator");
    info.description = QObject::tr("Static horizontal line.");
    info.ID = "line";
    info.icon = "insert-horizontal-rule";
    MENU.insert(info.ID, info);
    //Settings
    info = LPI(); //clear it
    info.name = QObject::tr("Preferences");
    info.description = QObject::tr("Show the desktop settings menu.");
    info.ID = "settings";
    info.icon = "configure";
    MENU.insert(info.ID, info);
    //Window List
    info = LPI(); //clear it
    info.name = QObject::tr("Task Manager");
    info.description = QObject::tr("List the open, minimized, active, and urgent application windows");
    info.ID = "windowlist";
    info.icon = "preferences-system-windows";
    MENU.insert(info.ID, info);
    //Custom Apps
    info = LPI(); //clear it
    info.name = QObject::tr("Custom App");
    info.description = QObject::tr("Start a custom application");
    info.ID = "app";
    info.icon = "application-x-desktop";
    MENU.insert(info.ID, info);
    //Lock Screen item
    info = LPI(); //clear it
    info.name = QObject::tr("Lock Session");
    info.description = QObject::tr("Lock the current desktop session");
    info.ID = "lockdesktop";
    info.icon = "system-lock-screen";
    MENU.insert(info.ID, info);
}
