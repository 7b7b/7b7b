#include <QApplication>
#include <QFile>

#include "MainUI.h"
#include <LUtils.h>

int main(int argc, char ** argv)
{
    QApplication a(argc, argv);

    //Read the input variables
    QString path = "";
    for(int i=1; i<argc; i++) {
        QString tmp(argv[i]);
        if(!tmp.startsWith("-")) {
            path = QString(argv[i]);
            break;
        }
    }
    if(!path.isEmpty()) {
        path = LUtils::PathToAbsolute(path);
    } else {
		return 0;
	}
    MainUI w;
    w.LoadFile(path);
    w.show();
    int retCode = a.exec();
    return retCode;
}
