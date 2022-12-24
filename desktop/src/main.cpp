//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QString>
#include <QTextStream>
#include <QUrl>


#include "LSession.h"
#include "Globals.h"

#include <LuminaXDG.h> //from libLuminaUtils
#include <LuminaOS.h>
#include <LUtils.h>
#include <LDesktopUtils.h>

#define DEBUG 0

int main(int argc, char ** argv)
{
    if (argc > 1) {
        if (QString(argv[1]) == QString("--version")) {
            qDebug() << LDesktopUtils::LuminaDesktopVersion();
            return 0;
        }
    }
    if(!QFile::exists(LOS::LuminaShare())) {
        qDebug() << "7b7b does not appear to be installed correctly. Cannot find: " << LOS::LuminaShare();
        return 1;
    }
    //Setup any pre-QApplication initialization values
    LXDG::setEnvironmentVars();
	
	

	QSettings* sessionsettings = new QSettings("7b7b-desktop", "sessionsettings");

	QString OVS = sessionsettings.value("DesktopVersion","0").toString(); 
	LDesktopUtils::checkUserFiles(OVS, LDesktopUtils::LuminaDesktopVersion());

	const char* style = sessionsettings->value("StyleApplication","Fusion").toString().toStdString().c_str();

	setenv("QT_QPA_PLATFORMTHEME",style,true);

    LSession a(argc, argv);
    if(!a.isPrimaryProcess()) {
        return 0;
    }
    //Setup the log file
    QElapsedTimer *timer=0;
    if(DEBUG) {
        timer = new QElapsedTimer();
        timer->start();
    }
    if(DEBUG) {
        qDebug() << "Session Setup:" << timer->elapsed();
    }
    a.setupSession();
    if(DEBUG) {
        qDebug() << "Exec Time:" << timer->elapsed();
        delete timer;
    }
    int retCode = a.exec();
    //qDebug() << "Stopping the window manager";
    qDebug() << "Finished Closing Down 7b7b";
    return retCode;
}
