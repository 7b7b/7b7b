//===========================================
//  Lumina-DE source code
//  Copyright (c) 2013-2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LuminaXDG.h"
#include "LuminaOS.h"
#include "LUtils.h"
#include <QObject>
#include <QTimer>

static QStringList mimeglobs;
static qint64 mimechecktime;

//=============================
//  XDGDesktop CLASS
//=============================
XDGDesktop::XDGDesktop(QString file, QObject *parent) 
	: QObject(parent),
	filePath(file),
	exec(""),
	tryexec("") {
    isHidden=false;
    useTerminal=false;
    startupNotify=false;
    useVGL = false;
    type = XDGDesktop::BAD;
    if(!filePath.isEmpty()) {
        sync();    //if an input file is given - go ahead and sync now
    }
}

void XDGDesktop::sync() {
    //Reset internal vars
    isHidden=false;
    useTerminal=false;
    startupNotify=false;
    type = XDGDesktop::BAD;
    exec = tryexec = "";
    //Read in the File
    if(!filePath.endsWith(".desktop")) {
        return;
    }
    lastRead = QDateTime::currentDateTime();
    QStringList file = LUtils::readFile(filePath);
    if(file.isEmpty()) {
        return;    //done with init right here - nothing to load
    }
    //Get the current localization code
    type = XDGDesktop::APP; //assume this initially if we read the file properly
    QString lang = QLocale::system().name(); //lang code
    QString slang = lang.section("_",0,0); //short lang code
    //Now start looping over the information
    XDGDesktopAction CDA; //current desktop action
    bool insection=false;
    bool inaction=false;
    for(int i=0; i<file.length(); i++) {
        QString line = file[i];
        //if(filePath.contains("pcbsd")){ qDebug() << " - Check Line:" << line << inaction << insection; }
        //Check if this is the end of a section
        if(line.startsWith("[") && inaction) {
            insection=false;
            inaction=false;
            //Add the current Action structure to the main desktop structure if appropriate
            if(!CDA.ID.isEmpty()) {
                actions << CDA;
                CDA = XDGDesktopAction();
            }
        } else if(line.startsWith("[")) {
            insection=false;
            inaction = false;
        }
        //Now check if this is the beginning of a section
        if(line=="[Desktop Entry]") {
            insection=true;
            continue;
        }
        else if(line.startsWith("[Desktop Action ")) {
            //Grab the ID of the action out of the label
            CDA.ID = line.section("]",0,0).section("Desktop Action",1,1).simplified();
            inaction = true;
            continue;
        } else if( (!insection && !inaction) || line.startsWith("#")) {
            continue;
        }
        //Now parse out the file
        line = line.simplified();
        QString var = line.section("=",0,0).simplified();
        QString loc = var.section("[",1,1).section("]",0,0).simplified(); // localization
        var = var.section("[",0,0).simplified(); //remove the localization
        QString val = line.section("=",1,50).simplified();
        if( val.count("\"")==2 && val.startsWith("\"") && val.endsWith("\"")) {
            val.chop(1);    //remove the starting/ending quotes
            val = val.remove(0,1);
        }
        //-------------------
        if(var=="Name") {
            if(insection) {
                if(loc==slang) {
                    name = val;   //short locale code
                }
                else if(loc==lang) {
                    name = val;
                }
                else if(name.isEmpty() && loc.isEmpty()) {
                    /*qDebug() << "Empty" << val;*/ name = val;
                }
            } else if(inaction) {
                if(CDA.name.isEmpty() && loc.isEmpty()) {
                    CDA.name = val;
                }
                else if(CDA.name.isEmpty() && loc==slang) {
                    CDA.name = val;    //short locale code
                }
                else if(loc == lang) {
                    CDA.name = val;
                }
            }
            //hasName = true;
        } else if(var=="GenericName" && insection) {
            if(genericName.isEmpty() && loc.isEmpty()) {
                genericName = val;
            }
            else if(genericName.isEmpty() && loc==slang) {
                genericName = val;    //short locale code
            }
            else if(loc == lang) {
                genericName = val;
            }
        } else if(var=="Comment" && insection) {
            if(comment.isEmpty() && loc.isEmpty()) {
                comment = val;
            }
            else if(comment.isEmpty() && loc==slang) {
                comment = val;    //short locale code
            }
            else if(loc == lang) {
                comment = val;
            }
        } else if(var=="Icon") {
            //Quick fix for bad-registrations which add the icon suffix for theme icons
            if(!val.startsWith("/") && val.endsWith(".png") ) {
                val = val.section(".",0,-2);
            }
            if(insection) {
                if(icon.isEmpty() && loc.isEmpty()) {
                    icon = val;
                }
                else if(icon.isEmpty() && loc==slang) {
                    icon = val;    //short locale code
                }
                else if(loc == lang) {
                    icon = val;
                }
            } else if(inaction) {
                if(CDA.icon.isEmpty() && loc.isEmpty()) {
                    CDA.icon = val;
                }
                else if(CDA.icon.isEmpty() && loc==slang) {
                    CDA.icon = val;    //short locale code
                }
                else if(loc == lang) {
                    CDA.icon = val;
                }
            }
        }
        else if( (var=="TryExec") && (tryexec.isEmpty()) && insection) {
            tryexec = val;
        }
        else if(var=="Exec") {
            if(insection && exec.isEmpty() ) {
                exec = val;
            }
            else if(inaction && CDA.exec.isEmpty() ) {
                CDA.exec = val;
            }
        }
        else if( (var=="Path") && (path.isEmpty() ) && insection) {
            path = val;
        }
        else if(var=="NoDisplay" && !isHidden && insection) {
            isHidden = (val.toLower()=="true");
        }
        else if(var=="Hidden" && !isHidden && insection) {
            isHidden = (val.toLower()=="true");
        }
        else if(var=="Categories" && insection) {
            catList = val.split(";",Qt::SkipEmptyParts);
        }
        else if(var=="OnlyShowIn" && insection) {
            showInList = val.split(";",Qt::SkipEmptyParts);
        }
        else if(var=="NotShowIn" && insection) {
            notShowInList = val.split(";",Qt::SkipEmptyParts);
        }
        else if(var=="Terminal" && insection) {
            useTerminal= (val.toLower()=="true");
        }
        else if(var=="Actions" && insection) {
            actionList = val.split(";",Qt::SkipEmptyParts);
        }
        else if(var=="MimeType" && insection) {
            mimeList = val.split(";",Qt::SkipEmptyParts);
        }
        else if(var=="Keywords" && insection) {
            if(keyList.isEmpty() && loc.isEmpty()) {
                keyList = val.split(";",Qt::SkipEmptyParts);
            }
            else if(loc == lang) {
                keyList = val.split(";",Qt::SkipEmptyParts);
            }
        }
        else if(var=="StartupNotify" && insection) {
            startupNotify = (val.toLower()=="true");
        }
        else if(var=="StartupWMClass" && insection) {
            startupWM = val;
        }
        else if(var=="URL" && insection) {
            url = val;
        }
        else if(var=="Type" && insection) {
            if(val.toLower()=="application") {
                type = XDGDesktop::APP;
            }
            else if(val.toLower()=="link") {
                type = XDGDesktop::LINK;
            }
            else if(val.toLower().startsWith("dir")) {
                type = XDGDesktop::DIR;    //older specs are "Dir", newer specs are "Directory"
            }
            else {
                type = XDGDesktop::BAD;    //Unknown type
            }
        }
    } //end reading file
    if(!CDA.ID.isEmpty()) {
        actions << CDA;    //if an action was still being read, add that to the list now
        //CDA = XDGDesktopAction();
    }

    file.clear(); //done with contents of file
    //If there are OnlyShowIn desktops listed, add them to the name
    if( !showInList.isEmpty() && !showInList.contains("7b7b", Qt::CaseInsensitive) ) {
        name.append(" ("+showInList.join(", ")+")");
    }
    //Quick fix for showing "wine" applications (which quite often don't list a category, or have other differences)
    if(catList.isEmpty() && filePath.contains("/wine/")) {
        catList << "Wine"; //Internal 7b7b category only (not in XDG specs as of 11/14/14)
        //Also add a fix for the location of Wine icons
        if( !icon.isEmpty() ) {
            QStringList sizes;
            sizes << "256x256" << "128x128" << "64x64" << "48x48" << "32x32" << "16x16";
            QString upath = QDir::homePath()+"/.local/share/icons/hicolor/%1/apps/%2.png";
            //qDebug() << "Wine App: Check icon" << upath;
            for(int i=0; i<sizes.length(); i++) {
                if( QFile::exists(upath.arg(sizes[i],icon)) ) {
                    icon = upath.arg(sizes[i],icon);
                    //qDebug() << " - Found Icon:" << icon;
                    break;
                }
            }
        }
    }

}


