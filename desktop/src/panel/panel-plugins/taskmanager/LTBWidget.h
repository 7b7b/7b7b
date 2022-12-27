//===========================================
//  Lumina-DE source code
//  Copyright (c) 2013, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_TOOLBAR_WIDGET_H
#define _LUMINA_TOOLBAR_WIDGET_H

#include <QToolButton>
#include <QEvent>
#include <QWheelEvent>

#include "Globals.h"
#include <LuminaX11.h>

class LTBWidget : public QToolButton {
    Q_OBJECT

private:
    LXCB::WINDOWVISIBILITY cstate;
    QString rawstyle;
    void updateBackground() {
        if(cstate == LXCB::IGNORE) {
            this->setObjectName("");    //just use the defaults
        }
        else if(cstate == LXCB::VISIBLE) {
            this->setObjectName("WindowVisible");
        }
        else if(cstate == LXCB::INVISIBLE) {
            this->setObjectName("WindowInvisible");
        }
        else if(cstate == LXCB::ACTIVE) {
            this->setObjectName("WindowActive");
        }
    }

signals:

    void wheelScroll(int change);

public:
    explicit LTBWidget(QWidget* parent)
        : QToolButton(parent),
          cstate(LXCB::IGNORE) {

        this->setPopupMode(QToolButton::InstantPopup);
        this->setAutoRaise(true);
        updateBackground();
    }

    ~LTBWidget() {
    }

    void setState(LXCB::WINDOWVISIBILITY newstate) {
        cstate = newstate;
        updateBackground();
    }

public slots:


protected:
    void wheelEvent(QWheelEvent *event) {
        // 1/15th of a rotation (delta/120) is usually one "click" of the wheel
        int change = event->angleDelta().y()/120;
        emit wheelScroll(change);
    }

};

#endif
