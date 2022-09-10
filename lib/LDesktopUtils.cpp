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
    QString ver = "1.6.2";
    return ver;
}

QString LDesktopUtils::LuminaDesktopBuildDate() {
    return "";
}

QStringList LDesktopUtils::listFavorites() {
    QStringList fav;
    fav = LUtils::readFile(QString(getenv("XDG_CONFIG_HOME"))+"/lumina-desktop/favorites.list");
    fav.removeAll(""); //remove any empty lines
    fav.removeDuplicates();
    return fav;
}

bool LDesktopUtils::saveFavorites(QStringList list) {
    list.removeDuplicates();
    //qDebug() << "Save Favorites:" << list;
    bool ok = LUtils::writeFile(QString(getenv("XDG_CONFIG_HOME"))+"/lumina-desktop/favorites.list", list, true);
    //if(ok){ fav = list; } //also save internally in case of rapid write/read of the file
    return ok;
}

bool LDesktopUtils::isFavorite(QString path) {
    QStringList fav = LDesktopUtils::listFavorites();
    for(int i=0; i<fav.length(); i++) {
        if(fav[i].endsWith("::::"+path)) {
            return true;
        }
    }
    return false;
}

bool LDesktopUtils::addFavorite(QString path, QString name) {
    //Generate the type of favorite this is
    QFileInfo info(path);
    QString type;
    if(info.isDir()) {
        type="dir";
    }
    else if(info.suffix()=="desktop") {
        type="app";
    }
    else {
        type = LXDG::findAppMimeForFile(path);
    }
    //Assign a name if none given
    if(name.isEmpty()) {
        name = info.fileName();
    }
    //qDebug() << "Add Favorite:" << path << type << name;
    //Now add it to the list
    QStringList favs = LDesktopUtils::listFavorites();
    //qDebug() << "Current Favorites:" << favs;
    bool found = false;
    for(int i=0; i<favs.length(); i++) {
        if(favs[i].endsWith("::::"+path)) {
            favs[i] = name+"::::"+type+"::::"+path;
            found = true;
        }
    }
    if(!found) {
        favs << name+"::::"+type+"::::"+path;
    }
    return LDesktopUtils::saveFavorites(favs);
}

