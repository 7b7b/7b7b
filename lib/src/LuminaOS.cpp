//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QDebug>
#include "LuminaOS.h"
#include <unistd.h>
#include <stdio.h> // Needed for BUFSIZ

QString LOS::OSName() {
    return "Linux";
}

//OS-specific prefix(s)
QString LOS::LuminaShare() {
    return (L_SHAREDIR+"/lumina-desktop/");    //Install dir for Lumina share files
}
QString LOS::AppPrefix() {
    return "/usr/";    //Prefix for applications
}
QString LOS::SysPrefix() {
    return "/usr/";    //Prefix for system
}

// ==== ExternalDevicePaths() ====
QStringList LOS::ExternalDevicePaths() {
    //Returns: QStringList[<type>::::<filesystem>::::<path>]
    //Note: <type> = [USB, HDRIVE, DVD, SDCARD, UNKNOWN]
    QStringList devs = LUtils::getCmdOutput("mount");
    //Now check the output
    for(int i=0; i<devs.length(); i++) {
        if(devs[i].startsWith("/dev/")) {
            devs[i] = devs[i].simplified();
            QString type = devs[i].section(" ",0,0);
            type.remove("/dev/");
            //Determine the type of hardware device based on the dev node
            if(type.startsWith("sd") || type.startsWith("nvme")) {
                type = "HDRIVE";
            }
            else if(type.startsWith("sr")) {
                type="DVD";
            }
            else if(type.contains("mapper")) {
                type="LVM";
            }
            else {
                type = "UNKNOWN";
            }
            //Now put the device in the proper output format
            devs[i] = type+"::::"+devs[i].section(" ",4,4)+"::::"+devs[i].section(" ",2,2);
        } else {
            //invalid device - remove it from the list
            devs.removeAt(i);
            i--;
        }
    }
    return devs;
}

//Check for user system permission (shutdown/restart)
//Most likely the user has systemd as the main init system, this will be an issue with other init systems
bool LOS::userHasShutdownAccess() {
    return true;
}

//Check for suspend support
bool LOS::systemCanSuspend() {
    return false;
}

//Put the system into the suspend state
void LOS::systemSuspend() {

}

//file system capacity
QString LOS::FileSystemCapacity(QString dir) { //Return: percentage capacity as give by the df command
    QStringList mountInfo = LUtils::getCmdOutput("df \"" + dir+"\"");
    QString::SectionFlag skipEmpty = QString::SectionSkipEmpty;
    //we take the 5th word on the 2 line
    QString capacity = mountInfo[1].section(" ",4,4, skipEmpty) + " used";
    return capacity;
}