bool XDGDesktop::isValid(bool showAll) {
    bool ok=true;
    //bool DEBUG = false;
    //if(DEBUG){ qDebug() << "[LXDG] Check File validity:" << dFile.name << dFile.filePath; }
    switch (type) {
    case XDGDesktop::BAD:
        ok=false;
        //if(DEBUG){ qDebug() << " - Bad file type"; }
        break;
    case XDGDesktop::APP:
        if(!tryexec.isEmpty() && !LXDG::checkExec(tryexec)) {
            ok=false;
        }//if(DEBUG){ qDebug() << " - tryexec does not exist";} }
        else if(exec.isEmpty() || name.isEmpty()) {
            ok=false;
        }//if(DEBUG){ qDebug() << " - exec or name is empty";} }
        else if(!LXDG::checkExec(exec.section(" ",0,0,QString::SectionSkipEmpty)) ) {
            ok=false;
        }//if(DEBUG){ qDebug() << " - first exec binary does not exist";} }
        break;
    case XDGDesktop::LINK:
        ok = !url.isEmpty();
        //if(DEBUG && !ok){ qDebug() << " - Link with missing URL"; }
        break;
    case XDGDesktop::DIR:
        ok = !path.isEmpty() && QFile::exists(path);
        //if(DEBUG && !ok){ qDebug() << " - Dir with missing path"; }
        break;
    default:
        ok=false;
        //if(DEBUG){ qDebug() << " - Unknown file type"; }
    }
    if(!showAll) {
        QString cdesk = getenv("XDG_CURRENT_DESKTOP");
        if(cdesk.isEmpty()) {
            cdesk="7b7b";
        }
        if(!showInList.isEmpty()) {
            ok = showInList.contains(cdesk, Qt::CaseInsensitive);
        }
        else if(!notShowInList.isEmpty()) {
            ok = !notShowInList.contains(cdesk,Qt::CaseInsensitive);
        }
        else if(name.isEmpty()) {
            ok = false;
        }
    }
    return ok;
}

QString XDGDesktop::getDesktopExec(QString ActionID) {
    //Generate the executable line for the application
    QString out = exec;
    if( !ActionID.isEmpty() ) {
        //Go through and grab the proper exec for the listed action
        for(int i=0; i<actions.length(); i++) {
            if(actions[i].ID == ActionID) {
                out = actions[i].exec;
                break;
            }
        }
    }

    if(out.isEmpty()) {
        return "";
    }
    else if(useTerminal) {
        //Get the currently default terminal
        QString term = LXDG::findDefaultAppForMime("application/terminal");
        if(!QFile::exists(term)) {
            term = "xterm -lc";
        }
        else if(term.endsWith(".desktop")) {
            XDGDesktop DF(term);
            if(DF.isValid()) {
                term = DF.getDesktopExec();
            }
            else {
                term = "xterm -lc";
            }
            //DF.deleteLater(); //done with this struct
        } else if( !LUtils::isValidBinary(term)) {
            term = "xterm -lc";
        }
        out = term+" -e "+out;  //-e is a nearly-universal flag for terminal emulators
    }
    //Now perform any of the XDG flag substitutions as appropriate (9/2014 standards)
    if(out.contains("%i") && !icon.isEmpty() ) {
        out.replace("%i", "--icon \""+icon+"\"");
    }
    if(out.contains("%c")) {
        if(!name.isEmpty()) {
            out.replace("%c", "\""+name+"\"");
        }
        else if(!genericName.isEmpty()) {
            out.replace("%c", "\""+genericName+"\"");
        }
        else {
            out.replace("%c", "\""+filePath.section("/",-1).section(".desktop",0,0)+"\"");
        }
    }
    if(out.contains("%k")) {
        out.replace("%k", "\""+filePath+"\"");
    }
    return out;
}

QString XDGDesktop::generateExec(QStringList inputfiles, QString ActionID) {
    QString dExec = getDesktopExec(ActionID);
    //Does the app need the input files in URL or File syntax?
    bool URLsyntax = (dExec.contains("%u") || dExec.contains("%U"));
    //Adjust the input file formats as needed
    //qDebug() << "Got inputfiles:" << inputfiles << URLsyntax;
    for(int i=0; i<inputfiles.length(); i++) {
        bool isURL = inputfiles[i].startsWith("www") || inputfiles[i].contains("://");
        //Run it through the QUrl class to catch/fix any URL syntax issues
        if(URLsyntax) {
            if(inputfiles[i].startsWith("mailto:") ) {} //don't touch this syntax - already formatted
            else if(isURL) {
                inputfiles[i] = QUrl(inputfiles[i]).url();
            }
            else {
                inputfiles[i] = QUrl::fromLocalFile(inputfiles[i]).url();
            }
        } else {
            if(isURL) {
                inputfiles[i] = QUrl(inputfiles[i]).toLocalFile();
            }
        }
    }
    inputfiles.removeAll(""); //just in case any empty ones get through
    //Now to the exec replacements as needed
    //qDebug() << "Generate Exec:" << exec << inputfiles;
    if(dExec.contains("%f")) {
        if(inputfiles.isEmpty()) {
            dExec.replace("%f","");
        }
        else {
            dExec.replace("%f", "\""+inputfiles.first()+"\"");    //Note: can only take one input
        }
    } else if(dExec.contains("%F")) {
        if(inputfiles.isEmpty()) {
            dExec.replace("%F","");
        }
        else {
            dExec.replace("%F", "\""+inputfiles.join("\" \"")+"\"");
        }
    }
    if(dExec.contains("%u")) {
        if(inputfiles.isEmpty()) {
            dExec.replace("%u","");
        }
        else {
            dExec.replace("%u",  "\""+inputfiles.first()+"\"");    //Note: can only take one input
        }
    } else if(dExec.contains("%U")) {
        if(inputfiles.isEmpty()) {
            dExec.replace("%U","");
        }
        else {
            dExec.replace("%U", "\""+inputfiles.join("\" \"")+"\"");
        }
    }
    //Sanity check for known Local/URL syntax issues from some apps
    if(!URLsyntax && dExec.contains("%20")) {
        dExec.replace("%20"," ");
    }
    //Clean up any leftover "Exec" field codes (should have already been replaced earlier)
    if(dExec.contains("%")) {
        dExec = dExec.remove("%U").remove("%u").remove("%F").remove("%f").remove("%i").remove("%c").remove("%k");
    }
    return dExec.simplified();
}