void LDesktopUtils::removeFavorite(QString path) {
    QStringList fav = LDesktopUtils::listFavorites();
    bool changed = false;
    for(int i=0; i<fav.length(); i++) {
        if(fav[i].endsWith("::::"+path)) {
            fav.removeAt(i);
            i--;
            changed=true;
        }
    }
    if(changed) {
        LDesktopUtils::saveFavorites(fav);
    }
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

    bool skipmime = QFile::exists( QString(getenv("XDG_CONFIG_HOME"))+"/lumina-mimapps.list" );
    //qDebug() << " - Skipping mimetype default apps" << skipmime;
    QStringList sysDefaults;
    if(!skipOS) {
        sysDefaults = LUtils::readFile(LOS::AppPrefix()+"etc/luminaDesktop.conf");
    }
    if(sysDefaults.isEmpty() && !skipOS) {
        sysDefaults = LUtils::readFile(LOS::AppPrefix()+"etc/luminaDesktop.conf.dist");
    }
    if(sysDefaults.isEmpty() && !skipOS) {
        sysDefaults = LUtils::readFile(LOS::SysPrefix()+"etc/luminaDesktop.conf");
    }
    if(sysDefaults.isEmpty() && !skipOS) {
        sysDefaults = LUtils::readFile(LOS::SysPrefix()+"etc/luminaDesktop.conf.dist");
    }
    if(sysDefaults.isEmpty() && !skipOS) {
        sysDefaults = LUtils::readFile(L_ETCDIR+"/luminaDesktop.conf");
    }
    if(sysDefaults.isEmpty() && !skipOS) {
        sysDefaults = LUtils::readFile(L_ETCDIR+"/luminaDesktop.conf.dist");
    }
    if(sysDefaults.isEmpty()) {
        sysDefaults = LUtils::readFile(LOS::LuminaShare()+"luminaDesktop.conf");
    }
    //Find the number of the left-most desktop screen
    //QString screen = "0";
    int screen = 0;
    QRect screenGeom;
    QList<QScreen*> screens = QGuiApplication::screens();
    QList<QScreen*>::const_iterator it;
    int i = 0;
    for(it = screens.constBegin(); it != screens.constEnd(); ++it, ++i) {
        if((*it)->availableGeometry().x()==0) {
            screen = i;
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
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
        //Now parse the variable and put the value in the proper file

        if(var.contains("_default_")) {
            val = LUtils::AppToAbsolute(val);    //got an application/binary
        }
        //Special handling for values which need to exist first
        if(var.endsWith("_ifexists") ) {
            var = var.remove("_ifexists"); //remove this flag from the variable
            //Check if the value exists (absolute path only)
            val = LUtils::AppToAbsolute(val);
            //qDebug() << "Checking if favorite app exists:" << val;
            if(!QFile::exists(val)) {
                continue;    //skip this line - value/file does not exist
            }
        }

        //Parse/save the value
        QString sset; //temporary strings
        if(var=="session_enablenumlock") {
            sset = "EnableNumlock="+ istrue;
        }
        else if(var=="session_playloginaudio") {
            sset = "PlayStartupAudio="+istrue;
        }
        else if(var=="session_playlogoutaudio") {
            sset = "PlayLogoutAudio="+istrue;
        }
        else if(var=="session_default_terminal" && !skipmime) {
            LXDG::setDefaultAppForMime("application/terminal", val);
            //sset = "default-terminal="+val;
        } else if(var=="session_default_filemanager" && !skipmime) {
            LXDG::setDefaultAppForMime("inode/directory", val);
            //sset = "default-filemanager="+val;
            //loset = "directory="+val;
        } else if(var=="session_default_webbrowser" && !skipmime) {
            //loset = "webbrowser="+val;
            LXDG::setDefaultAppForMime("x-scheme-handler/http", val);
            LXDG::setDefaultAppForMime("x-scheme-handler/https", val);
        } else if(var=="session_default_email" && !skipmime) {
            LXDG::setDefaultAppForMime("application/email",val);
            //loset = "email="+val;
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
    //if(!lopenset.isEmpty()){ lopenset.prepend("[default]"); } //the session options exist within this set

// -- MIMETYPE DEFAULTS --
    tmp = sysDefaults.filter("mime_default_");
    for(int i=0; i<tmp.length()  && !skipmime; i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        qDebug() << "Mime entry:" << var << val;
        if(val.isEmpty()) {
            continue;
        }
        QString istrue = (val.toLower()=="true") ? "true": "false";
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
        //Now parse the variable and put the value in the proper file
        //Special handling for values which need to exist first
        if(var.endsWith("_ifexists") ) {
            var = var.remove("_ifexists"); //remove this flag from the variable
            val = LUtils::AppToAbsolute(val);
            //qDebug() << "Checking if Mime app exists:" << val;
            //Check if the value exists (absolute path only)
            if(!QFile::exists(val)) {
                continue;    //skip this line - value/file does not exist
            }
        }
        //Now turn this variable into the mimetype only
        var = var.section("_default_",1,-1);
        //qDebug() << " - Set Default Mime:" << var << val;
        LXDG::setDefaultAppForMime(var, val);
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
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
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
            //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
            if(var.contains(".")) {
                var.replace(".","_");
            }
            //Now parse the variable and put the value in the proper file
            if(var==(panvar+"_pixelsize")) {
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
            }
            else if(var==(panvar+"_autohide")) {
                deskset << "hidepanel="+istrue;
            }
            else if(var==(panvar+"_location")) {
                deskset << "location="+val.toLower();
            }
            else if(var==(panvar+"_plugins")) {
                deskset << "pluginlist="+val;
            }
            else if(var==(panvar+"_pinlocation")) {
                deskset << "pinLocation="+val.toLower();
            }
            else if(var==(panvar+"_edgepercent")) {
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
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
        //Now parse the variable and put the value in the proper file
        if(var=="menu_plugins") {
            deskset << "itemlist="+val;
        }
    }
    if(!tmp.isEmpty()) {
        deskset << "";    //space between sections
    }

    // -- FAVORITES --
    tmp = sysDefaults.filter("favorites_");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("favorites.");
    }
    for(int i=0; i<tmp.length(); i++) {
        //qDebug() << "Found Favorite Entry:" << tmp[i];
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
        //Now parse the variable and put the value in the proper file
        qDebug() << "Favorite entry:" << var << val;
        val = LUtils::AppToAbsolute(val); //turn any relative files into absolute
        if(var=="favorites_add_ifexists" && QFile::exists(val)) {
            qDebug() << " - Exists/Adding:" << val;
            LDesktopUtils::addFavorite(val);
        }
        else if(var=="favorites_add") {
            qDebug() << " - Adding:";
            LDesktopUtils::addFavorite(val);
        }
        else if(var=="favorites_remove") {
            qDebug() << " - Removing:";
            LDesktopUtils::removeFavorite(val);
        }
    }

    tmp = sysDefaults.filter("desktoplinks_");
    QString desktopFolder = LUtils::standardDirectory(LUtils::Desktop);
    desktopFolder.append("/");
    for(int i=0; i<tmp.length(); i++) {
        if(tmp[i].startsWith("#") || !tmp[i].contains("=") ) {
            continue;
        }
        QString var = tmp[i].section("=",0,0).toLower().simplified();
        QString val = tmp[i].section("=",1,1).section("#",0,0).simplified();
        val = LUtils::AppToAbsolute(val); //turn any relative files into absolute
        if(var=="desktoplinks_add" && QFile::exists(val) && !QFile::exists(desktopFolder+val.section("/",-1)) ) {
            QFile::link(val, desktopFolder+val.section("/",-1));
        }
    }
    tmp = sysDefaults.filter("theme_");
    if(tmp.isEmpty()) {
        tmp = sysDefaults.filter("theme.");
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
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
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
        //Change in 0.8.5 - use "_" instead of "." within variables names - need backwards compat for a little while
        if(var.contains(".")) {
            var.replace(".","_");
        }
        //Now parse the variable and put the value in the proper file
        if(var=="usersetup_run") {
            qDebug() << "Running user setup command:" << val;
            QProcess::execute(val);
        }
    }

}

bool LDesktopUtils::checkUserFiles(QString lastversion, QString currentversion) {
    //WARNING: Make sure you create a QApplication instance before calling this function!!!

    //internal version conversion examples:
    //  [1.0.0 -> 1000000], [1.2.3 -> 1002003], [0.6.1 -> 6001]
    //returns true if something changed
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
    int maj, mid, min; //major/middle/minor version numbers (<Major>.<Middle>.<Minor>)
    maj = mid = min = 0;
    bool ok = true;
    maj = version.section(".",0,0).toInt(&ok);
    if(ok) {
        mid = version.section(".",1,1).toInt(&ok);
    }
    else {
        maj = 0;
    }
    if(ok) {
        min = version.section(".",2,2).toInt(&ok);
    }
    else {
        mid = 0;
    }
    if(!ok) {
        min = 0;
    }
    //Now assemble the number
    //NOTE: This format allows numbers to be anywhere from 0->999 without conflict
    return (maj*1000000 + mid*1000 + min);
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
