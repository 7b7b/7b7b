//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012-2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_DESKTOP_SESSION_H
#define _LUMINA_DESKTOP_SESSION_H

#include <QApplication>
#include <QDebug>
#include <QString>

#include <QtGui/private/qtx11extras_p.h>
#include <QEvent>
#include <QTranslator>
#include <QSettings>
#include <QProxyStyle>
#include <QList>
#include <QThread>
#include <QThread>
#include <QUrl>
#include <QClipboard>

#include "Globals.h"
#include "widgets/AppMenu.h"
#include "widgets/SystemWindow.h"
#include "desktop/LDesktop.h"

#include <LuminaX11.h>
#include <LuminaSingleApplication.h>

//SYSTEM TRAY STANDARD DEFINITIONS
#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

class LSession : public LSingleApplication{
	Q_OBJECT
public:
	LSession(int &argc, char **argv);
	~LSession();

	static bool checkUserFiles();
	//Functions to be called during startup
	void setupSession();

	//Public System Tray Functions
	QList<WId> currentTrayApps(WId visualTray);
	bool registerVisualTray(WId);
	void unregisterVisualTray(WId);

	//Public start menu buttons
	bool registerStartButton(QString ID);
	void unregisterStartButton(QString ID);

	//Special functions for XCB event filter parsing only
	//  (DO NOT USE MANUALLY)
	void RootSizeChange();
	void WindowPropertyEvent();
	void WindowPropertyEvent(WId);
	void SysTrayDockRequest(WId);
	void WindowClosedEvent(WId);
	void WindowConfigureEvent(WId);
	void WindowDamageEvent(WId);
	void WindowSelectionClearEvent(WId);

	//System Access
	//Return a pointer to the current session
	static LSession* handle(){
	  return static_cast<LSession*>(LSession::instance());
	}

	static void LaunchApplication(QString cmd);
	QFileInfoList DesktopFiles();

	QRect screenGeom(int num);

	AppMenu* applicationMenu();
	void systemWindow();
	LXCB *XCB; //class for XCB usage

	QSettings* sessionSettings();
	QSettings* DesktopPluginSettings();
	//LScreenSaver* screenSaver();

	//Keep track of which non-desktop window should be treated as active
	WId activeWindow(); //This will return the last active window if a desktop element is currently active

	//Temporarily change the session locale (nothing saved between sessions)
	void switchLocale(QString localeCode);

	//Play System Audio
	void playAudioFile(QString filepath);
	//Window Adjustment Routine (due to Fluxbox not respecting _NET_WM_STRUT)
	void adjustWindowGeom(WId win, bool maximize = false);

private:
	QList<LDesktop*> DESKTOPS;
	QFileSystemWatcher *watcher;
	QTimer *screenTimer;
	QRect screenRect;
	bool xchange; //flag for when the x11 session was adjusted

	//Internal variable for global usage
	AppMenu *appmenu;
	SystemWindow *sysWindow;
	QTranslator *currTranslator;
	QSettings *sessionsettings, *DPlugSettings;
	bool cleansession;
	//QList<QRect> savedScreens;

	//System Tray Variables
	WId SystemTrayID, VisualTrayID;
	int TrayDmgEvent, TrayDmgError;
	QList<WId> RunningTrayApps;
	bool TrayStopping;
	//Start Button Variables
	QString StartButtonID;

	//Task Manager Variables
	WId lastActiveWin;
	QList<WId> RunningApps;
	QList<WId> checkWin;
	QFileInfoList desktopFiles;

	void CleanupSession();

	int VersionStringToNumber(QString version);

    bool ignoreClipboard; // flag for (handle/store)Clipboard

public slots:
	void StartLogout();
	void StartShutdown(bool skipupdates = false);
	void StartReboot(bool skipupdates = false);
	void LockScreen();

	void reloadIconTheme();

private slots:
	void NewCommunication(QStringList);
	void launchStartupApps(); //used during initialization
	void watcherChange(QString);
	void screensChanged();
	void screenResized(int);
	void checkWindowGeoms();

	//System Tray Functions
	void startSystemTray();
	void stopSystemTray(bool detachall = false);
	void attachTrayWindow(WId);
	void removeTrayWindow(WId);

	//Internal simplification functions
	void updateDesktops();
	void registerDesktopWindows();


	void SessionEnding();

    // Clipboard
    void handleClipboard(QClipboard::Mode mode);
    void storeClipboard(QString text, QClipboard::Mode mode);

signals:
	//System Tray Signals
	void VisualTrayAvailable(); //new Visual Tray Plugin can be registered
	void TrayListChanged(); //Item added/removed from the list
	void TrayIconChanged(WId); //WinID of Tray App
	//Start Button signals
	void StartButtonAvailable();
	void StartButtonActivated();
	//Task Manager Signals
	void WindowListEvent(WId);
	void WindowListEvent();
	//General Signals
	void LocaleChanged();
	void IconThemeChanged();
	void DesktopConfigChanged();
	void SessionConfigChanged();
	void FavoritesChanged();
	void DesktopFilesChanged();
	void MediaFilesChanged();
	void WorkspaceChanged();

};

#endif
