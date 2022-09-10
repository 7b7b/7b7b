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
QString LUtils::standardDirectory(StandardDir dir, bool createAsNeeded){
// enum StandardDir {Desktop, Documents, Downloads, Music, Pictures, PublicShare, Templates, Videos}
  QString var="XDG_%1_DIR";
  QString defval="$HOME";
  QString val;
  switch (dir){
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
  if(configdir.isEmpty()){ configdir = QDir::homePath()+"/.config"; }
  QString conffile=configdir+"/user-dirs.dirs";
  if(QFile::exists(conffile)){
    static QStringList _contents;
    static QDateTime _lastread;
    if(_contents.isEmpty() || _lastread < QFileInfo(conffile).lastModified()){
      _contents = LUtils::readFile(conffile);
      _lastread = QDateTime::currentDateTime();
    }
    QStringList match = _contents.filter(var+"=");
    if(!match.isEmpty()){
      val = match.first().section("=",-1).simplified();
      if(val.startsWith("\"")){ val = val.remove(0,1); }
      if(val.endsWith("\"")){ val.chop(1); }
    }
  }
  //Now check the value and return it
  if(val.isEmpty()){ val = defval; }; //use the default value
  val = val.replace("$HOME", QDir::homePath());
  if(createAsNeeded && !QFile::exists(val) ){
    QDir dir;
    dir.mkpath(val);
  }
  return val;
}

QString LUtils::runCommand(bool &success, QString command, QStringList arguments, QString workdir, QStringList env){
  QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); //need output
  //First setup the process environment as necessary
  QProcessEnvironment PE = QProcessEnvironment::systemEnvironment();
    if(!env.isEmpty()){
      for(int i=0; i<env.length(); i++){
    if(!env[i].contains("=")){ continue; }
        PE.insert(env[i].section("=",0,0), env[i].section("=",1,100));
      }
    }
    proc.setProcessEnvironment(PE);
  //if a working directory is specified, check it and use it
  if(!workdir.isEmpty()){
    proc.setWorkingDirectory(workdir);
  }
  //Now run the command (with any optional arguments)
  if(arguments.isEmpty()){ proc.start(command); }
  else{ proc.start(command, arguments); }
  //Wait for the process to finish (but don't block the event loop)
  QString info;
  while(!proc.waitForFinished(1000)){
    if(proc.state() == QProcess::NotRunning){ break; } //somehow missed the finished signal
    QString tmp = proc.readAllStandardOutput();
    if(tmp.isEmpty()){ proc.terminate(); }
    else{ info.append(tmp); }
  }
  info.append(proc.readAllStandardOutput()); //make sure we don't miss anything in the output
  success = (proc.exitCode()==0); //return success/failure
  return info;
}

int LUtils::runCmd(QString cmd, QStringList args){
  bool success;
  LUtils::runCommand(success, cmd, args);
  return success;

  /*QFuture<QStringList> future = QtConcurrent::run(ProcessRun, cmd, args);
  return future.result()[0].toInt(); //turn it back into an integer return code*/
}

QStringList LUtils::getCmdOutput(QString cmd, QStringList args){
  bool success;
  QString log = LUtils::runCommand(success, cmd, args);
  return log.split("\n");
  /*QFuture<QStringList> future = QtConcurrent::run(ProcessRun, cmd, args);
  return future.result()[1].split("\n"); //Split the return message into lines*/
}

QStringList LUtils::readFile(QString filepath){
  QStringList out;
  QFile file(filepath);
  if(file.open(QIODevice::Text | QIODevice::ReadOnly)){
    QTextStream in(&file);
    while(!in.atEnd()){
      out << in.readLine();
    }
    file.close();
  }
  return out;
}

bool LUtils::writeFile(QString filepath, QStringList contents, bool overwrite){
  QFile file(filepath);
  if(file.exists() && !overwrite){ return false; }
  bool ok = false;
  if(contents.isEmpty()){ contents << "\n"; }
  if( file.open(QIODevice::WriteOnly | QIODevice::Truncate) ){
    QTextStream out(&file);
    out << contents.join("\n");
    if(!contents.last().isEmpty()){ out << "\n"; } //always end with a new line
    file.close();
    ok = true;
  }
  return ok;
}

