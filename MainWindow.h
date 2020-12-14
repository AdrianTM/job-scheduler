/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class TCommand;
class Crontab;
class CronView;
class ExecuteList;
class QModelIndex;
class QTabWidget;
class QItemSelectionModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void saveCron();
    void reloadCron();
    void dataChanged();
    void changeCurrent(Crontab *cron, TCommand *cmnd);

    void AboutJobScheduler();

signals:

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void displayHelp();
    void initCron();

    void readSettings();
    void writeSettings();

    void closeEvent(QCloseEvent *event);

    QAction *saveAction;
    QAction *cutAction;
    QAction *newAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *deleteAction;
    QAction *quitAction;
    QAction *reloadAction;
    QAction *aboutAction;
    QAction *aboutQtAction;
    QAction *helpAction;

    CronView *cronView;
    ExecuteList *executeList;

    bool useEtcCron;
    int exeMaxNum;
    int exeMaxDate;
    QSize winSize;
    QSize viewSize;

    QList<Crontab*> crontabs;

};

#endif
