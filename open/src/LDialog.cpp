//===========================================
//  Copyright (c) 2013, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
//  Lumina-DE source code
#include "LDialog.h"
#include "ui_LDialog.h"

LDialog::LDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LDialog() ) {
    ui->setupUi(this); //load the designer file
    //set the output flags to the defaults
    appSelected = false;
    setDefault = false;
    appExec.clear();
    appPath.clear();
    appFile.clear();
    settings = new QSettings("lumina-desktop", "lumina-open", this);
    
    ui->label_goodbin->setPixmap(LXDG::findIcon("cancel", "").pixmap(QSize(24,24)));
    //Connect the signals/slots
    connect(ui->combo_apps, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUI()) );
    connect(ui->radio_rec, SIGNAL(toggled(bool)), this, SLOT(radioChanged()) );
    connect(ui->radio_avail, SIGNAL(toggled(bool)), this, SLOT(radioChanged()) );
    connect(ui->radio_custom, SIGNAL(toggled(bool)), this, SLOT(radioChanged()) );
}

LDialog::~LDialog() {
}

// ----------
//   PUBLIC
// ----------
void LDialog::setFileInfo(QString filename, QString extension, bool isFile) {
    //Set the labels for the file
    qDebug() << "SetFileInfo:" << filename << extension << isFile;
    filePath = filename; //save for later
    ui->label_file->setText( this->fontMetrics().elidedText( filename, Qt::ElideMiddle, 300 ) );
    bool shownetwork = false;
    QString ext;
    if (extension == "") {
		ext = "Undefined type";
	}
    else if(isFile) {
		ext = extension;
    }
    else if(extension=="email") {
		ext = QString(tr("Email Link"));
        shownetwork = true;
    }
    else if(extension.startsWith("x-scheme-handler/")) {
		ext = QString(tr("Internet URL - %1")).arg(extension.section("/",-1));
        shownetwork = true;
    }
    else {
		ext = extension+" link";
    }
	ui->label_extension->setText(ext);
    fileEXT = extension; //NOTE: this is the mime-type for the file now, not the extension
    generateAppList(shownetwork);
}

//static functions
QString LDialog::getDefaultApp(QString extension) {
    if(!extension.contains("/")) {
        return "";
    }
    qDebug() << "Get Default App:" << extension;
    return LXDG::findDefaultAppForMime(extension);
}

void LDialog::setDefaultApp(QString extension, QString appFile) {
    if(!extension.contains("/")) {
        extension = LXDG::findAppMimeForFile(appFile);
    }
    //mime type default: set on the system itself
    //if(appFile.endsWith(".desktop")){ appFile = appFile.section("/",-1); } //only need the relative path
    LXDG::setDefaultAppForMime(extension, appFile);
}

// -----------
//   PRIVATE
// -----------
QStringList LDialog::getPreferredApplications() {
    QStringList out;
    //First list all the applications registered for that same mimetype
    QString mime = fileEXT;
    out << LXDG::findAvailableAppsForMime(mime);

    //Now search the internal settings for that extension and find any applications last used
    QStringList keys = settings->allKeys();
    for(int i=0; i<keys.length(); i++) {
        if(keys[i].startsWith("default/")) {
            continue;    //ignore the defaults (they will also be in the main)
        }
        if(keys[i].toLower() == fileEXT.toLower()) {
            QStringList files = settings->value(keys[i]).toString().split(":::");
            qDebug() << "Found Files:" << keys[i] << files;
            bool cleaned = false;
            for(int j=0; j<files.length(); j++) {
                if(QFile::exists(files[j])) {
                    out << files[j];
                }
                else {
                    files.removeAt(j);    //file no longer available - remove it
                    j--;
                    cleaned=true;
                }
            }
            if(cleaned) {
                settings->setValue(keys[i], files.join(":::"));    //update the registry
            }
            if(!out.isEmpty()) {
                break;    //already found files
            }
        }
    }
    //Make sure we don't have any duplicates before we return the list
    out.removeDuplicates();
    return out;
}

void LDialog::setPreferredApplication(QString desktopfile) {
    QStringList keys = settings->allKeys();
    for(int i=0; i<keys.length(); i++) {
        if(keys[i].toLower() == fileEXT.toLower()) {
            QStringList files = settings->value(keys[i]).toString().split(":::");
            files.removeAll(desktopfile);
            QStringList newfiles;
            newfiles << desktopfile;
            //Only keep the 5 most recent preferred applications per extension
            for(int j=0; j<5 && j<files.length(); j++) {
                newfiles << files[j];
            }
            settings->setValue(keys[i], newfiles.join(":::"));
            return;
        }
    }
    //No key found for this extension - make a new one
    settings->setValue(fileEXT.toLower(), desktopfile);
}

// -----------------
//   PRIVATE SLOTS
// -----------------
void LDialog::updateUI() {
    //Check for a selected application
    bool good = false;
    if(ui->radio_custom->isChecked() && !ui->line_bin->text().isEmpty()) {
		QString bin = ui->line_bin->text();
		QFileInfo* info = new QFileInfo(bin);
		good = (LUtils::isValidBinary(bin) && !info->isDir());

		//Now verify that the file exists and is executable
		QString pixicon = "cancel";
		if( good ) {
			pixicon = "ok";
		}

		ui->label_goodbin->setPixmap(LXDG::findIcon(pixicon, "").pixmap(QSize(24,24)));
    }
    else if(ui->radio_rec->isChecked() || (ui->combo_apps->count() > 0 && !ui->combo_apps->currentData().toString().isEmpty()) ) {
        good = true; //a valid app is always selected on this page if it is available
    }
    ui->tool_ok->setEnabled(good);
}

