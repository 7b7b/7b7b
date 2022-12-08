//===========================================
//  Lumina-DE source code
//  Copyright (c) 2013-2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "LUtils.h"

#include "LuminaOS.h"
#include "LuminaXDG.h"

#include <QApplication>
#include <QtConcurrent>

#include <unistd.h>

//=============
//  LUtils Functions
//=============

//Return the path to one of the XDG standard directories
QString LUtils::standardDirectory(StandardDir dir, bool createAsNeeded) {
// enum StandardDir {Desktop, Documents, Downloads, Music, Pictures, PublicShare, Templates, Videos}
    QString var="XDG_%1_DIR";
    QString defval="$HOME";
    QString val;
    switch (dir) {
    case Desktop:
        var = var.arg("DESKTOP");
        defval.append("/Desktop");
        break;
    case Documents:
        var = var.arg("DOCUMENTS");
        defval.append("/Documents");
        break;
    case Downloads:
        var = var.arg("DOWNLOAD");
        defval.append("/Downloads");
        break;
    case Music:
        var = var.arg("MUSIC");
        defval.append("/Music");
        break;
    case Pictures:
        var = var.arg("PICTURES");
        defval.append("/Pictures");
        break;
    case PublicShare:
        var = var.arg("PUBLICSHARE");
        break;
    case Templates:
        var = var.arg("TEMPLATES");
        break;
    case Videos:
        var = var.arg("VIDEOS");
        defval.append("/Videos");
        break;
    }
    //Read the XDG user dirs file (if it exists)
    QString configdir = getenv("XDG_DATA_HOME");
    if(configdir.isEmpty()) {
        configdir = QDir::homePath()+"/.config";
    }
    QString conffile=configdir+"/user-dirs.dirs";
    if(QFile::exists(conffile)) {
        static QStringList _contents;
        static QDateTime _lastread;
        if(_contents.isEmpty() || _lastread < QFileInfo(conffile).lastModified()) {
            _contents = LUtils::readFile(conffile);
            _lastread = QDateTime::currentDateTime();
        }
        QStringList match = _contents.filter(var+"=");
        if(!match.isEmpty()) {
            val = match.first().section("=",-1).simplified();
            if(val.startsWith("\"")) {
                val = val.remove(0,1);
            }
            if(val.endsWith("\"")) {
                val.chop(1);
            }
        }
    }
    //Now check the value and return it
    if(val.isEmpty()) {
        val = defval;
    }; //use the default value
    val = val.replace("$HOME", QDir::homePath());
    if(createAsNeeded && !QFile::exists(val) ) {
        QDir dir;
        dir.mkpath(val);
    }
    return val;
}

QString LUtils::runCommand(bool &success, QString command, QStringList arguments, QString workdir, QStringList env) {
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); //need output
    //First setup the process environment as necessary
    QProcessEnvironment PE = QProcessEnvironment::systemEnvironment();
    if(!env.isEmpty()) {
        for(int i=0; i<env.length(); i++) {
            if(!env[i].contains("=")) {
                continue;
            }
            PE.insert(env[i].section("=",0,0), env[i].section("=",1,100));
        }
    }
    proc.setProcessEnvironment(PE);
    //if a working directory is specified, check it and use it
    if(!workdir.isEmpty()) {
        proc.setWorkingDirectory(workdir);
    }
    //Now run the command (with any optional arguments)
    if(arguments.isEmpty()) {
        proc.start(command);
    }
    else {
        proc.start(command, arguments);
    }
    //Wait for the process to finish (but don't block the event loop)
    QString info;
    while(!proc.waitForFinished(1000)) {
        if(proc.state() == QProcess::NotRunning) {
            break;    //somehow missed the finished signal
        }
        QString tmp = proc.readAllStandardOutput();
        if(tmp.isEmpty()) {
            proc.terminate();
        }
        else {
            info.append(tmp);
        }
    }
    info.append(proc.readAllStandardOutput()); //make sure we don't miss anything in the output
    success = (proc.exitCode()==0); //return success/failure
    return info;
}