bool XDGDesktop::saveDesktopFile(bool merge) {
    qDebug() << "Save Desktop File:" << filePath << "Merge:" << merge;
    bool autofile = filePath.contains("/autostart/"); //use the "Hidden" field instead of the "NoDisplay"
    int insertloc = -1;
    QStringList info;
    if(QFile::exists(filePath) && merge) {
        //Load the existing file and merge in in any changes
        info = LUtils::readFile(filePath);
        //set a couple flags based on the contents before we start iterating through
        // - determine if a translated field was changed (need to remove all the now-invalid translations)
        bool clearName, clearComment, clearGName;
        QString tmp = "";
        if(!info.filter("Name=").isEmpty()) {
            tmp = info.filter("Name=").first().section("=",1,50);
        }
        clearName=(tmp!=name);
        tmp.clear();
        if(!info.filter("Comment=").isEmpty()) {
            tmp = info.filter("Comment=").first().section("=",1,50);
        }
        clearComment=(tmp!=comment);
        tmp.clear();
        if(!info.filter("GenericName=").isEmpty()) {
            tmp = info.filter("GenericName=").first().section("=",1,50);
        }
        clearGName=(tmp!=genericName);
        //Now start iterating through the file and changing fields as necessary
        bool insection = false;
        for(int i=0; i<info.length(); i++) {
            if(info[i]=="[Desktop Entry]") {
                insection = true;
                continue;
            } else if(info[i].startsWith("[")) {
                if(insection) {
                    insertloc = i;    //save this location for later insertions
                }
                insection = false;
                continue;
            }
            if(!insection || info[i].isEmpty() || info[i].section("#",0,0).simplified().isEmpty()) {
                continue;
            }
            QString var = info[i].section("=",0,0);
            QString val = info[i].section("=",1,50).simplified();
            //NOTE: Clear the dFile variable as it is found/set in the file (to keep track of what has been used already)
            //    For boolian values, set them to false
            // --LOCALIZED VALUES --
            if(var.startsWith("Name")) {
                if(var.contains("[") && clearName) {
                    info.removeAt(i);
                    i--;
                    continue;
                }
                else if(!var.contains("[")) {
                    info[i] = var+"="+name;
                    name.clear();
                }
            } else if(var.startsWith("GenericName")) {
                if(var.contains("[") && clearGName) {
                    info.removeAt(i);
                    i--;
                    continue;
                }
                else if(!var.contains("[")) {
                    info[i] = var+"="+genericName;
                    genericName.clear();
                }
            } else if(var.startsWith("Comment")) {
                if(var.contains("[") && clearComment) {
                    info.removeAt(i);
                    i--;
                    continue;
                }
                else if(!var.contains("[")) {
                    info[i] = var+"="+comment;
                    comment.clear();
                }

                // --STRING/LIST VALUES--
            } else if(var=="Exec") {
                info[i] = var+"="+exec;
                exec.clear();
            }
            else if(var=="TryExec") {
                info[i] = var+"="+tryexec;
                tryexec.clear();
            }
            else if(var=="Path") {
                info[i] = var+"="+path;
                path.clear();
            }
            else if(var=="Icon") {
                info[i] = var+"="+icon;
                icon.clear();
            }
            else if(var=="StartupWMClass") {
                info[i] = var+"="+startupWM;
                startupWM.clear();
            }
            else if(var=="MimeType") {
                info[i] = var+"="+mimeList.join(";");
                mimeList.clear();
            }
            else if(var=="Categories") {
                info[i] = var+"="+catList.join(";");
                catList.clear();
            }
            else if(var=="Keywords") {
                info[i] = var+"="+keyList.join(";");
                keyList.clear();
            }
            else if(var=="Actions") {
                info[i] = var+"="+actionList.join(";");
                actionList.clear();
            }
            else if(var=="OnlyShowIn") {
                info[i] = var+"="+showInList.join(";");
                showInList.clear();
            }
            else if(var=="NotShowIn") {
                info[i] = var+"="+notShowInList.join(";");
                notShowInList.clear();
            }
            else if(var=="URL") {
                info[i] = var+"="+url;
                url.clear();
            }

            // --BOOLIAN VALUES--
            else if(var=="Hidden") {
                if(!autofile) {
                    info.removeAt(i);
                    i--;
                    continue;
                }
                else {
                    info[i] = var+"="+(isHidden ? "true": "false");
                    isHidden=false;
                }
            } else if(var=="NoDisplay") {
                if(autofile) {
                    info.removeAt(i);
                    i--;
                    continue;
                }
                else {
                    info[i] = var+"="+(isHidden ? "true": "false");
                    isHidden=false;
                }
            } else if(var=="Terminal") {
                info[i] = var+"="+(useTerminal ? "true": "false");
                useTerminal=false;
            } else if(var=="StartupNotify") {
                info[i] = var+"="+(startupNotify ? "true": "false");
                startupNotify=false;
            }
            // Remove any lines that have been un-set or removed from the file
            if(info[i].section("=",1,50).simplified().isEmpty()) {
                info.removeAt(i);
                i--;
            }
        }

    } else {
        //Just write a new file and overwrite any old one
        // (pre-set some values here which are always required)
        info << "[Desktop Entry]";
        info << "Version=1.0";
        if(type==XDGDesktop::APP) {
            info << "Type=Application";
        }
        else if(type==XDGDesktop::LINK) {
            info << "Type=Link";
        }
        else if(type==XDGDesktop::DIR) {
            info << "Type=Dir";
        }
    }

    if(insertloc<0) {
        insertloc = info.size();    //put it at the end
    }
    //Now add in any items that did not exist in the original file
    if( !exec.isEmpty() ) {
        info.insert(insertloc,"Exec="+exec);
    }
    if( !tryexec.isEmpty() ) {
        info.insert(insertloc,"TryExec="+tryexec);
    }
    if( !path.isEmpty() ) {
        info.insert(insertloc,"Path="+path);
    }
    if( !icon.isEmpty() ) {
        info.insert(insertloc,"Icon="+icon);
    }
    if( !name.isEmpty() ) {
        info.insert(insertloc,"Name="+name);
    }
    if( !genericName.isEmpty() ) {
        info.insert(insertloc,"GenericName="+genericName);
    }
    if( !comment.isEmpty() ) {
        info.insert(insertloc,"Comment="+comment);
    }
    if( !startupWM.isEmpty() ) {
        info.insert(insertloc,"StartupWMClass="+startupWM);
    }
    if( !mimeList.isEmpty() ) {
        info.insert(insertloc,"MimeType="+mimeList.join(";"));
    }
    if( !catList.isEmpty() ) {
        info.insert(insertloc,"Categories="+catList.join(";"));
    }
    if( !keyList.isEmpty() ) {
        info.insert(insertloc,"Keywords="+keyList.join(";"));
    }
    if( !actionList.isEmpty() ) {
        info.insert(insertloc,"Actions="+actionList.join(";"));
    }
    if( !showInList.isEmpty() ) {
        info.insert(insertloc,"OnlyShowIn="+showInList.join(";"));
    }
    else if( !notShowInList.isEmpty() ) {
        info.insert(insertloc,"NotShowIn="+notShowInList.join(";"));
    }
    if( !url.isEmpty() ) {
        info.insert(insertloc,"URL="+url);
    }
    if( isHidden && autofile ) {
        info.insert(insertloc,"Hidden=true");
    }
    else if(isHidden) {
        info.insert(insertloc,"NoDisplay=true");
    }
    if( useTerminal) {
        info.insert(insertloc,"Terminal=true");
    }
    if( startupNotify ) {
        info.insert(insertloc,"StartupNotify=true");
    }

    //Now save the file
    return LUtils::writeFile(filePath, info, true);
}

