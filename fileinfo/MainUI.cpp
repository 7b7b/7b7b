//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QtConcurrent/QtConcurrentRun>
#include "MainUI.h"
#include "ui_MainUI.h"

#include <LUtils.h>

MainUI::MainUI() : QMainWindow(), ui(new Ui::MainUI) {
    ui->setupUi(this); //load the designer form
    terminate_thread = false;
    fileINFO = new QFileInfo();
    UpdateIcons(); //Set all the icons in the dialog
    SetupConnections();
}

MainUI::~MainUI() {
    terminate_thread = true;
    this->close();
}

//=============
//      PUBLIC
//=============
void MainUI::LoadFile(QString path) {

    fileINFO = new QFileInfo(path);
    if(!fileINFO->filePath().isEmpty()) {
        SyncFileInfo();
    }
}

void MainUI::UpdateIcons() {

}

//==============
//     PRIVATE
//==============
void MainUI::stopDirSize() {
    if(sizeThread.isRunning()) {
        terminate_thread = true;
        sizeThread.waitForFinished();
        QApplication::processEvents(); //throw away any last signals waiting to be processed
    }
    terminate_thread = false;
}

void MainUI::GetDirSize(const QString dirname) const {
    const quint16 update_frequency = 2000; //For reducing the number of folder_size_changed calls
    quint64 filesize = 0;
    quint64 file_number = 0;
    quint64 dir_number = 1;
    QDir folder(dirname);
    QFileInfoList file_list;
    QString dir_name;
    QList<QString> head;
    folder.setFilter(QDir::Hidden|QDir::AllEntries|QDir::NoDotAndDotDot);
    file_list = folder.entryInfoList();
    for(int i=0; i<file_list.size(); ++i) {
        if(terminate_thread)
            break;
        if(file_list[i].isDir() && !file_list[i].isSymLink()) {
            ++dir_number;
            head.prepend(file_list[i].absoluteFilePath());
        }
        else
            ++file_number;
        if(!file_list[i].isSymLink())
            filesize += file_list[i].size();
    }
    while(!head.isEmpty()) {
        if(terminate_thread)
            break;
        dir_name = head.takeFirst();
        if(!folder.cd(dir_name)) {
            continue;
        }
        file_list = folder.entryInfoList();
        for(int i=0; i<file_list.size(); ++i) {
            if(file_list[i].isDir() && !file_list[i].isSymLink()) {
                ++dir_number;
                head.prepend(file_list[i].absoluteFilePath());
            }
            else
                ++file_number;
            if(!file_list[i].isSymLink())
                filesize += file_list[i].size();
            if(i%update_frequency == 0)
                emit folder_size_changed(filesize, file_number, dir_number, false);
        }
    }
    emit folder_size_changed(filesize, file_number, dir_number, true);
}

// Initialization procedures
void MainUI::SetupConnections() {
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(closeApplication()) );
    connect(this, SIGNAL(folder_size_changed(quint64, quint64, quint64, bool)), this, SLOT(refresh_folder_size(quint64, quint64, quint64, bool)));
}

//UI Buttons
void MainUI::closeApplication() {
    terminate_thread = true;
    this->close();
}

void MainUI::refresh_folder_size(quint64 size, quint64 files, quint64 folders, bool finished) {
    if(finished)
        ui->label_file_size->setText( LUtils::BytesToDisplaySize( size ) + " - " + tr(" Folders: ") + QString::number(folders) + " / " + tr("Files: ") + QString::number(files) );
    else
        ui->label_file_size->setText( LUtils::BytesToDisplaySize( size ) + " - " + tr(" Folders: ") + QString::number(folders) + " / " + tr("Files: ") + QString::number(files) + tr("  Calculating..." ));
}

void MainUI::SyncFileInfo() {
    stopDirSize();
    if(fileINFO->filePath().isEmpty()) {
        return;
    }
	QString mime = LXDG::findAppMimeForFile(fileINFO->fileName());
	
    if (!fileINFO->isDir()) {
        ui->label_file_name->setText( fileINFO->fileName() );
        this->setWindowTitle(fileINFO->fileName() + " Properties");
        ui->label_file_mimetype->setText( mime );
        ui->label_file_size->setText( LUtils::BytesToDisplaySize( fileINFO->size() ));
    } else {
        ui->label_file_name->setText( fileINFO->absolutePath() );
        this->setWindowTitle(fileINFO->absolutePath() + " Properties");
        ui->label_file_mimetype->setText(tr("inode/directory"));
        ui->label_file_size->setText(tr("Calculating..."));
        
        sizeThread = QtConcurrent::run([=]{
			MainUI::GetDirSize(fileINFO->absoluteFilePath());
		});
    }
    ui->label_file_owner->setText(fileINFO->owner());
    ui->label_file_group->setText(fileINFO->group());
    
    ui->label_file_created->setText( fileINFO->birthTime().toString() );
    ui->label_file_modified->setText( fileINFO->lastModified().toString() );

    //Get the file permissions
    QString perms;
    if(fileINFO->isReadable() && fileINFO->isWritable()) {
        perms = tr("Read/Write");
    }
    else if(fileINFO->isReadable()) {
        perms = tr("Read Only");
    }
    else if(fileINFO->isWritable()) {
        perms = tr("Write Only");
    }
    else {
        perms = tr("No Access");
    }
    ui->label_file_perms->setText(perms);

    //Now the special "type" for the file
    QString ftype;
    if(fileINFO->suffix().toLower()=="desktop") {
        ftype = tr("XDG Shortcut");
    }
    else if(fileINFO->isDir()) {
        ftype = tr("Directory");
    }
    else if(fileINFO->isExecutable()) {
        ftype = tr("Binary");
    }
    else {
        ftype = fileINFO->suffix().toUpper();
    }
    if(fileINFO->isHidden()) {
        ftype = QString(tr("Hidden %1")).arg(ftype);
    }
    ui->label_file_type->setText(ftype);
	
	// Nows the icons
	QString icon = "";
	if (fileINFO->suffix() == "desktop"){
		XDGDesktop *desk = new XDGDesktop(fileINFO->absoluteFilePath(), 0);
		icon = QString(desk->icon);
    }
    //Now load the icon for the file
    if(mime.startsWith("image/")) {
		icon = "image-x-generic";
    } else if(mime.startsWith("video/")) {
		icon = "video-x-generic";
    } else if (fileINFO->isDir() ){
		icon = "folder";
	}
	ui->label_file_icon->setPixmap( LXDG::findIcon( icon, "unknown").pixmap(QSize(64,64)) );
    this->setWindowIcon( LXDG::findIcon( icon, "unknown") );
}