int LUtils::runCmd(QString cmd, QStringList args) {
    bool success;
    LUtils::runCommand(success, cmd, args);
    return success;

    /*QFuture<QStringList> future = QtConcurrent::run(ProcessRun, cmd, args);
    return future.result()[0].toInt(); //turn it back into an integer return code*/
}

QStringList LUtils::getCmdOutput(QString cmd, QStringList args) {
    bool success;
    QString log = LUtils::runCommand(success, cmd, args);
    return log.split("\n");
    /*QFuture<QStringList> future = QtConcurrent::run(ProcessRun, cmd, args);
    return future.result()[1].split("\n"); //Split the return message into lines*/
}

QStringList LUtils::readFile(QString filepath) {
    QStringList out;
    QFile file(filepath);
    if(file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while(!in.atEnd()) {
            out << in.readLine();
        }
        file.close();
    }
    return out;
}

bool LUtils::writeFile(QString filepath, QStringList contents, bool overwrite) {
    QFile file(filepath);
    if(file.exists() && !overwrite) {
        return false;
    }
    bool ok = false;
    if(contents.isEmpty()) {
        contents << "\n";
    }
    if( file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        QTextStream out(&file);
        out << contents.join("\n");
        if(!contents.last().isEmpty()) {
            out << "\n";    //always end with a new line
        }
        file.close();
        ok = true;
    }
    return ok;
}

bool LUtils::isValidBinary(QString& bin) {
    //Trim off any quotes
    if(bin.startsWith("\"") && bin.endsWith("\"")) {
        bin.chop(1);
        bin = bin.remove(0,1);
    }
    if(bin.startsWith("\'") && bin.endsWith("\'")) {
        bin.chop(1);
        bin = bin.remove(0,1);
    }
    //Now look for relative/absolute path
    if(!bin.startsWith("/")) {
        //Relative path: search for it on the current "PATH" settings
        QStringList paths = QString(qgetenv("PATH")).split(":");
        for(int i=0; i<paths.length(); i++) {
            if(QFile::exists(paths[i]+"/"+bin)) {
                bin = paths[i]+"/"+bin;
                break;
            }
        }
    }
    //bin should be the full path by now
    if(!bin.startsWith("/")) {
        return false;
    }
    QFileInfo info(bin);
    bool good = (info.exists() && info.isExecutable());
    if(good) {
        bin = info.absoluteFilePath();
    }
    return good;
}

QSettings* LUtils::openSettings(QString org, QString name, QObject *parent) {
    //Start with the base configuration directory
    QString path = QString(getenv("XDG_CONFIG_HOME")).simplified();
    if(path.isEmpty()) {
        path = QDir::homePath()+"/.config";
    }
    //Now add the organization directory
    path = path+"/"+org;
    QDir dir(path);
    if(!dir.exists()) {
        dir.mkpath(path);
    }
    //Now generate/check the name of the file
    unsigned int user = getuid();
    QString filepath = dir.absoluteFilePath(name+".conf");
    if(user==0) {
        //special case - make sure we don't clobber the user-permissioned file
        QString rootfilepath = dir.absoluteFilePath(name+"_root.conf");
        if(!QFileInfo::exists(rootfilepath) && QFileInfo::exists(filepath)) {
            QFile::copy(filepath, rootfilepath); //make a copy of the user settings before they start to diverge
        }
        return (new QSettings(rootfilepath, QSettings::IniFormat, parent));
    } else {
        return (new QSettings(filepath, QSettings::IniFormat, parent));
    }

}