bool XDGDesktop::setAutoStarted(bool autostart) {
    //First get the list of system directories to search (system first, user-provided files come later and overwrite sys files as needed)
    QStringList paths = QString(getenv("XDG_CONFIG_DIRS")).split(":");
    QString upath = QString(getenv("XDG_CONFIG_HOME")).section(":",0,0);
    if(upath.isEmpty()) {
        upath = QDir::homePath()+"/.config/autostart/";
    }
    else {
        upath.append("/autostart/");
    }
    //Verify that the autostart directory exists
    if(!QFile::exists(upath)) {
        QDir dir;
        dir.mkpath(upath);
    }

    //Quick check/finish for user-defined files which are getting disabled (just remove the file)
    if(filePath.startsWith(upath) && !autostart) {
        return QFile::remove(filePath);
    }
    bool sysfile = false;
    for(int i=0; i<paths.length(); i++) {
        if(filePath.startsWith(paths[i]+"/autostart/") ) {
            sysfile = true;
            //Change it to the user-modifiable directory
            filePath = filePath.replace(paths[i]+"/autostart/", upath);
        }
    }
    //Make sure the user-autostart dir is specified, and clean the app structure as necessary
    if( !filePath.startsWith(upath) && autostart) {
        //Some other non-override autostart file - set it up to open with 7b7b-open
        if(!filePath.endsWith(".desktop")) {
            exec = "7b7b-open "+filePath;
            tryexec = filePath; //make sure this file exists
            if(name.isEmpty()) {
                name = filePath.section("/",-1);
            }
            if(icon.isEmpty()) {
                icon = LXDG::findAppMimeForFile(filePath);
                icon.replace("/","-");
            }
            filePath = upath+filePath.section("/",-1)+".desktop";
            type = XDGDesktop::APP;
        } else {
            //Some other *.desktop file on the system (keep almost all the existing settings/values)
            // - setup a redirect to the other file
            exec = "7b7b-open "+filePath;
            tryexec = filePath; //make sure this file exists
            // - Adjust the actual path where this file will get saved
            filePath = upath+filePath.section("/",-1);
        }
    }
    //Now save the "hidden" value into the file
    isHidden = !autostart; //if hidden, it will not be autostarted
    //Now save the file as necessary
    bool saved = false;
    //qDebug() << " - Saving AutoStart File:" << filePath << name << isHidden;
    if(sysfile) {
        //Just an override file for the "hidden" field - nothing more
        QStringList info;
        info << "[Desktop Entry]" << "Type=Application" << QString("Hidden=")+ (isHidden ? QString("true"): QString("false"));
        saved = LUtils::writeFile(filePath, info, true);
    } else {
        //Need to actually save the full file
        saved = saveDesktopFile();
    }
    return saved;
}

void XDGDesktop::addToMenu(QMenu *topmenu) {
    if(!this->isValid()) {
        return;
    }
    if(actions.isEmpty()) {
        //Just a single entry point - no extra actions
        QAction *act = new QAction(this->name, topmenu);
        act->setIcon(LXDG::findIcon(this->icon, ""));
        act->setToolTip(this->comment);
        act->setWhatsThis(this->filePath);
        topmenu->addAction(act);
    } else {
        //This app has additional actions - make this a sub menu
        // - first the main menu/action
        QMenu *submenu = new QMenu(this->name, topmenu);
        submenu->setIcon( LXDG::findIcon(this->icon,"") );
        //This is the normal behavior - not a special sub-action (although it needs to be at the top of the new menu)
        QAction *act = new QAction(this->name, submenu);
        act->setIcon(LXDG::findIcon(this->icon, ""));
        act->setToolTip(this->comment);
        act->setWhatsThis(this->filePath);
        submenu->addAction(act);
        //Now add entries for every sub-action listed
        for(int sa=0; sa<this->actions.length(); sa++) {
            QAction *sact = new QAction( this->actions[sa].name, this);
            sact->setIcon(LXDG::findIcon( this->actions[sa].icon, this->icon));
            sact->setToolTip(this->comment);
            sact->setWhatsThis("-action \""+this->actions[sa].ID+"\" \""+this->filePath+"\"");
            submenu->addAction(sact);
        }
        topmenu->addMenu(submenu);
    }
}


//====XDGDesktopList Functions ====
XDGDesktopList::XDGDesktopList(QObject *parent, bool watchdirs) : QObject(parent) {
    synctimer = new QTimer(this); //interval set automatically based on changes/interactions
    connect(synctimer, SIGNAL(timeout()), this, SLOT(updateList()) );
    keepsynced = watchdirs;
    if(watchdirs) {
        watcher = new QFileSystemWatcher(this);
        connect(watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(watcherChanged()) );
        connect(watcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(watcherChanged()) );
    } else {
        watcher = 0;
    }
}

XDGDesktopList::~XDGDesktopList() {
    //nothing special to do here
}

XDGDesktopList* XDGDesktopList::instance() {
    static XDGDesktopList *APPLIST = 0;
    if(APPLIST==0) {
        APPLIST = new XDGDesktopList(0, true);
    }
    return APPLIST;
}

void XDGDesktopList::watcherChanged() {
    if(synctimer->isActive()) {
        synctimer->stop();
    }
    synctimer->setInterval(1000); //1 second delay before check kicks off
    synctimer->start();
}

void XDGDesktopList::updateList() {
    //run the check routine
    if(synctimer->isActive()) {
        synctimer->stop();
    }
    hashmutex.lock();
    QStringList appDirs = LXDG::systemApplicationDirs(); //get all system directories
    QStringList found, newfiles; //for avoiding duplicate apps (might be files with same name in different priority directories)
    QStringList oldkeys = files.keys();
    bool appschanged = false;
    bool firstrun = lastCheck.isNull() || oldkeys.isEmpty();
    lastCheck = QDateTime::currentDateTime();
    //Variables for internal loop use only (to prevent re-initializing variable on every iteration)
    QString path;
    QDir dir;
    QStringList appslist;
    for(int i=0; i<appDirs.length(); i++) {
        if( !dir.cd(appDirs[i]) ) {
            continue;    //could not open dir for some reason
        }
        appslist = dir.entryList(QStringList() << "*.desktop",QDir::Files, QDir::Name);
        for(int a=0; a<appslist.length(); a++) {
            path = dir.absoluteFilePath(appslist[a]);
            if(files.contains(path) && (files.value(path)->lastRead>QFileInfo(path).lastModified()) ) {
                //Re-use previous data for this file (nothing changed)
                found << files[path]->name;  //keep track of which files were already found
            } else {
                if(files.contains(path)) {
                    appschanged = true;
                    files.take(path)->deleteLater();
                }
                XDGDesktop *dFile = new XDGDesktop(path, this);
                if(dFile->type!=XDGDesktop::BAD) {
                    appschanged = true; //flag that something changed - needed to load a file
                    if(!oldkeys.contains(path)) {
                        newfiles << path;    //brand new file (not an update to a previously-read file)
                    }
                    files.insert(path, dFile);
                    found << dFile->name;
                } else {
                    dFile->deleteLater(); //bad file - discard it
                }
            }
            oldkeys.removeAll(path); //make sure this key does not get cleaned up later
        } //end loop over apps
    } //end loop over appDirs
    //Save the extra info to the internal lists
    if(!firstrun) {
        removedApps = oldkeys;//files which were removed
        newApps = newfiles; //files which were added
    }
    //Now go through and cleanup any old keys where the associated file does not exist anymore
    for(int i=0; i<oldkeys.length(); i++) {
        //qDebug() << "Removing file from internal map:" << oldkeys[i];
        if(i==0) {
            appschanged = true;
        }
        //files.remove(oldkeys[i]);
        files.take(oldkeys[i])->deleteLater();
    }
    //If this class is automatically managing the lists, update the watched files/dirs and send out notifications
    if(watcher!=0) {
        if(appschanged) {
            qDebug() << "Auto App List Update:" << lastCheck  << "Files Found:" << files.count();
        }
        watcher->removePaths(QStringList() << watcher->files() << watcher->directories());
        watcher->addPaths(appDirs);
        if(appschanged) {
            emit appsUpdated();
        }
        synctimer->setInterval(60000); //Update in 1 minute if nothing changes before then
        synctimer->start();
    }
    hashmutex.unlock();
}

