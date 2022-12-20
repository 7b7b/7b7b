//===========================================
//  Lumina-DE source code
//  Copyright (c) 2013, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_OPEN_DIALOG_H
#define _LUMINA_OPEN_DIALOG_H

#include <QDialog>
#include <QFileDialog>

#include <LuminaXDG.h>
#include <LUtils.h>

namespace Ui {
class LDialog;
};

class LDialog : public QDialog {
    Q_OBJECT
public:
    explicit LDialog(QWidget *parent = 0);
    ~LDialog();

    //inputs
    void setFileInfo(QString filename, QString extension, bool isFile = true);

    //outputs
    bool appSelected, setDefault;
    QString appExec;
    QString appPath;
    QString appFile;

    //static functions
    static QString getDefaultApp(QString extension);
    static void setDefaultApp(QString extension, QString appFile);

private:
    Ui::LDialog *ui;
    QString fileEXT, filePath;
    QSettings *settings;
    QStringList PREFAPPS;

    //DB set/get
    QStringList getPreferredApplications();
    void setPreferredApplication(QString);

private slots:
    void updateUI();
    void generateAppList(bool shownetwork = false);
    //Internal UI slots
    void radioChanged();
    //void on_group_binary_toggled(bool checked);
    void on_tool_ok_clicked();
    void on_tool_cancel_clicked();
    void on_tool_findBin_clicked();
    void on_line_bin_textChanged();

};

#endif
