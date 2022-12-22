//===========================================
//  Lumina-DE source code
//  Copyright (c) 2012-2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================

#include "LDesktopUtils.h"

#include <QApplication>
#include <QScreen>
#include <QSettings>

QString LDesktopUtils::LuminaDesktopVersion() {
    QString ver = "1.7.0";
    return ver;
}

QString LDesktopUtils::LuminaDesktopBuildDate() {
    return "";
}

void LDesktopUtils::LoadSystemDefaults(bool skipOS) {
    //Will create the Lumina configuration files based on the current system template (if any)
    qDebug() << "Loading System Defaults";
    //Ensure that the settings directory exists
    QString setdir = QString(getenv("XDG_CONFIG_HOME"))+"/lumina-desktop";
    if(!QFile::exists(setdir)) {
        QDir dir;
        dir.mkpath(setdir);
    }

    QStringList sysDefaults;

    if(!skipOS) {
		sysDefaults = LUtils::readFile(LOS::LuminaShare()+"luminaDesktop.conf");
    }

    //Find the number of the left-most desktop screen
    QRect screenGeom;
    QList<QScreen*> screens = QGuiApplication::screens();
    QList<QScreen*>::const_iterator it;
    for(it = screens.constBegin(); it != screens.constEnd(); ++it) {
        if((*it)->availableGeometry().x()==0) {
            screenGeom = (*it)->availableGeometry();
            break;
        }
    }
    //Now setup the default "desktopsettings.conf" and "sessionsettings.conf" files
    QStringList deskset, sesset;//, lopenset;

    // -- SESSION SETTINGS --
    QStringList tmp = sysDefaults.filter("session_");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("session.");    //for backwards compat
    }
    sesset << "[General]"; //everything is in this section
    sesset << "DesktopVersion="+LDesktopUtils::LuminaDesktopVersion();
    for(int i=0; i<tmp.length(); i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        if(val.isEmpty()) {
            continue;
        }
        QString istrue = (val.toLower()=="true") ? "true": "false";

        //Parse/save the value
        QString sset; //temporary strings
        if (var == "session_windowmanager"){
			sset = "WindowManager=" + val;
		} else if (var == "session_datetimeorder"){
			sset = "DateTimeOrder=" + val;
		} else if (var == "session_timeformat"){
			sset = "TimeFormat=" + val;
		} else if (var == "session_dateformat"){
			sset = "DateFormat=" + val;
		} else if (var == "session_shutdowncmd"){
			sset = "ShutdownCmd=" + val;
		} else if (var == "session_restartcmd"){
			sset = "RestartCmd=" + val;
		} else if (var == "session_lockcmd"){
			sset = "LockCmd=" + val;
		} else if (var == "session_applicationstyle") {
			sset = "StyleApplication=" + val;
		}
        //Put the line into the file (overwriting any previous assignment as necessary)
        if(!sset.isEmpty()) {
            //int index = sesset.indexOf(QRegExp(sset.section("=",0,0)+"=*", Qt::CaseSensitive, QRegExp::Wildcard));
            int index = sesset.indexOf(QRegularExpression(sset.section("=",0,0)+"=*"));
            if(index<0) {
                sesset << sset;    //new line
            }
            else {
                sesset[index] = sset;    //overwrite the other line
            }
        }
    }

    // -- DESKTOP SETTINGS --
    QString deskID = QApplication::primaryScreen()->name();
    //(only works for the primary desktop at the moment)
    tmp = sysDefaults.filter("desktop_");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("desktop.");    //for backwards compat
    }
    if(!tmp.isEmpty()) {
        deskset << "[desktop-"+deskID+"]";
    }
    for(int i=0; i<tmp.length(); i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        if(val.isEmpty()) {
            continue;
        }
        QString istrue = (val.toLower()=="true") ? "true": "false";
        //Now parse the variable and put the value in the proper file
        if(var=="desktop_visiblepanels") {
            deskset << "panels="+val;
        }
        else if(var=="desktop_backgroundfiles") {
            deskset << "background\\filelist="+val;
        }
        else if(var=="desktop_backgroundrotateminutes") {
            deskset << "background\\minutesToChange="+val;
        }
        else if(var=="desktop_plugins") {
            deskset << "pluginlist="+val;
        }
        else if(var=="desktop_generate_icons") {
            deskset << "generateDesktopIcons="+istrue;
        }
    }
    if(!tmp.isEmpty()) {
        deskset << "";    //space between sections
    }

    // -- PANEL SETTINGS --
    //(only works for the primary desktop at the moment)
    for(int i=1; i<11; i++) {
        QString panvar = "panel"+QString::number(i);
        tmp = sysDefaults.filter(panvar);
        if(!tmp.isEmpty()) {
            deskset << "[panel_"+deskID+"."+QString::number(i-1)+"]";
        }
        for(int j=0; j<tmp.length(); j++) {
            if(tmp[j].startsWith("#") || !tmp[j].contains("=") ) {
                continue;
            }
            QString var = tmp[j].section("=",0,0).toLower().simplified();
            QString val = tmp[j].section("=",1,1).section("#",0,0).simplified();
            if(val.isEmpty()) {
                continue;
            }
            QString istrue = (val.toLower()=="true") ? "true": "false";
            //Now parse the variable and put the value in the proper file
            if (var==(panvar+"_pixelsize")){
				//qDebug() << "Panel Size:" << val;
				if(val.contains("%")) {
					QString last = val.section("%",1,1).toLower(); //last character
					val = val.section("%",0,0);
					if(last=="h") {
						val = QString::number( qRound(screenGeom.height()*val.toDouble())/100 );    //adjust value to a percentage of the height of the screen
					}
					else if(last=="w") {
						val = QString::number( qRound(screenGeom.width()*val.toDouble())/100 );    //adjust value to a percentage of the width of the screen
					}
				}
				//qDebug() << " -- Adjusted:" << val;
				deskset << "height="+val;
			} else if (var==(panvar+"_autohide")){
				deskset << "hidepanel="+istrue;
			} else if (var==(panvar+"_location")){
				deskset << "location="+val.toLower();
			} else if (var==(panvar+"_plugins")){
				deskset << "pluginlist="+val;
			} else if (var==(panvar+"_pinlocation")){
				deskset << "pinLocation="+val.toLower();
			} else if (var==(panvar+"_edepercent")){
				deskset << "lengthPercent="+val;
			}
        }
        if(!tmp.isEmpty()) {
            deskset << "";    //space between sections
        }
    }

    // -- MENU settings --
    tmp = sysDefaults.filter("menu_");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("menu.");    //backwards compat
    }
    if(!tmp.isEmpty()) {
        deskset << "[menu]";
    }
    for(int i=0; i<tmp.length(); i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).toLower().simplified();
        if(val.isEmpty()) {
            continue;
        }
        //Now parse the variable and put the value in the proper file
        if(var=="menu_plugins") {
            deskset << "itemlist="+val;
        }
    }
    if(!tmp.isEmpty()) {
        deskset << "";    //space between sections
    }
    
    LUtils::writeFile(setdir+"/sessionsettings.conf", sesset, true);
    LUtils::writeFile(setdir+"/desktopsettings.conf", deskset, true);

    //Now run any extra config scripts or utilities as needed
    tmp = sysDefaults.filter("usersetup_run");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("usersetup.run");
    }
    for(int i=0; i<tmp.length(); i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        //Now parse the variable and put the value in the proper file
        if(var=="usersetup_run") {
            qDebug() << "Running user setup command:" << val;
            QProcess::execute(val);
        }
    }

}

