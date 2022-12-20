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
    QStringList nameList = QStringList() << QObject::tr("Application Menu") << QObject::tr("Task Manager") << QObject::tr("Spacer")
    << QObject::tr("System Tray") << QObject::tr("Time/Date") << QObject::tr("Show Desktop");

	QStringList descriptionList = QStringList() << QObject::tr("Start menu alternative which focuses on launching applications.")
	<< QObject::tr("View and control any running application windows (group similar windows under a single button).")
	<< QObject::tr("Invisible spacer to separate plugins.") << QObject::tr("Display area for dockable system applications")
	<< QObject::tr("View the current time and date.") << QObject::tr("Hide all open windows and show the desktop");

	QStringList IDList = QStringList() << "appmenu" << "taskmanager" << "spacer" << "systemtray" << "clock" << "homebutton";

	QStringList iconList = QStringList() << "format-list-unordered" << "preferences-system-windows" << "transform-move" << "preferences-system-windows-actions"
	<< "preferences-system-time" << "user-desktop";

	LPI info;
	for (int i = 0; i < nameList.length(); i++){
		info = LPI();
		info.name = nameList[i];
		info.description = descriptionList[i];
		info.ID = IDList[i];
		info.icon = iconList[i];
		PANEL.insert(info.ID, info);
	}
}

// DESKTOP PLUGINS
void LPlugins::LoadDesktopPlugins() {
    DESKTOP.clear();
    QStringList nameList = QStringList() << QObject::tr("Application Launcher") << QObject::tr("Desktop Icons View");
	
	QStringList descriptionList = QStringList() << QObject::tr("Desktop button for launching an application")
	<< QObject::tr("Configurable area for automatically showing desktop icons");
	
	QStringList IDList = QStringList() << "applauncher" << "desktopview";
	
	QStringList iconList = QStringList() << "quickopen" << "preferences-desktop-icons";

	LPI info;
	for (int i = 0; i < nameList.length(); i++){
		info = LPI();
		info.name = nameList[i];
		info.description = descriptionList[i];
		info.ID = IDList[i];
		info.icon = iconList[i];
		DESKTOP.insert(info.ID, info);
	}
}

//    MENU PLUGINS
void LPlugins::LoadMenuPlugins() {
    MENU.clear();
	QStringList nameList = QStringList() << QObject::tr("Terminal") << QObject::tr("Browse Files") << QObject::tr("Applications") << QObject::tr("Separator")
	<< QObject::tr("Preferences") << QObject::tr("Task Manager") << QObject::tr("Custom App") << QObject::tr("Lock Session");

	QStringList descriptionList = QStringList() << QObject::tr("Start the default system terminal.")
	<< QObject::tr("Browse the system with the default file manager.") << QObject::tr("Show the system applications menu.")
	<< QObject::tr("Static horizontal line.") << QObject::tr("Show the desktop settings menu.")
	<< QObject::tr("List the open, minimized, active, and urgent application windows") << QObject::tr("Start a custom application")
	<< QObject::tr("Lock the current desktop session");

	QStringList IDList = QStringList() << "terminal" << "filemanager" << "applications" << "line" << "settings" << "windowlist" << "app" << "lockdesktop";

	QStringList iconList = QStringList() << "utilities-terminal" << "user-home" << "system-run" << "insert-horizontal-rule" << "configure"
	<< "preferences-system-windows" << "application-x-desktop" << "system-lock-screen";

	LPI info;
	for (int i = 0; i < nameList.length(); i++){
		info = LPI();
		info.name = nameList[i];
		info.description = descriptionList[i];
		info.ID = IDList[i];
		info.icon = iconList[i];
		MENU.insert(info.ID, info);
	}
}