QList<XDGDesktop*> XDGDesktopList::apps(bool showAll, bool showHidden) {
    //showAll: include invalid files, showHidden: include NoShow/Hidden files
    //hashmutex.lock();
    QStringList keys = files.keys();
    QList<XDGDesktop*> out;
    for(int i=0; i<keys.length(); i++) {
        if( showHidden || !files[keys[i]]->isHidden ) { //this is faster than the "checkValidity()" function below  - so always filter here first
            if( files[keys[i]]->isValid(showAll) ) {
                out << files[keys[i]];
            }
        }
    }
    //hashmutex.unlock();
    return out;
}

XDGDesktop* XDGDesktopList::findAppFile(QString filename) {
    //hashmutex.lock();
    QStringList keys = files.keys().filter(filename);
    QString chk = filename.section("/",-1);
    XDGDesktop *found = 0;
    for(int i=0; i<keys.length(); i++) {
        if(keys[i] == filename || keys[i].endsWith("/"+chk)) {
            found = files[keys[i]];
        }
    }
    //hashmutex.unlock();
    //No matches
    return found;
}

void XDGDesktopList::populateMenu(QMenu *topmenu, bool byCategory) {
    topmenu->clear();
    if(byCategory) {
        QHash<QString, QList<XDGDesktop*> > APPS = LXDG::sortDesktopCats( this->apps(false,false) );
        QStringList cats = APPS.keys();
        cats.sort(); //make sure they are alphabetical
        for(int i=0; i<cats.length(); i++) {
            //Make sure they are translated and have the right icons
            QString name, icon;
            if(cats[i]=="All") {
                continue;    //skip this listing for the menu
            }
            else if(cats[i] == "Multimedia") {
                name = tr("Multimedia");
                icon = "applications-multimedia";
            }
            else if(cats[i] == "Development") {
                name = tr("Development");
                icon = "applications-development";
            }
            else if(cats[i] == "Education") {
                name = tr("Education");
                icon = "applications-education";
            }
            else if(cats[i] == "Game") {
                name = tr("Games");
                icon = "applications-games";
            }
            else if(cats[i] == "Graphics") {
                name = tr("Graphics");
                icon = "applications-graphics";
            }
            else if(cats[i] == "Network") {
                name = tr("Network");
                icon = "applications-internet";
            }
            else if(cats[i] == "Office") {
                name = tr("Office");
                icon = "applications-office";
            }
            else if(cats[i] == "Science") {
                name = tr("Science");
                icon = "applications-science";
            }
            else if(cats[i] == "Settings") {
                name = tr("Settings");
                icon = "preferences-system";
            }
            else if(cats[i] == "System") {
                name = tr("System");
                icon = "applications-system";
            }
            else if(cats[i] == "Utility") {
                name = tr("Utility");
                icon = "applications-utilities";
            }
            else if(cats[i] == "Wine") {
                name = tr("Wine");
                icon = "wine";
            }
            else {
                name = tr("Unsorted");
                icon = "applications-other";
            }

            QMenu *menu = new QMenu(name, topmenu);
            menu->setIcon(LXDG::findIcon(icon,""));
            QList<XDGDesktop*> appL = APPS.value(cats[i]);
            for( int a=0; a<appL.length(); a++) {
                appL[a]->addToMenu(menu);
            }
            topmenu->addMenu(menu);
        } //end loop over cats
    } else {
        QList<XDGDesktop*> APPS = this->apps(false, false);
        for(int i=0; i<APPS.length(); i++) {
            APPS[i]->addToMenu(topmenu);
        }
    }
}

//==== LXDG Functions ====
bool LXDG::checkExec(QString exec) {
    //Return true(good) or false(bad)
    //Check for quotes around the exec, and remove them as needed
    if(exec.startsWith("\"") && exec.count("\"")>=2) {
        exec = exec.section("\"",1,1).simplified();
    }
    if(exec.startsWith("\'") && exec.count("\'")>=2) {
        exec = exec.section("\'",1,1).simplified();
    }
    if(exec.startsWith("/")) {
        return QFile::exists(exec);
    }
    else {
        QStringList paths = QString(getenv("PATH")).split(":");
        for(int i=0; i<paths.length(); i++) {
            if(QFile::exists(paths[i]+"/"+exec)) {
                return true;
            }
        }
    }
    return false; //could not find the executable in the current path(s)
}

QStringList LXDG::systemApplicationDirs() {
    //Returns a list of all the directories where *.desktop files can be found
    QStringList appDirs = QString(getenv("XDG_DATA_HOME")).split(":");
    appDirs << QString(getenv("XDG_DATA_DIRS")).split(":");
    if(appDirs.isEmpty()) {
        appDirs << "/usr/local/share" << "/usr/share" << LOS::AppPrefix()+"/share" << LOS::SysPrefix()+"/share" << L_SHAREDIR;
    }
    appDirs.removeDuplicates();
    //Now create a valid list
    QStringList out;
    for(int i=0; i<appDirs.length(); i++) {
        if( QFile::exists(appDirs[i]+"/applications") ) {
            out << appDirs[i]+"/applications";
            //Also check any subdirs within this directory
            // (looking at you KDE - stick to the standards!!)
            out << LUtils::listSubDirectories(appDirs[i]+"/applications");
        }
    }
    //qDebug() << "System Application Dirs:" << out;
    return out;
}

QHash<QString,QList<XDGDesktop*> > LXDG::sortDesktopCats(QList<XDGDesktop*> apps) {
    //Sort the list of applications into their different categories (main categories only)
    //Create the category lists
    QList<XDGDesktop*> multimedia, dev, ed, game, graphics, network, office, science, settings, sys, utility, other, wine;
    //Sort the apps into the lists
    for(int i=0; i<apps.length(); i++) {
        if(apps[i]->catList.contains("AudioVideo")) {
            multimedia << apps[i];
        }
        else if(apps[i]->catList.contains("Development")) {
            dev << apps[i];
        }
        else if(apps[i]->catList.contains("Education")) {
            ed << apps[i];
        }
        else if(apps[i]->catList.contains("Game")) {
            game << apps[i];
        }
        else if(apps[i]->catList.contains("Graphics")) {
            graphics << apps[i];
        }
        else if(apps[i]->catList.contains("Network")) {
            network << apps[i];
        }
        else if(apps[i]->catList.contains("Office")) {
            office << apps[i];
        }
        else if(apps[i]->catList.contains("Science")) {
            science << apps[i];
        }
        else if(apps[i]->catList.contains("Settings")) {
            settings << apps[i];
        }
        else if(apps[i]->catList.contains("System")) {
            sys << apps[i];
        }
        else if(apps[i]->catList.contains("Utility")) {
            utility << apps[i];
        }
        else if(apps[i]->catList.contains("Wine")) {
            wine << apps[i];
        }
        else {
            other << apps[i];
        }
    }
    //Now create the output hash
    QHash<QString,QList<XDGDesktop*> > out;
    if(!multimedia.isEmpty()) {
        out.insert("Multimedia", LXDG::sortDesktopNames(multimedia));
    }
    if(!dev.isEmpty()) {
        out.insert("Development", LXDG::sortDesktopNames(dev));
    }
    if(!ed.isEmpty()) {
        out.insert("Education", LXDG::sortDesktopNames(ed));
    }
    if(!game.isEmpty()) {
        out.insert("Game", LXDG::sortDesktopNames(game));
    }
    if(!graphics.isEmpty()) {
        out.insert("Graphics", LXDG::sortDesktopNames(graphics));
    }
    if(!network.isEmpty()) {
        out.insert("Network", LXDG::sortDesktopNames(network));
    }
    if(!office.isEmpty()) {
        out.insert("Office", LXDG::sortDesktopNames(office));
    }
    if(!science.isEmpty()) {
        out.insert("Science", LXDG::sortDesktopNames(science));
    }
    if(!settings.isEmpty()) {
        out.insert("Settings", LXDG::sortDesktopNames(settings));
    }
    if(!sys.isEmpty()) {
        out.insert("System", LXDG::sortDesktopNames(sys));
    }
    if(!utility.isEmpty()) {
        out.insert("Utility", LXDG::sortDesktopNames(utility));
    }
    if(!wine.isEmpty()) {
        out.insert("Wine", LXDG::sortDesktopNames(wine));
    }
    if(!other.isEmpty()) {
        out.insert("Unsorted", LXDG::sortDesktopNames(other));
    }
    //return the resulting hash
    return out;
}