bool LUtils::isValidBinary(QString& bin){
  //Trim off any quotes
  if(bin.startsWith("\"") && bin.endsWith("\"")){ bin.chop(1); bin = bin.remove(0,1); }
  if(bin.startsWith("\'") && bin.endsWith("\'")){ bin.chop(1); bin = bin.remove(0,1); }
  //Now look for relative/absolute path
  if(!bin.startsWith("/")){
    //Relative path: search for it on the current "PATH" settings
    QStringList paths = QString(qgetenv("PATH")).split(":");
    for(int i=0; i<paths.length(); i++){
      if(QFile::exists(paths[i]+"/"+bin)){ bin = paths[i]+"/"+bin; break;}
    }
  }
  //bin should be the full path by now
  if(!bin.startsWith("/")){ return false; }
  QFileInfo info(bin);
  bool good = (info.exists() && info.isExecutable());
  if(good){ bin = info.absoluteFilePath(); }
  return good;
}

QSettings* LUtils::openSettings(QString org, QString name, QObject *parent){
  //Start with the base configuration directory
  QString path = QString(getenv("XDG_CONFIG_HOME")).simplified();
  if(path.isEmpty()){ path = QDir::homePath()+"/.config"; }
  //Now add the organization directory
  path = path+"/"+org;
  QDir dir(path);
  if(!dir.exists()){ dir.mkpath(path); }
  //Now generate/check the name of the file
  unsigned int user = getuid();
  QString filepath = dir.absoluteFilePath(name+".conf");
  if(user==0){
    //special case - make sure we don't clobber the user-permissioned file
    QString rootfilepath = dir.absoluteFilePath(name+"_root.conf");
    if(!QFileInfo::exists(rootfilepath) && QFileInfo::exists(filepath)){
      QFile::copy(filepath, rootfilepath); //make a copy of the user settings before they start to diverge
    }
    return (new QSettings(rootfilepath, QSettings::IniFormat, parent));
  }else{
    return (new QSettings(filepath, QSettings::IniFormat, parent));
  }

}

QStringList LUtils::systemApplicationDirs(){
  //Returns a list of all the directories where *.desktop files can be found
  QStringList appDirs = QString(getenv("XDG_DATA_HOME")).split(":");
  appDirs << QString(getenv("XDG_DATA_DIRS")).split(":");
  if(appDirs.isEmpty()){ appDirs << "/usr/local/share" << "/usr/share" << LOS::AppPrefix()+"/share" << LOS::SysPrefix()+"/share" << L_SHAREDIR; }
  appDirs.removeDuplicates();
  //Now create a valid list
  QStringList out;
  for(int i=0; i<appDirs.length(); i++){
    if( QFile::exists(appDirs[i]+"/applications") ){
      out << appDirs[i]+"/applications";
      //Also check any subdirs within this directory
      // (looking at you KDE - stick to the standards!!)
      out << LUtils::listSubDirectories(appDirs[i]+"/applications");
    }
  }
  //qDebug() << "System Application Dirs:" << out;
  return out;
}

QString LUtils::GenerateOpenTerminalExec(QString term, QString dirpath){
  //Check the input terminal application (default/fallback - determined by calling application)
  //if(!LUtils::isValidBinary(term)){
    if(term.endsWith(".desktop")){
      //Pull the binary name out of the shortcut
      XDGDesktop DF(term);
      if(DF.type == XDGDesktop::BAD){ term = "xterm"; }
      else{ term= DF.exec.section(" ",0,0); } //only take the binary name - not any other flags
    }else{
	term = "xterm"; //fallback
    }
  //}
  //Now create the calling command for the designated terminal
  // NOTE: While the "-e" routine is supposed to be universal, many terminals do not properly use it
  //  so add some special/known terminals here as necessary
  QString exec;
  qWarning() << " - Reached terminal initialization" << term;
  if(term=="mate-terminal" || term=="lxterminal" || term=="gnome-terminal"){
    exec = term+" --working-directory=\""+dirpath+"\"";
  }else if(term=="xfce4-terminal"){
    exec = term+" --default-working-directory=\""+dirpath+"\"";
  }else if(term=="konsole" || term == "qterminal"){
    exec = term+" --workdir \""+dirpath+"\"";
  }else{
    //-e is the parameter for most of the terminal appliction to execute an external command.
    //In this case we start a shell in the selected directory
      //Need the user's shell first
      QString shell = QString(getenv("SHELL"));
      if(!LUtils::isValidBinary(shell)){ shell = "/bin/sh"; } //universal fallback for a shell
    exec = term + " -e \"cd " + dirpath + " && " + shell + " \" ";
  }
  qDebug() << exec;
  return exec;
}

