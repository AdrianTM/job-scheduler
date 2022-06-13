/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "MainWindow.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(application);

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(app.applicationName()));
    app.setOrganizationName("MX-Linux");

    QTranslator qtTran;
    if (qtTran.load(QLocale::system(), "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTran);

    QTranslator qtBaseTran;
    if (qtBaseTran.load("qtbase_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtBaseTran);

    QTranslator appTran;
    if (appTran.load(app.applicationName() + "_" + QLocale::system().name(), "/usr/share/" + app.applicationName() + "/locale"))
        app.installTranslator(&appTran);

    if (getuid() == 0) {
        qputenv("XDG_RUNTIME_DIR", "/run/user/0");
        qputenv("HOME", "/root");
    }
    MainWindow window;
    window.show();
    return app.exec();
}