//Return the icon to use for the given category
QString LXDG::DesktopCatToIcon(QString cat) {
    QString icon = "applications-other";
    if(cat=="Multimedia") {
        icon = "applications-multimedia";
    }
    else if(cat=="Development") {
        icon = "applications-development";
    }
    else if(cat=="Education") {
        icon = "applications-education";
    }
    else if(cat=="Game") {
        icon = "applications-games";
    }
    else if(cat=="Graphics") {
        icon = "applications-graphics";
    }
    else if(cat=="Network") {
        icon = "applications-internet";
    }
    else if(cat=="Office") {
        icon = "applications-office";
    }
    else if(cat=="Science") {
        icon = "applications-science";
    }
    else if(cat=="Settings") {
        icon = "preferences-system";
    }
    else if(cat=="System") {
        icon = "applications-system";
    }
    else if(cat=="Utility") {
        icon = "applications-utilities";
    }
    else if(cat=="Wine") {
        icon = "wine";
    }
    return icon;
}

QList<XDGDesktop*> LXDG::sortDesktopNames(QList<XDGDesktop*> apps) {
    //Sort the list by name of the application
    QHash<QString, XDGDesktop*> sorter;
    for(int i=0; i<apps.length(); i++) {
        sorter.insert(apps[i]->name.toLower(), apps[i]);
    }
    QStringList keys = sorter.keys();
    keys.sort();
    //Re-assemble the output list
    QList<XDGDesktop*> out;
    for(int i=0; i<keys.length(); i++) {
        out << sorter[keys[i]];
    }
    return out;
}

void LXDG::setEnvironmentVars() {
    //Set the default XDG environment variables if not already set
    setenv("XDG_DATA_HOME",QString(QDir::homePath()+"/.local/share").toUtf8(), 0);
    setenv("XDG_CONFIG_HOME",QString(QDir::homePath()+"/.config").toUtf8(), 0);
    setenv("XDG_DATA_DIRS","/usr/local/share:/usr/share", 0);
    setenv("XDG_CONFIG_DIRS","/etc/xdg:/usr/local/etc/xdg", 0);
    setenv("XDG_CACHE_HOME",QString(QDir::homePath()+"/.cache").toUtf8(), 0);
    //Don't set "XDG_RUNTIME_DIR" yet - need to look into the specs
}

QIcon LXDG::findIcon(QString iconName, QString fallback) {

	QIcon ico;
	ico = QIcon::fromTheme(iconName);

	if(ico.isNull() && !fallback.isEmpty()) {
		ico = LXDG::findIcon(fallback,"");
	}
	return ico;

}

QStringList LXDG::getChildIconDirs(QString parent) {
    //This is a recursive function that returns the absolute path(s) of directories with *.png files
    QDir D(parent);
    QStringList out;
    QStringList dirs = D.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    if(!dirs.isEmpty() && (dirs.contains("32x32") || dirs.contains("scalable")) ) {
        //Need to sort these directories by image size
        //qDebug() << " - Parent:" << parent << "Dirs:" << dirs;
        for(int i=0; i<dirs.length(); i++) {
            if(dirs[i].contains("x")) {
                dirs[i].prepend( QString::number(10-dirs[i].section("x",0,0).length())+QString::number(10-dirs[i].at(0).digitValue())+"::::");
            }
            else if(dirs[i].at(0).isNumber()) {
                dirs[i].prepend( QString::number(10-dirs[i].length())+QString::number(10-dirs[i].at(0).digitValue())+"::::");
            }
            else {
                dirs[i].prepend( "0::::");
            }
        }
        dirs.sort();
        for(int i=0; i<dirs.length(); i++) {
            dirs[i] = dirs[i].section("::::",1,50);    //chop the sorter off the front again
        }
        //qDebug() << "Sorted:" << dirs;
    }
    QStringList img = D.entryList(QStringList() << "*.png" << "*.svg", QDir::Files | QDir::NoDotAndDotDot, QDir::NoSort);
    if(img.length() > 0) {
        out << D.absolutePath();
    }
    for(int i=0; i<dirs.length(); i++) {
        img.clear();
        img = getChildIconDirs(D.absoluteFilePath(dirs[i])); //re-use the old list variable
        if(img.length() > 0) {
            out << img;
        }
    }
    return out;
}

QStringList LXDG::getIconThemeDepChain(QString theme, QStringList paths) {
    QStringList results;
    for(int i=0; i<paths.length(); i++) {
        if( QFile::exists(paths[i]+theme+"/index.theme") ) {
            QStringList deps = LUtils::readFile(paths[i]+theme+"/index.theme").filter("Inherits=");
            if(!deps.isEmpty()) {
                deps = deps.first().section("=",1,-1).split(";",Qt::SkipEmptyParts);
                for(int j=0; j<deps.length(); j++) {
                    results << deps[j] << getIconThemeDepChain(deps[j],paths);
                }
            }
            break; //found primary theme index file - stop here
        }
    }
    return results;;
}

QStringList LXDG::systemMimeDirs() {
    //Returns a list of all the directories where *.xml MIME files can be found
    QStringList appDirs = QString(getenv("XDG_DATA_HOME")).split(":");
    appDirs << QString(getenv("XDG_DATA_DIRS")).split(":");
    if(appDirs.isEmpty()) {
        appDirs << "/usr/local/share" << "/usr/share";
    }
    //Now create a valid list
    QStringList out;
    for(int i=0; i<appDirs.length(); i++) {
        if( QFile::exists(appDirs[i]+"/mime") ) {
            out << appDirs[i]+"/mime";
        }
    }
    return out;
}

QIcon LXDG::findMimeIcon(QString extension) {
    QIcon ico;
    QString mime = LXDG::findAppMimeForFile(extension);
    if(mime.isEmpty()) {
        mime = LXDG::findAppMimeForFile(extension.toLower());
    }
    mime.replace("/","-"); //translate to icon mime name
    if(!mime.isEmpty()) {
        ico = LXDG::findIcon(mime, "unknown");   //use the "unknown" mimetype icon as fallback
    }
    if(ico.isNull()) {
        ico = LXDG::findIcon("unknown","");    //just in case
    }
    return ico;
}

