//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "page_session_options.h"
#include "ui_page_session_options.h"
#include <QStyleFactory>
//==========
//    PUBLIC
//==========
page_session_options::page_session_options(QWidget *parent) : PageWidget(parent), ui(new Ui::page_session_options()) {
    ui->setupUi(this);
    loading = false;
    //Display formats for panel clock
    ui->combo_session_datetimeorder->clear();
    ui->combo_session_datetimeorder->addItem( tr("Time (Date as tooltip)"), "timeonly");
    ui->combo_session_datetimeorder->addItem( tr("Date (Time as tooltip)"), "dateonly");
    ui->combo_session_datetimeorder->addItem( tr("Time first then Date"), "timedate");
    ui->combo_session_datetimeorder->addItem( tr("Date first then Time"), "datetime");
	
	ui->combo_styles->clear();
	ui->combo_styles->addItems(QStyleFactory::keys());
	
    connect(ui->push_session_resetSysDefaults, SIGNAL(clicked()), this, SLOT(sessionResetSys()) );
    connect(ui->tool_help_time, SIGNAL(clicked()), this, SLOT(sessionShowTimeCodes()) );
    connect(ui->tool_help_date, SIGNAL(clicked()), this, SLOT(sessionShowDateCodes()) );
    connect(ui->line_session_time, SIGNAL(textChanged(QString)), this, SLOT(sessionLoadTimeSample()) );
    connect(ui->line_session_date, SIGNAL(textChanged(QString)), this, SLOT(sessionLoadDateSample()) );
    connect(ui->windowmanager, SIGNAL(textChanged(QString)), this, SLOT(settingChanged()) );
    connect(ui->shutdown_cmd, SIGNAL(textChanged(QString)), this, SLOT(settingChanged()) );
    connect(ui->restart_cmd, SIGNAL(textChanged(QString)), this, SLOT(settingChanged()) );
    connect(ui->lock_cmd, SIGNAL(textChanged(QString)), this, SLOT(settingChanged()) );
    connect(ui->combo_session_datetimeorder, SIGNAL(currentIndexChanged(int)), this, SLOT(settingChanged()) );
    connect(ui->combo_styles, SIGNAL(currentIndexChanged(int)), this, SLOT(settingChanged()) );
    updateIcons();
}

page_session_options::~page_session_options() {

}

//================
//    PUBLIC SLOTS
//================
void page_session_options::SaveSettings() {
    QSettings sessionsettings("7b7b-desktop","sessionsettings");
    sessionsettings.setValue("TimeFormat", ui->line_session_time->text());
    sessionsettings.setValue("DateFormat", ui->line_session_date->text());
    sessionsettings.setValue("DateTimeOrder", ui->combo_session_datetimeorder->currentData().toString());
    sessionsettings.setValue("StyleApplication", ui->combo_styles->currentText());
    sessionsettings.setValue("WindowManager", ui->windowmanager->text());
    sessionsettings.setValue("ShutdownCmd", ui->shutdown_cmd->text());
    sessionsettings.setValue("RestartCmd", ui->restart_cmd->text());
    sessionsettings.setValue("LockCmd", ui->lock_cmd->text());

    emit HasPendingChanges(false);
}

void page_session_options::LoadSettings(int) {
    emit HasPendingChanges(false);
    emit ChangePageTitle( tr("Desktop Settings") );
    loading = true;
    QSettings sessionsettings("7b7b-desktop","sessionsettings");
    ui->line_session_time->setText( sessionsettings.value("TimeFormat","").toString() );
    ui->line_session_date->setText( sessionsettings.value("DateFormat","").toString() );
    int index = ui->combo_session_datetimeorder->findData( sessionsettings.value("DateTimeOrder","timeonly").toString() );
    int indexStyle = ui->combo_styles->findText( sessionsettings.value("StyleApplication","Fusion").toString() );
    ui->combo_session_datetimeorder->setCurrentIndex(index);
	ui->combo_styles->setCurrentIndex(indexStyle);
    ui->windowmanager->setText( sessionsettings.value("WindowManager","").toString() );
    ui->shutdown_cmd->setText( sessionsettings.value("ShutdownCmd","").toString() );
    ui->restart_cmd->setText( sessionsettings.value("RestartCmd","").toString() );
    ui->lock_cmd->setText( sessionsettings.value("LockCmd","").toString() );

    QString lopenWatchFile = QString(getenv("XDG_CONFIG_HOME"))+"/7b7b-desktop/nowatch";

    sessionLoadTimeSample();
    sessionLoadDateSample();
    QApplication::processEvents(); //throw away any interaction events from loading
    loading = false;
}