QStringList LUtils::systemApplicationDirs() {
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

QStringList LUtils::listSubDirectories(QString dir, bool recursive) {
    //This is a recursive method for returning the full paths of all subdirectories (if recursive flag is enabled)
    QDir maindir(dir);
    QStringList out;
    QStringList subs = maindir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Name);
    for(int i=0; i<subs.length(); i++) {
        out << maindir.absoluteFilePath(subs[i]);
        if(recursive) {
            out << LUtils::listSubDirectories(maindir.absoluteFilePath(subs[i]), recursive);
        }
    }
    return out;
}

QString LUtils::PathToAbsolute(QString path) {
    //Convert an input path to an absolute path (this does not check existance ot anything)
    if(path.startsWith("/")) {
        return path;    //already an absolute path
    }
    if(path.startsWith("~")) {
        path.replace(0,1,QDir::homePath());
    }
    if(!path.startsWith("/")) {
        //Must be a relative path
        //if(path.startsWith("./")){ path = path.remove(2); }
        if(path.startsWith("./")) {
            path = path.remove(2, 2);
        }
        path.prepend( QDir::currentPath()+"/");
    }
    return path;
}

QString LUtils::AppToAbsolute(QString path) {
    if(path.startsWith("~/")) {
        path = path.replace("~/", QDir::homePath()+"/" );
    }
    if(path.startsWith("/") || QFile::exists(path)) {
        return path;
    }
    if(path.endsWith(".desktop")) {
        //Look in the XDG dirs
        QStringList dirs = systemApplicationDirs();
        for(int i=0; i<dirs.length(); i++) {
            if(QFile::exists(dirs[i]+"/"+path)) {
                return (dirs[i]+"/"+path);
            }
        }
    } else {
        //Look on $PATH for the binary
        QStringList paths = QString(getenv("PATH")).split(":");
        for(int i=0; i<paths.length(); i++) {
            if(QFile::exists(paths[i]+"/"+path)) {
                return (paths[i]+"/"+path);
            }
        }
    }
    return path;
}

QStringList LUtils::videoExtensions() {
    static QStringList vidExtensions;
    vidExtensions << "avi" << "mkv" << "mp4" << "mov" << "webm" << "wmv";
    return vidExtensions;
}

QStringList LUtils::imageExtensions(bool wildcards) {
    //Note that all the image extensions are lowercase!!
    static QStringList imgExtensions;
    static QStringList imgExtensionsWC;
    if(imgExtensions.isEmpty()) {
        QList<QByteArray> fmt = QImageReader::supportedImageFormats();
        for(int i=0; i<fmt.length(); i++) {
            imgExtensionsWC << "*."+QString::fromLocal8Bit(fmt[i]);
            imgExtensions << QString::fromLocal8Bit(fmt[i]);
        }
    }
    if(wildcards) {
        return imgExtensionsWC;
    }
    return imgExtensions;
}

QStringList LUtils::knownLocales() {
    QDir i18n = QDir(LOS::LuminaShare()+"i18n");
    if( !i18n.exists() ) {
        return QStringList();
    }
    QStringList files = i18n.entryList(QStringList() << "lumina-desktop_*.qm", QDir::Files, QDir::Name);
    if(files.isEmpty()) {
        return QStringList();
    }
    //Now strip off the filename and just leave the locale tag
    for(int i=0; i<files.length(); i++) {
        files[i].chop(3); //remove the ".qm" on the end
        files[i] = files[i].section("_",1,50).simplified();
    }
    files << "en_US"; //default locale
    files.sort();
    return files;
}

