//===========================================
//  Lumina Desktop Source Code
//  Copyright (c) 2016, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef _LUMINA_CONFIG_GLOBALS_H
#define _LUMINA_CONFIG_GLOBALS_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QShortcut>
#include <QToolButton>
#include <QTimer>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>

//Now the Lumina Library classes
#include <LuminaXDG.h>
#include <LUtils.h>
#include <LDesktopUtils.h>
#include <LuminaX11.h>
#include <LuminaOS.h>

#include "pages/PageWidget.h"

#endif

//Now the global class for available system applications
extern XDGDesktopList *APPSLIST;