QString LXDG::findAppMimeForFile(QString filename, bool multiple) {
    QString out;
    QString extension = filename.section(".",1,-1);
    if("."+extension == filename) {
        extension.clear();    //hidden file without extension
    }
    //qDebug() << "MIME SEARCH:" << filename << extension;
    QStringList mimefull = LXDG::loadMimeFileGlobs2();
    QStringList mimes;
    //Just in case the filename is a mimetype itself
    if( mimefull.filter(":"+filename+":").length() == 1) {
        return filename;
    }
    while(mimes.isEmpty()) {
        //Check for an exact mimetype match
        if(mimefull.filter(":"+extension+":").length() == 1) {
            return extension;
        }
        //Look for globs at the end of the filename
        if(!extension.isEmpty()) {
            mimes = mimefull.filter(":*."+extension);
            //If nothing found, try a case-insensitive search
            if(mimes.isEmpty()) {
                mimes = mimefull.filter(":*."+extension, Qt::CaseInsensitive);
            }
            //Now ensure that the filter was accurate (*.<extention>.<something> will still be caught)
            for(int i=0; i<mimes.length(); i++) {
                if(!filename.endsWith( mimes[i].section(":*",-1), Qt::CaseInsensitive )) {
                    mimes.removeAt(i);
                    i--;
                }
                else if(mimes[i].section(":",0,0).length()==2) {
                    mimes[i].prepend("0");    //ensure 3-character priority number
                }
                else if(mimes[i].section(":",0,0).length()==1) {
                    mimes[i].prepend("00");    //ensure 3-character priority number
                }
            }
        }
        //Look for globs at the start of the filename
        if(mimes.isEmpty()) {
            mimes = mimefull.filter(":"+filename.left(2)); //look for the first 2 characters initially
            //Note: This initial filter will only work if the wildcard (*) is not within the first 2 characters of the pattern
            //Now ensure that the filter was accurate
            for(int i=0; i<mimes.length(); i++) {
                if(!filename.startsWith( mimes[i].section(":",3,-1,QString::SectionSkipEmpty).section("*",0,0), Qt::CaseInsensitive )) {
                    mimes.removeAt(i);
                    i--;
                }
            }
        }
        if(mimes.isEmpty()) {
            if(extension.contains(".")) {
                extension = extension.section(".",1,-1);
            }
            else {
                break;
            }
        }
    } //end of mimes while loop
    mimes.sort(); //this automatically puts them in reverse weight order (100 on down)
    QStringList matches;
    //qDebug() << "Mimes:" << mimes;
    for(int m=mimes.length()-1; m>=0; m--) {
        QString mime = mimes[m].section(":",1,1,QString::SectionSkipEmpty);
        matches << mime;
    }
    //qDebug() << "Matches:" << matches;
    if(multiple && !matches.isEmpty() ) {
        out = matches.join("::::");
    }
    else if( !matches.isEmpty() ) {
        out = matches.first();
    }
    else { //no mimetype found - assign one (internal only - no system database changes)
        if(extension.isEmpty()) {
            out = "unknown/"+filename.toLower();
        }
        else {
            out = "unknown/"+extension.toLower();
        }
    }
    //qDebug() << "Out:" << out;
    return out;
}

QStringList LXDG::findFilesForMime(QString mime) {
    QStringList out;
    QStringList mimes = LXDG::loadMimeFileGlobs2().filter(mime);
    for(int i=0; i<mimes.length(); i++) {
        out << mimes[i].section(":",2,2); // "*.<extension>"
    }
    //qDebug() << "Mime to Files:" << mime << out;
    return out;
}

QStringList LXDG::listFileMimeDefaults() {
    //This will spit out a itemized list of all the mimetypes and relevant info
    // Output format: <mimetype>::::<extension>::::<default>::::<localized comment>
    QStringList mimes = LXDG::loadMimeFileGlobs2();
    //Remove all the application files from the list (only a single app defines/uses this type in general)
    /*QStringList apps = mimes.filter(":application/");
    //qDebug() << "List Mime Defaults";
    for(int i=0; i<apps.length(); i++){ mimes.removeAll(apps[i]); }*/
    //Now start filling the output list
    QStringList out;
    for(int i=0; i<mimes.length(); i++) {
        QString mimetype = mimes[i].section(":",1,1);
        QStringList tmp = mimes.filter(mimetype);
        //Collect all the different extensions with this mimetype
        QStringList extlist;
        for(int j=0; j<tmp.length(); j++) {
            mimes.removeAll(tmp[j]);
            extlist << tmp[j].section(":",2,2);
        }
        extlist.removeDuplicates(); //just in case
        //Now look for a current default for this mimetype
        QString dapp = LXDG::findDefaultAppForMime(mimetype); //default app;

        //Create the output entry
        //qDebug() << "Mime entry:" << i << mimetype << dapp;
        out << mimetype+"::::"+extlist.join(", ")+"::::"+dapp+"::::"+LXDG::findMimeComment(mimetype);

        i--; //go back one (continue until the list is empty)
    }
    return out;
}

QString LXDG::findMimeComment(QString mime) {
    QString comment;
    QStringList dirs = LXDG::systemMimeDirs();
    QString lang = QString(getenv("LANG")).section(".",0,0);
    QString shortlang = lang.section("_",0,0);
    for(int i=0; i<dirs.length(); i++) {
        if(QFile::exists(dirs[i]+"/"+mime+".xml")) {
            QStringList info = LUtils::readFile(dirs[i]+"/"+mime+".xml");
            QStringList filter = info.filter("<comment xml:lang=\""+lang+"\">");
            //First look for a full language match, then short language, then general comment
            if(filter.isEmpty()) {
                filter = info.filter("<comment xml:lang=\""+shortlang+"\">");
            }
            if(filter.isEmpty()) {
                filter = info.filter("<comment>");
            }
            if(!filter.isEmpty()) {
                comment = filter.first().section(">",1,1).section("</",0,0);
                break;
            }
        }
    }
    return comment;
}

QString LXDG::findDefaultAppForMime(QString mime) {
    //First get the priority-ordered list of default file locations
    QStringList dirs;
    dirs << QString(getenv("XDG_CONFIG_HOME"))+"/7b7b-mimeapps.list" \
         << QString(getenv("XDG_DATA_HOME"))+"/applications/7b7b-mimeapps.list" \
         << QString(getenv("XDG_CONFIG_HOME"))+"/mimeapps.list" \
         << QString(getenv("XDG_DATA_HOME"))+"/applications/mimeapps.list";
    QStringList tmp = QString(getenv("XDG_CONFIG_DIRS")).split(":");
    for(int i=0; i<tmp.length(); i++) {
        dirs << tmp[i]+"/7b7b-mimeapps.list";
        dirs << tmp[i]+"/mimeapps.list";
    }
    tmp = QString(getenv("XDG_DATA_DIRS")).split(":");
    for(int i=0; i<tmp.length(); i++) {
        dirs << tmp[i]+"/applications/7b7b-mimeapps.list";
        dirs << tmp[i]+"/applications/mimeapps.list";
    }

    //Now go through all the files in order of priority until a default is found
    QString cdefault;
    for(int i=0; i<dirs.length() && cdefault.isEmpty(); i++) {
        if(!QFile::exists(dirs[i])) {
            continue;
        }
        QStringList info = LUtils::readFile(dirs[i]);
        if(info.isEmpty()) {
            continue;
        }
        QStringList white; //lists to keep track of during the search (black unused at the moment)
        QString workdir = dirs[i].section("/",0,-2); //just the directory
        // qDebug() << "Check File:" << mime << dirs[i] << workdir;
        int def = info.indexOf("[Default Applications]"); //find this line to start on
        if(def>=0) {
            for(int d=def+1; d<info.length(); d++) {
                //qDebug() << "Check Line:" << info[d];
                if(info[d].startsWith("[")) {
                    break;    //starting a new section now - finished with defaults
                }
                if(info[d].contains(mime+"=") ) {
                    white = info[d].section("=",1,-1).split(";") + white; //exact mime match - put at front of list
                    break; //already found exact match
                } else if(info[d].contains("*") && info[d].contains("=") ) {
                    //QRegExp rg(info[d].section("=",0,0), Qt::CaseSensitive, QRegExp::WildcardUnix);
                    QRegularExpression rg(info[d].section("=",0,0));
                    //if(rg.exactMatch(mime)){

                    if(rg.match(mime).hasMatch()) {
                        white << info[d].section("=",1,-1).split(";"); //partial mime match - put at end of list
                    }
                }
            }
        }
        // Now check for any white-listed files in this work dir
        // find the full path to the file (should run even if nothing in this file)
        //qDebug() << "WhiteList:" << white;
        for(int w=0; w<white.length(); w++) {
            if(white[w].isEmpty()) {
                continue;
            }
            //First check for absolute paths to *.desktop file
            if( white[w].startsWith("/") ) {
                if( QFile::exists(white[w]) ) {
                    cdefault=white[w];
                    break;
                }
                else {
                    white.removeAt(w);    //invalid file path - remove it from the list
                    w--;
                }
            }
            //Now check for relative paths to  file (in current priority-ordered work dir)
            else if( QFile::exists(workdir+"/"+white[w]) ) {
                cdefault=workdir+"/"+white[w];
                break;
            }
            //Now go through the XDG DATA dirs and see if the file is in there
            else {
                white[w] = LUtils::AppToAbsolute(white[w]);
                if(QFile::exists(white[w])) {
                    cdefault = white[w];
                }
            }
        }
    } //End loop over files

    return cdefault;
}