void LUtils::setLocaleEnv(QString lang, QString msg, QString time, QString num,QString money,QString collate, QString ctype) {
    //Adjust the current locale environment variables
    bool all = false;
    if(msg.isEmpty() && time.isEmpty() && num.isEmpty() && money.isEmpty() && collate.isEmpty() && ctype.isEmpty() ) {
        if(lang.isEmpty()) {
            return;    //nothing to do - no changes requested
        }
        all = true; //set everything to the "lang" value
    }
    //If no lang given, but others are given, then use the current setting
    if(lang.isEmpty()) {
        lang = getenv("LC_ALL");
    }
    if(lang.isEmpty()) {
        lang = getenv("LANG");
    }
    if(lang.isEmpty()) {
        lang = "en_US";
    }
    //Now go through and set/unset the environment variables
    // - LANG & LC_ALL
    if(!lang.contains(".")) {
        lang.append(".UTF-8");
    }
    setenv("LANG",lang.toUtf8(),1);  //overwrite setting (this is always required as the fallback)
    if(all) {
        setenv("LC_ALL",lang.toUtf8(),1);
    }
    else {
        unsetenv("LC_ALL");    //make sure the custom settings are used
    }
    // - LC_MESSAGES
    if(msg.isEmpty()) {
        unsetenv("LC_MESSAGES");
    }
    else {
        if(!msg.contains(".")) {
            msg.append(".UTF-8");
        }
        setenv("LC_MESSAGES",msg.toUtf8(),1);
    }
    // - LC_TIME
    if(time.isEmpty()) {
        unsetenv("LC_TIME");
    }
    else {
        if(!time.contains(".")) {
            time.append(".UTF-8");
        }
        setenv("LC_TIME",time.toUtf8(),1);
    }
    // - LC_NUMERIC
    if(num.isEmpty()) {
        unsetenv("LC_NUMERIC");
    }
    else {
        if(!num.contains(".")) {
            num.append(".UTF-8");
        }
        setenv("LC_NUMERIC",num.toUtf8(),1);
    }
    // - LC_MONETARY
    if(money.isEmpty()) {
        unsetenv("LC_MONETARY");
    }
    else {
        if(!money.contains(".")) {
            money.append(".UTF-8");
        }
        setenv("LC_MONETARY",money.toUtf8(),1);
    }
    // - LC_COLLATE
    if(collate.isEmpty()) {
        unsetenv("LC_COLLATE");
    }
    else {
        if(!collate.contains(".")) {
            collate.append(".UTF-8");
        }
        setenv("LC_COLLATE",collate.toUtf8(),1);
    }
    // - LC_CTYPE
    if(ctype.isEmpty()) {
        unsetenv("LC_CTYPE");
    }
    else {
        if(!ctype.contains(".")) {
            ctype.append(".UTF-8");
        }
        setenv("LC_CTYPE",ctype.toUtf8(),1);
    }
}

QString LUtils::currentLocale() {
    QString curr = getenv("LC_ALL");// = QLocale::system();
    if(curr.isEmpty()) {
        curr = getenv("LANG");
    }
    if(curr.isEmpty()) {
        curr = "en_US";
    }
    curr = curr.section(".",0,0); //remove any encodings off the end
    return curr;
}

double LUtils::DisplaySizeToBytes(QString dSize) {
    if(dSize.isEmpty() || dSize[dSize.size()-1].isNumber()) {
		return 0.0;
	}
    dSize.remove(" ");
	QStringList metrics = QStringList() << "B" << "K" << "M" << "G" << "T" << "P";
	QString metric = "B";

	if (!dSize[dSize.size()-2].isNumber()) {
		dSize.chop(1);
	}
	metric = dSize.right(1);
	dSize.chop(1);
	
	double num = dSize.toDouble();
	for(int i=0; i<metrics.length(); i++) {
		if(metric==metrics[i]) {
			break;
		}
		num = num*1024.00;
    }

    return num;
}

QString LUtils::BytesToDisplaySize(qint64 bytes) {
    QStringList metrics = QStringList() << "B" << "KB" << "MB" << "GB" << "TB" << "PB";
	int i;

	double dblBytes = bytes;
	for(i = 0; dblBytes >= 1024 && i<metrics.length(); i++) {
		dblBytes /= 1024;
	}
	
	return (QString::number(qRound(dblBytes*100)/100.0)+metrics[i]);
}
