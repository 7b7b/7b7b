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

class LTBWidget : public QToolButton{
	Q_OBJECT

private:
	LXCB::WINDOWVISIBILITY cstate;
	QString rawstyle;
	void updateBackground(){
	  if(cstate == LXCB::IGNORE){  this->setObjectName(""); } //just use the defaults
	  else if(cstate == LXCB::VISIBLE){ this->setObjectName("WindowVisible"); }//background = "background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 rgba(255, 255, 255, 240), stop:0.505682 rgba(240, 240, 240, 150), stop:1 rgba(210, 210, 210, 55));"; border="border: 1px solid transparent;"; }
	  else if(cstate == LXCB::INVISIBLE){this->setObjectName("WindowInvisible"); } //background = "background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 rgba(215, 215, 215, 240), stop:0.505682 rgba(184, 185, 186, 150), stop:1 rgba(221, 246, 255, 55));"; border="border: 1px solid transparent;"; }
	  else if(cstate == LXCB::ACTIVE){ this->setObjectName("WindowActive"); }//background= "background: qlineargradient(spkground: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 rgba(252, 187, 127, 240), stop:0.505682 rgba(255, 222, 197, 150), stop:1 rgba(221, 246, 255, 55));"; border="border: 1px solid transparent;"; }
	  
	  QString raw = rawstyle;
	  this->setStyleSheet( raw.replace("%1","background: transparent; ").replace("%2", "border: 1px solid transparent;") );
	}
	
signals:

	void wheelScroll(int change);
	
public:
	LTBWidget(QWidget* parent) : QToolButton(parent){
	  cstate = LXCB::IGNORE;
		
	  this->setPopupMode(QToolButton::InstantPopup);
	  this->setAutoRaise(true);
	  
	  rawstyle = "LTBWidget{ %1 border-radius: 5px;}  \
	  LTBWidget::menu-indicator{image: none;} \
	  LTBWidget::hover{ background: palette(highlight); border-radius: 5px; } \
	  LTBWidget::menu-button{ background: transparent; width: 15px; } \
	  LTBWidget[popupMode=\"1\"]{%1 %2 border-radius: 5px; padding-right: 15px;} \
	  LTBWidget[popupMode=\"1\"]::hover{%1 border: 1px solid black; border-radius: 5px; padding-right: 15px}";
	  updateBackground();
	}
	
	~LTBWidget(){ 
	}
	
	void setState(LXCB::WINDOWVISIBILITY newstate){
	  cstate = newstate;
	  updateBackground();
	}
	
public slots:
	

protected:
	void wheelEvent(QWheelEvent *event){
	  // 1/15th of a rotation (delta/120) is usually one "click" of the wheel
	  int change = event->angleDelta().y()/120;
	  emit wheelScroll(change);
	}

};

#endif