void LDialog::generateAppList(bool shownetwork) {
    //Now load the preferred applications
    XDGDesktopList applist;
    applist.updateList();
    PREFAPPS = getPreferredApplications();
    qDebug() << "Preferred Apps:" << PREFAPPS;
    ui->combo_rec->clear();
    //Now get the application mimetype for the file extension (if available)
    QStringList mimetypes = LXDG::findAppMimeForFile(filePath, true).split("::::"); //use all mimetypes
    mimetypes.removeDuplicates();
    QString defapp = getDefaultApp(mimetypes.first()); //default application
    if(!defapp.isEmpty() && !PREFAPPS.contains(defapp) ) {
        PREFAPPS << defapp;    //ensure this is listed in the preferred apps list
    }
    //Now add all the detected applications
    QHash< QString, QList<XDGDesktop*> > hash = LXDG::sortDesktopCats( applist.apps(false,true) );
    QStringList cat = hash.keys();
    cat.sort(); //sort alphabetically
    ui->combo_apps->clear();
    for(int c=0; c<cat.length(); c++) {
        QList<XDGDesktop*> app = hash[cat[c]];
        if(app.length()<1) {
            continue;
        }
        for(int a=0; a<app.length(); a++) {
            if(shownetwork && (cat[c].toLower()=="network" || cat[c].toLower()=="utility") ) {
                //Need to show preferred internet applications - look for ones that handle URL's
                if(app[a]->exec.contains("%u") || app[a]->exec.contains("%U")) {
                    //qDebug() << "Add to Preferred Apps:" << app[a]->filePath;
                    PREFAPPS << app[a]->filePath;
                }
            }
            ui->combo_apps->addItem(LXDG::findIcon(app[a]->icon, "application-x-desktop"), app[a]->name, app[a]->filePath);
            //Check to see if this app matches the mime type
            if(!mimetypes.isEmpty()) {
                QStringList tmp = mimetypes;
                tmp << app[a]->mimeList;
                if(tmp.removeDuplicates() > 0 ) {
                    // also put this app in the preferred list
                    //qDebug() << "Mimetype match:" << mimetypes << app[a]->mimeList;
                    PREFAPPS.append(app[a]->filePath);
                    //If this is the first preferred app found - select this app initially
                    if(ui->combo_apps->currentIndex()<=0) {
                        ui->combo_apps->setCurrentIndex(ui->combo_apps->count()-1);
                    }
                }
            }
        }
    }
    if(ui->combo_apps->currentIndex()<=0) {
        ui->combo_apps->setCurrentIndex(2);    //Start on the first "real" app - not the first category header
    }
    //Now add all the preferred applications
    PREFAPPS.removeDuplicates();
    for(int i=0; i<PREFAPPS.length(); i++) {
        XDGDesktop dFile(PREFAPPS[i]);
        if( dFile.isValid() ) {
            QString txt = dFile.name;
            if(PREFAPPS[i] == defapp) {
                txt.prepend( tr("[default] ") );
            }
            ui->combo_rec->addItem( LXDG::findIcon(dFile.icon, "application-x-desktop"), txt);
            if(i==0) {
                ui->combo_rec->setCurrentIndex(0);    //make sure the first item is selected
            }
        } else {
            PREFAPPS.removeAt(i); //invalid app
            i--;
        }
    }
    //Update the UI
    if(PREFAPPS.isEmpty()) {
        ui->radio_rec->setEnabled(false); //no preferred apps
        ui->radio_avail->setChecked(true);
    } else {
        ui->radio_rec->setChecked(true);
    }
}

void LDialog::radioChanged() {
    if(ui->radio_rec->isChecked()) {
        ui->stackedWidget->setCurrentWidget(ui->page_rec);
    } else if(ui->radio_avail->isChecked()) {
        ui->stackedWidget->setCurrentWidget(ui->page_avail);
    } else {
        ui->stackedWidget->setCurrentWidget(ui->page_custom);
    }
    updateUI();
}

void LDialog::on_tool_ok_clicked() {
    appSelected = true;
    setDefault = ui->check_default->isChecked();
    if(ui->radio_custom->isChecked()) {
        appExec = ui->line_bin->text();
	} else {
        //application selected
        QString path;
        if (ui->radio_rec->isChecked()){
			path = PREFAPPS[ui->combo_rec->currentIndex()];
		} else {
			path = ui->combo_apps->currentData().toString();
		}
        XDGDesktop app(path);
        //Set the output variables
        appExec =  app.getDesktopExec();
        appPath = app.path;
        appFile = app.filePath;
        setPreferredApplication(app.filePath); //bump this to the top of the preferred list for next time		
	}
    this->close();
}

void LDialog::on_tool_cancel_clicked() {
    appSelected = false;
    this->close();
}

void LDialog::on_tool_findBin_clicked() {
    QString filepath = QFileDialog::getOpenFileName(this, tr("Find Application Binary"), QDir::homePath() );
    if(!filepath.isEmpty()) {
        ui->line_bin->setText(filepath);
    }
}

void LDialog::on_line_bin_textChanged() {
    updateUI();
}
