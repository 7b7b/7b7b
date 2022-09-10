//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QDebug>
//#include <QApplication>
#include <QFile>
#include <QDir>
#include <QString>
#include <QTextStream>
//#include <QDesktopWidget>
//#include <QList>
//#include <QDebug>
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
        qDebug() << "Lumina does not appear to be installed correctly. Cannot find: " << LOS::LuminaShare();
        return 1;
    }
    //Setup any pre-QApplication initialization values
    LXDG::setEnvironmentVars();
    setenv("DESKTOP_SESSION","Lumina",1);
    setenv("XDG_CURRENT_DESKTOP","Lumina",1);
    unsetenv("QT_AUTO_SCREEN_SCALE_FACTOR"); //causes pixel-specific scaling issues with the desktop - turn this on after-the-fact for other apps
    //Startup the session
    LSession a(argc, argv);
    if(!a.isPrimaryProcess()) {
        return 0;
    }
    //Ensure that the user's config files exist
    /*if( LSession::checkUserFiles() ){  //make sure to create any config files before creating the QApplication
      qDebug() << "User files changed - restarting the desktop session";
      return 787; //return special restart code
    }*/
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
    qDebug() << "Finished Closing Down Lumina";
    return retCode;
}