QStringList LUtils::listSubDirectories(QString dir, bool recursive){
  //This is a recursive method for returning the full paths of all subdirectories (if recursive flag is enabled)
  QDir maindir(dir);
  QStringList out;
  QStringList subs = maindir.entryList(QDir::NoDotAndDotDot | QDir::Dirs, QDir::Name);
  for(int i=0; i<subs.length(); i++){
    out << maindir.absoluteFilePath(subs[i]);
    if(recursive){
      out << LUtils::listSubDirectories(maindir.absoluteFilePath(subs[i]), recursive);
    }
  }
  return out;
}

QString LUtils::PathToAbsolute(QString path){
  //Convert an input path to an absolute path (this does not check existance ot anything)
  if(path.startsWith("/")){ return path; } //already an absolute path
  if(path.startsWith("~")){ path.replace(0,1,QDir::homePath()); }
  if(!path.startsWith("/")){
    //Must be a relative path
    //if(path.startsWith("./")){ path = path.remove(2); }
    if(path.startsWith("./")){ path = path.remove(2, 2); }
    path.prepend( QDir::currentPath()+"/");
  }
  return path;
}

QString LUtils::AppToAbsolute(QString path){
  if(path.startsWith("~/")){ path = path.replace("~/", QDir::homePath()+"/" ); }
  if(path.startsWith("/") || QFile::exists(path)){ return path; }
  if(path.endsWith(".desktop")){
    //Look in the XDG dirs
    QStringList dirs = systemApplicationDirs();
    for(int i=0; i<dirs.length(); i++){
      if(QFile::exists(dirs[i]+"/"+path)){ return (dirs[i]+"/"+path); }
    }
  }else{
    //Look on $PATH for the binary
    QStringList paths = QString(getenv("PATH")).split(":");
    for(int i=0; i<paths.length(); i++){
      if(QFile::exists(paths[i]+"/"+path)){ return (paths[i]+"/"+path); }
    }
  }
  return path;
}

QStringList LUtils::videoExtensions() {
  static QStringList vidExtensions;
  vidExtensions << "avi" << "mkv" << "mp4" << "mov" << "webm" << "wmv";
  return vidExtensions;
}

QStringList LUtils::imageExtensions(bool wildcards){
  //Note that all the image extensions are lowercase!!
  static QStringList imgExtensions;
  static QStringList imgExtensionsWC;
  if(imgExtensions.isEmpty()){
    QList<QByteArray> fmt = QImageReader::supportedImageFormats();
    for(int i=0; i<fmt.length(); i++){
      imgExtensionsWC << "*."+QString::fromLocal8Bit(fmt[i]);
      imgExtensions << QString::fromLocal8Bit(fmt[i]);
    }
  }
  if(wildcards){ return imgExtensionsWC; }
  return imgExtensions;
}

QStringList LUtils::knownLocales(){
  QDir i18n = QDir(LOS::LuminaShare()+"i18n");
    if( !i18n.exists() ){ return QStringList(); }
  QStringList files = i18n.entryList(QStringList() << "lumina-desktop_*.qm", QDir::Files, QDir::Name);
    if(files.isEmpty()){ return QStringList(); }
  //Now strip off the filename and just leave the locale tag
  for(int i=0; i<files.length(); i++){
     files[i].chop(3); //remove the ".qm" on the end
     files[i] = files[i].section("_",1,50).simplified();
  }
  files << "en_US"; //default locale
  files.sort();
  return files;
}