//=================
//         PRIVATE
//=================
bool page_session_options::verifySettingsReset() {
    bool ok =(QMessageBox::Yes ==  QMessageBox::warning(this, tr("Verify Settings Reset"), tr("Are you sure you want to reset your desktop settings? This change cannot be reversed!"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) );
    return ok;
}

//=================
//    PRIVATE SLOTS
//=================
void page_session_options::sessionResetSys() {
    if( !verifySettingsReset() ) {
        return;    //cancelled
    }
    LDesktopUtils::LoadSystemDefaults();
    QTimer::singleShot(500,this, SLOT(LoadSettings()) );
}

void page_session_options::sessionLoadTimeSample() {
    if(ui->line_session_time->text().simplified().isEmpty()) {
        ui->label_session_timesample->setText( QLocale().toString(QTime::currentTime(), QLocale::ShortFormat) );
    } else {
        ui->label_session_timesample->setText( QTime::currentTime().toString( ui->line_session_time->text() ) );
    }
    settingChanged();
}

void page_session_options::sessionShowTimeCodes() {
    QStringList msg;
    msg << tr("Valid Time Codes:") << "\n";
    msg << QString(tr("%1: Hour without leading zero (1)")).arg("h");
    msg << QString(tr("%1: Hour with leading zero (01)")).arg("hh");
    msg << QString(tr("%1: Minutes without leading zero (2)")).arg("m");
    msg << QString(tr("%1: Minutes with leading zero (02)")).arg("mm");
    msg << QString(tr("%1: Seconds without leading zero (3)")).arg("s");
    msg << QString(tr("%1: Seconds with leading zero (03)")).arg("ss");
    msg << QString(tr("%1: AM/PM (12-hour) clock (upper or lower case)")).arg("A or a");
    msg << QString(tr("%1: Timezone")).arg("t");
    QMessageBox::information(this, tr("Time Codes"), msg.join("\n") );
}

void page_session_options::sessionLoadDateSample() {
    if(ui->line_session_date->text().simplified().isEmpty()) {
        ui->label_session_datesample->setText( QLocale().toString(QTime::currentTime(), QLocale::ShortFormat) );
    } else {
        ui->label_session_datesample->setText( QDate::currentDate().toString( ui->line_session_date->text() ) );
    }
    settingChanged();
}

void page_session_options::sessionShowDateCodes() {
    QStringList msg;
    msg << tr("Valid Date Codes:") << "\n";
    msg << QString(tr("%1: Numeric day without a leading zero (1)")).arg("d");
    msg << QString(tr("%1: Numeric day with leading zero (01)")).arg("dd");
    msg << QString(tr("%1: Day as abbreviation (localized)")).arg("ddd");
    msg << QString(tr("%1: Day as full name (localized)")).arg("dddd");
    msg << QString(tr("%1: Numeric month without leading zero (2)")).arg("M");
    msg << QString(tr("%1: Numeric month with leading zero (02)")).arg("MM");
    msg << QString(tr("%1: Month as abbreviation (localized)")).arg("MMM");
    msg << QString(tr("%1: Month as full name (localized)")).arg("MMMM");
    msg << QString(tr("%1: Year as 2-digit number (15)")).arg("yy");
    msg << QString(tr("%1: Year as 4-digit number (2015)")).arg("yyyy");
    msg << tr("Text may be contained within single-quotes to ignore replacements");
    QMessageBox::information(this, tr("Date Codes"), msg.join("\n") );
}