bool LDesktopUtils::checkUserFiles(QString lastversion, QString currentversion) {
    //WARNING: Make sure you create a QApplication instance before calling this function!!!

    int oldversion = LDesktopUtils::VersionStringToNumber(lastversion);
    int nversion = LDesktopUtils::VersionStringToNumber(currentversion);
    bool newversion =  ( oldversion < nversion ); //increasing version number
    bool newrelease = ( lastversion.contains("-devel", Qt::CaseInsensitive) && QApplication::applicationVersion().contains("-release", Qt::CaseInsensitive) ); //Moving from devel to release
    bool firstrun = false;

    //Check for the desktop settings file
    QString dset = QString(getenv("XDG_CONFIG_HOME"))+"/lumina-desktop/desktopsettings.conf";
    if(!QFile::exists(dset)) {
        firstrun = true;
        LDesktopUtils::LoadSystemDefaults();
    }
    if(firstrun) {
        qDebug() << "First time using Lumina!!";
    }
    return (firstrun || newversion || newrelease);
}

int LDesktopUtils::VersionStringToNumber(QString version) {
    version = version.section("_",0,0).section("-",0,0); //trim any extra labels off the end
    int ver[3] = {0};
    bool ok = true;
    for (int i = 0; i < 3; i++){
		if(ok){
			ver[i] = version.section(".",i,i).toInt(&ok);
		} else {
			ver[i] = 0;
		}
	}
    //Now assemble the number
    //NOTE: This format allows numbers to be anywhere from 0->999 without conflict
    return (ver[0]*1000000 + ver[1]*1000 + ver[2]);
}

void LDesktopUtils::MigrateDesktopSettings(QSettings *settings, QString fromID, QString toID) {
    //desktop-ID
    QStringList keys = settings->allKeys();
    QStringList filter = keys.filter("desktop-"+fromID+"/");
    for(int i=0; i<filter.length(); i++) {
        settings->setValue("desktop-"+toID+"/"+filter[i].section("/",1,-1), settings->value(filter[i]));
        settings->remove(filter[i]);
    }
    //panel_ID.[number]
    filter = keys.filter("panel_"+fromID+".");
    for(int i=0; i<filter.length(); i++) {
        settings->setValue("panel_"+toID+"."+ filter[i].section("/",0,0).section(".",-1)+"/"+filter[i].section("/",1,-1), settings->value(filter[i]));
        settings->remove(filter[i]);
    }
}
