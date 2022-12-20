#include <QApplication>

#include "mainWindow.h"
#include <LuminaOS.h>
#include <LUtils.h>

#include <LuminaSingleApplication.h>
#include <LuminaXDG.h>

XDGDesktopList *APPSLIST = 0;

int main(int argc, char ** argv)
{
    LSingleApplication a(argc, argv, "lumina-config"); //loads translations inside constructor
    if(!a.isPrimaryProcess()) {
        return 0;
    }

    mainWindow w;
    w.show();

    int retCode = a.exec();
    return retCode;
}
