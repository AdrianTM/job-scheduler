/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(application);

    QApplication app(argc, argv);

    QString locale = QLocale::system().name();

    QTranslator appTran;
    appTran.load(QCoreApplication::applicationName() + "_" + locale,
                 "/usr/share/" + QCoreApplication::applicationName() + "/locale");
    app.installTranslator(&appTran);

    MainWindow window;
    window.show();
    return app.exec();
}
