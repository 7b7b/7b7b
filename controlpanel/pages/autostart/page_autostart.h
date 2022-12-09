//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_CONFIG_PAGE_AUTOSTART_H
#define _LUMINA_CONFIG_PAGE_AUTOSTART_H
#include "globals.h"
#include "../PageWidget.h"

namespace Ui {
class page_autostart;
};

class page_autostart : public PageWidget {
    Q_OBJECT
public:
    explicit page_autostart(QWidget *parent);
    ~page_autostart();

public slots:
    void SaveSettings() override;
    void LoadSettings(int screennum) override;
    void updateIcons() override;

private:
    Ui::page_autostart *ui;

    QString getSysApp(bool allowreset);

private slots:
    void addsessionstartapp();
    void addsessionstartbin();
    void addsessionstartfile();
};
#endif
