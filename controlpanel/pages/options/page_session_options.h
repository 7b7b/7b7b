//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_CONFIG_PAGE_SESSION_OPTIONS_H
#define _LUMINA_CONFIG_PAGE_SESSION_OPTIONS_H
#include "globals.h"
#include "../PageWidget.h"

namespace Ui {
class page_session_options;
};

class page_session_options : public PageWidget {
    Q_OBJECT
public:
    explicit page_session_options(QWidget *parent);
    ~page_session_options();

public slots:
    void SaveSettings() override;
    void LoadSettings(int screennum = -1) override;
    void updateIcons() override;

    virtual void settingChanged() {
        if(!loading) {
            emit HasPendingChanges(true);
        }
    }
private:
    Ui::page_session_options *ui;
    bool loading;
    bool verifySettingsReset();

private slots:
    void sessionResetSys();
    void sessionResetLumina();
    void sessionLoadTimeSample();
    void sessionShowTimeCodes();
    void sessionLoadDateSample();
    void sessionShowDateCodes();
};
#endif
