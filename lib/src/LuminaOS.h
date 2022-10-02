//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014-17, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
//  This is the main interface for any OS-specific system calls
//    To port Lumina to a different operating system, just create a file
//    called "LuminaOS-<Operating System>.cpp", and use that file in
//    the project (libLumina.pro) instead of LuminaOS-FreeBSD.cpp
//===========================================
#ifndef _LUMINA_LIBRARY_OS_H
#define _LUMINA_LIBRARY_OS_H

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QDir>
#include <QObject>

#include "LUtils.h"

class LOS{
public:
	//Return the name of the OS being used
	static QString OSName();

	//OS-specific prefix(s)
	static QString LuminaShare(); //Install dir for Lumina share files
	static QString AppPrefix(); //Prefix for applications (/usr/local/ on FreeBSD)
	static QString SysPrefix(); //Prefix for system (/usr/ on FreeBSD)

	//Scan for mounted external devices
	static QStringList ExternalDevicePaths(); //Returns: QStringList[<type>::::<filesystem>::::<path>]
	//Note: <type> = [USB, HDRIVE, DVD, SDCARD, UNKNOWN]

	//Check for user system permission (shutdown/restart)
	static bool userHasShutdownAccess();

	//System Shutdown
	static void systemShutdown(bool skipupdates = false); //start poweroff sequence
	//System Restart
	static void systemRestart(bool skipupdates = false); //start reboot sequence
	//Check for suspend support
	static bool systemCanSuspend();
	//Put the system into the suspend state
	static void systemSuspend();

	//Get the filesystem capacity
	static QString FileSystemCapacity(QString dir) ; //Return: percentage capacity as give by the df command
};

#endif