void LUtils::setLocaleEnv(QString lang, QString msg, QString time, QString num,QString money,QString collate, QString ctype){
  //Adjust the current locale environment variables
  bool all = false;
  if(msg.isEmpty() && time.isEmpty() && num.isEmpty() && money.isEmpty() && collate.isEmpty() && ctype.isEmpty() ){
    if(lang.isEmpty()){ return; } //nothing to do - no changes requested
    all = true; //set everything to the "lang" value
  }
  //If no lang given, but others are given, then use the current setting
  if(lang.isEmpty()){ lang = getenv("LC_ALL"); }
  if(lang.isEmpty()){ lang = getenv("LANG"); }
  if(lang.isEmpty()){ lang = "en_US"; }
  //Now go through and set/unset the environment variables
  // - LANG & LC_ALL
  if(!lang.contains(".")){ lang.append(".UTF-8"); }
  setenv("LANG",lang.toUtf8() ,1); //overwrite setting (this is always required as the fallback)
  if(all){ setenv("LC_ALL",lang.toUtf8() ,1); }
  else{ unsetenv("LC_ALL"); } //make sure the custom settings are used
  // - LC_MESSAGES
  if(msg.isEmpty()){ unsetenv("LC_MESSAGES"); }
  else{
    if(!msg.contains(".")){ msg.append(".UTF-8"); }
    setenv("LC_MESSAGES",msg.toUtf8(),1);
  }
  // - LC_TIME
  if(time.isEmpty()){ unsetenv("LC_TIME"); }
  else{
    if(!time.contains(".")){ time.append(".UTF-8"); }
    setenv("LC_TIME",time.toUtf8(),1);
  }
    // - LC_NUMERIC
  if(num.isEmpty()){ unsetenv("LC_NUMERIC"); }
  else{
    if(!num.contains(".")){ num.append(".UTF-8"); }
    setenv("LC_NUMERIC",num.toUtf8(),1);
  }
    // - LC_MONETARY
  if(money.isEmpty()){ unsetenv("LC_MONETARY"); }
  else{
    if(!money.contains(".")){ money.append(".UTF-8"); }
    setenv("LC_MONETARY",money.toUtf8(),1);
  }
    // - LC_COLLATE
  if(collate.isEmpty()){ unsetenv("LC_COLLATE"); }
  else{
    if(!collate.contains(".")){ collate.append(".UTF-8"); }
    setenv("LC_COLLATE",collate.toUtf8(),1);
  }
    // - LC_CTYPE
  if(ctype.isEmpty()){ unsetenv("LC_CTYPE"); }
  else{
    if(!ctype.contains(".")){ ctype.append(".UTF-8"); }
    setenv("LC_CTYPE",ctype.toUtf8(),1);
  }
}

QString LUtils::currentLocale(){
  QString curr = getenv("LC_ALL");// = QLocale::system();
  if(curr.isEmpty()){ curr = getenv("LANG"); }
  if(curr.isEmpty()){ curr = "en_US"; }
  curr = curr.section(".",0,0); //remove any encodings off the end
  return curr;
}

double LUtils::DisplaySizeToBytes(QString num){
  //qDebug() << "Convert Num to Bytes:" << num;
  num = num.toLower().simplified();
  num = num.remove(" ");
  if(num.isEmpty()){ return 0.0; }
  if(num.endsWith("b")){ num.chop(1); } //remove the "bytes" marker (if there is one)
  QString lab = "b";
  if(!num[num.size()-1].isNumber()){
    lab = num.right(1); num.chop(1);
  }
  double N = num.toDouble();
  QStringList labs; labs <<"b"<<"k"<<"m"<<"g"<<"t"<<"p"; //go up to petabytes for now
  for(int i=0; i<labs.length(); i++){
    if(lab==labs[i]){ break; }//already at the right units - break out
    N = N*1024.0; //Move to the next unit of measurement
  }
  //qDebug() << " - Done:" << QString::number(N) << lab << num;
  return N;
}

QString LUtils::BytesToDisplaySize(qint64 ibytes){
  static QStringList labs = QStringList();
  if(labs.isEmpty()){ labs << "B" << "K" << "M" << "G" << "T" << "P"; }
  //Now get the dominant unit
  int c=0;
  double bytes = ibytes; //need to keep decimel places for calculations
  while(bytes>=1000 && c<labs.length() ){
    bytes = bytes/1024;
    c++;
  } //labs[c] is the unit
  //Bytes are now
  //Now format the number (up to 3 digits, not including decimel places)
  QString num;
  if(bytes>=100){
    //No decimel places
    num = QString::number(qRound(bytes));
  }else if(bytes>=10){
    //need 1 decimel place
    num = QString::number( (qRound(bytes*10)/10.0) );
  }else if(bytes>=1){
    //need 2 decimel places
    num = QString::number( (qRound(bytes*100)/100.0) );
  }else{
    //Fully decimel (3 places)
    num = "0."+QString::number(qRound(bytes*1000));
  }
  //qDebug() << "Bytes to Human-readable:" << bytes << c << num << labs[c];
  return (num+labs[c]);
}

QString LUtils::SecondsToDisplay(int secs){
  if(secs < 0){ return "??"; }
  QString rem; //remaining
  if(secs > 3600){
    int hours = secs/3600;
    rem.append( QString::number(hours)+"h ");
    secs = secs - (hours*3600);
  }
  if(secs > 60){
    int min = secs/60;
    rem.append( QString::number(min)+"m ");
    secs = secs - (min*60);
  }
  if(secs > 0){
    rem.append( QString::number(secs)+"s");
  }else{
    rem.append( "0s" );
  }
  return rem;
}