QStringList LXDG::findAvailableAppsForMime(QString mime) {
    QStringList dirs = LXDG::systemApplicationDirs();
    QStringList out;
    //Loop over all possible directories that contain *.destop files
    //  and check for the mimeinfo.cache file
    for(int i=0; i<dirs.length(); i++) {
        if(QFile::exists(dirs[i]+"/mimeinfo.cache")) {
            QStringList matches = LUtils::readFile(dirs[i]+"/mimeinfo.cache").filter(mime+"=");
            //Find any matches for our mimetype in the cache
            for(int j=0; j<matches.length(); j++) {
                QStringList files = matches[j].section("=",1,1).split(";",Qt::SkipEmptyParts);
                //Verify that each file exists before putting the full path to the file in the output
                for(int m=0; m<files.length(); m++) {
                    if(QFile::exists(dirs[i]+"/"+files[m])) {
                        out << dirs[i]+"/"+files[m];
                    } else if(files[m].contains("-")) { //kde4-<filename> -> kde4/<filename> (stupid KDE variations!!)
                        files[m].replace("-","/");
                        if(QFile::exists(dirs[i]+"/"+files[m])) {
                            out << dirs[i]+"/"+files[m];
                        }
                    }
                }
            }
        }
    }
    //qDebug() << "Found Apps for Mime:" << mime << out << dirs;
    return out;
}

void LXDG::setDefaultAppForMime(QString mime, QString app) {
    //qDebug() << "Set Default App For Mime:" << mime << app;
    QString filepath = QString(getenv("XDG_CONFIG_HOME"))+"/7b7b-mimeapps.list";
    QStringList cinfo = LUtils::readFile(filepath);
    //If this is a new file, make sure to add the header appropriately
    if(cinfo.isEmpty()) {
        cinfo << "#Automatically generated with 7b7b-config" << "# DO NOT CHANGE MANUALLY" << "[Default Applications]";
    }
    //Check for any current entry for this mime type
    QStringList tmp = cinfo.filter(mime+"=");
    int index = -1;
    if(!tmp.isEmpty()) {
        index = cinfo.indexOf(tmp.first());
    }
    //Now add the new default entry (if necessary)
    if(app.isEmpty()) {
        if(index>=0) {
            cinfo.removeAt(index);    //Remove entry
        }
    } else {
        if(index<0) {
            cinfo << mime+"="+app+";"; //new entry
        } else {
            cinfo[index] = mime+"="+app+";"; //overwrite existing entry
        }
    }
    LUtils::writeFile(filepath, cinfo, true);
    return;
}

QStringList LXDG::findAVFileExtensions() {
    //output format: QDir name filter for valid A/V file extensions
    QStringList globs = LXDG::loadMimeFileGlobs2();
    QStringList av = globs.filter(":audio/");
    av << globs.filter(":video/");
    for(int i=0; i<av.length(); i++) {
        //Just use all audio/video mimetypes (for now)
        av[i] = av[i].section(":",2,2);
        //Qt5 Auto detection (broken - QMediaPlayer seg faults with Qt 5.3 - 11/24/14)
        /*if( QMultimedia::NotSupported != QMediaPlayer::hasSupport(av[i].section(":",1,1)) ){ av[i] = av[i].section(":",2,2); }
        else{ av.removeAt(i); i--; }*/
    }
    av.removeDuplicates();
    return av;
}

QStringList LXDG::loadMimeFileGlobs2() {
    //output format: <weight>:<mime type>:<file extension (*.something)>
    if(mimeglobs.isEmpty() || (mimechecktime < (QDateTime::currentMSecsSinceEpoch()-30000)) ) {
        //qDebug() << "Loading globs2 mime DB files";
        mimeglobs.clear();
        mimechecktime = QDateTime::currentMSecsSinceEpoch(); //save the current time this was last checked
        QStringList dirs = LXDG::systemMimeDirs();
        for(int i=0; i<dirs.length(); i++) {
            if(QFile::exists(dirs[i]+"/globs2")) {
                QFile file(dirs[i]+"/globs2");
                if(!file.exists()) {
                    continue;
                }
                if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    continue;
                }
                QTextStream in(&file);
                while(!in.atEnd()) {
                    QString line = in.readLine();
                    if(!line.startsWith("#")) {
                        mimeglobs << line.simplified();
                    }
                }
                file.close();
            }
            if(i==dirs.length()-1 && mimeglobs.isEmpty()) {
                //Could not find the mimetype database on the system - use the fallback file distributed with 7b7b
                dirs << LOS::LuminaShare();
            }
        }//end loop over dirs
    }
    return mimeglobs;
}

//Find all the autostart *.desktop files
QList<XDGDesktop*> LXDG::findAutoStartFiles(bool includeInvalid) {

    //First get the list of directories to search (system first, user-provided files come later and overwrite sys files as needed)
    QStringList paths = QString(getenv("XDG_CONFIG_DIRS")).split(":");
    paths << QString(getenv("XDG_CONFIG_HOME")).split(":");
    //Now go through them and find any valid *.desktop files
    QList<XDGDesktop*> files;
    QStringList filenames; //make it easy to see if this filename is an override
    QDir dir;
    for(int i=0; i<paths.length(); i++) {
        if(!QFile::exists(paths[i]+"/autostart")) {
            continue;
        }
        dir.cd(paths[i]+"/autostart");
        QStringList tmp = dir.entryList(QStringList() << "*.desktop", QDir::Files, QDir::Name);
        for(int t=0; t<tmp.length(); t++) {
            XDGDesktop *desk = new XDGDesktop(dir.absoluteFilePath(tmp[t]));
            if(desk->type == XDGDesktop::BAD) {
                continue;    //could not read file
            }
            //Now figure out what to do with it
            if(filenames.contains(tmp[t])) {
                //This is an overwrite of a lower-priority (system?) autostart file
                // find the other file
                int old = -1;
                for(int o=0; o<files.length(); o++) {
                    if(files[o]->filePath.endsWith("/"+tmp[t])) {
                        old = o;    //found it
                        break;
                    }
                }
                if(desk->isValid(false)) {
                    //Full override of the lower-priority file (might be replacing exec/tryexec fields)
                    files.takeAt(old)->deleteLater();
                    files.insert(old,desk);
                } else {
                    //Small override file (only the "Hidden" field listed in spec)
                    files[old]->isHidden = desk->isHidden; //replace this value with the override
                    //files << desk; //still add this to the array (will be ignored/skipped later)
                }
            } else {
                //This is a new autostart file
                files << desk;
                filenames << tmp[t];
            }
        }//end of loop over *.desktop files
    } //end of loop over directories

    //Now filter the results by validity if desired
    if(!includeInvalid) {
        for(int i=0; i<files.length(); i++) {
            if( !files[i]->isValid(false) || files[i]->isHidden ) {
                //Invalid file - go ahead and remove it from the output list
                files.takeAt(i)->deleteLater();
                i--;
            }
        }
    }
    return files;
}

bool LXDG::setAutoStarted(bool autostart, QString filePath) {
    //Convenience function for the auto-start setter
    XDGDesktop desk(filePath);
    if(!filePath.endsWith(".desktop")) {
        desk.filePath = filePath;
        desk.useTerminal = false;
    }
    return desk.setAutoStarted(autostart);
}
