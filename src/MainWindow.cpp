/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/

#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <memory>
#include <vector>

#include "Clib.h"
#include "CronModel.h"
#include "CronView.h"
#include "Crontab.h"
#include "ExecuteList.h"
#include "ExecuteView.h"
#include "QCloseEvent"
#include "SaveDialog.h"
#include "TCommandEdit.h"
#include "VariableEdit.h"

#include "about.h"
#include "constants.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    readSettings();
    createActions();

    //	statusBar();

    cronModel = new CronModel(&crontabs, this);
    auto *spl = new QSplitter(this);
    cronView = new CronView(cronModel, spl);
    auto *tab = new QTabWidget(spl);
    auto *tCommandEdit = new TCommandEdit(tab);
    auto *variableEdit = new VariableEdit(tab);
    executeList = new ExecuteList(exeMaxNum, exeMaxDate, &crontabs, tab);

    tab->addTab(tCommandEdit, QIcon::fromTheme(QStringLiteral("edit-symbolic"), QIcon(":/images/edit_small.png")),
                tr("&Command"));
    tab->addTab(variableEdit, QIcon::fromTheme(QStringLiteral("edit-tag-symbolic"), QIcon(":/images/edit_small.png")),
                tr("&Variables"));
    tab->addTab(executeList, QIcon::fromTheme(QStringLiteral("view-list-symbolic"), QIcon(":/images/view_text.png")),
                tr("&Job List"));

    spl->addWidget(cronView);
    spl->addWidget(tab);

    connect(cronView, &CronView::viewSelected, this, &MainWindow::changeCurrent);
    connect(cronView, &CronView::viewSelected, tCommandEdit, &TCommandEdit::changeCurrent);
    connect(cronView, &CronView::viewSelected, variableEdit, &VariableEdit::changeCurrent);
    connect(cronView, &CronView::viewSelected, executeList, &ExecuteList::changeCurrent);
    connect(executeList->executeView, &ExecuteView::viewSelected, cronView, &CronView::changeCurrent);

    connect(cronView, &CronView::dataChanged, this, &MainWindow::dataChanged);
    connect(cronView, &CronView::dataChanged, executeList, &ExecuteList::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, this, &MainWindow::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, executeList, &ExecuteList::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, cronView, &CronView::tCommandChanged);
    connect(variableEdit, &VariableEdit::dataChanged, this, &MainWindow::dataChanged);

    connect(deleteAction, &QAction::triggered, cronView, &CronView::removeTCommand);
    connect(copyAction, &QAction::triggered, cronView, &CronView::copyTCommand);
    connect(cutAction, &QAction::triggered, cronView, &CronView::cutTCommand);
    connect(pasteAction, &QAction::triggered, cronView, &CronView::pasteTCommand);
    connect(newAction, &QAction::triggered, cronView, &CronView::newTCommand);

    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCron);
    connect(reloadAction, &QAction::triggered, this, &MainWindow::reloadCron);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutJobScheduler);
    connect(helpAction, &QAction::triggered, this, &MainWindow::displayHelp);
    connect(cronView, &CronView::pasted, this, [this]() {
        changeCurrent(cronView->getCurrentCrontab(), cronView->getCurrentTCommand());
    });
    connect(chuserAction, &QAction::triggered, this, &MainWindow::changeUser);

    initCron();

    if (isMaximized()) {
        resize(QSize(JobScheduler::MAINWINDOW_DEFAULT_WIDTH, JobScheduler::MAINWINDOW_DEFAULT_HEIGHT));
    }
    cronView->resize(viewSize);

    setWindowTitle(Clib::uName() + " - " + tr("Job Scheduler"));
    setWindowIcon(QIcon(":/images/job-scheduler.svg"));

    setCentralWidget(spl);
}

MainWindow::~MainWindow() = default;

void MainWindow::changeUser()
{
    saveCron();
    if (Clib::uId() != 0) {
        if (!QProcess::startDetached(QStringLiteral("job-scheduler-launcher"), {})) {
            QMessageBox::critical(this, tr("Job Scheduler"), tr("Failed to start job-scheduler-launcher."));
            return;
        }
    } else {
        QProcess proc;
        proc.start(QStringLiteral("logname"), {}, QIODevice::ReadOnly);
        if (!proc.waitForFinished()) {
            QMessageBox::critical(this, tr("Job Scheduler"), tr("Failed to determine the login user."));
            return;
        }
        QString user = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (user.isEmpty()) {
            QMessageBox::critical(this, tr("Job Scheduler"), tr("Failed to determine the login user."));
            return;
        }
        if (!QProcess::startDetached(QStringLiteral("runuser"),
                                     {QStringLiteral("-u"), user, QStringLiteral("job-scheduler")})) {
            QMessageBox::critical(this, tr("Job Scheduler"), tr("Failed to start job-scheduler as %1.").arg(user));
            return;
        }
    }
    QApplication::quit();
}

void MainWindow::createActions()
{

    auto *fileMenu = new QMenu(tr("&File"), this);
    QToolBar *fileToolBar = addToolBar(tr("File"));

    newAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("filenew"), QIcon(":/images/filenew.png")),
                                    tr("&New Item"));
    newAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    fileMenu->addSeparator();
    reloadAction
        = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("reload"), QIcon(":/images/reload.png")), tr("&Reload"));
    reloadAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    saveAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("filesave"), QIcon(":/images/filesave.png")),
                                     tr("&Save"));
    saveAction->setShortcut(QKeySequence(tr("Ctrl+S")));
    fileMenu->addSeparator();
    if (Clib::uId() != 0) {
        chuserAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("go-up")), tr("Start as &Root"));
    } else {
        chuserAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("go-down")), tr("Start as &Regular user"));
    }
    chuserAction->setShortcut(QKeySequence(tr("Ctrl+U")));
    fileMenu->addSeparator();

    quitAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("exit"), QIcon(":images/exit.png")), tr("E&xit"));
    quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));

    fileToolBar->addAction(newAction);
    fileToolBar->addAction(reloadAction);
    fileToolBar->addAction(saveAction);
    menuBar()->addMenu(fileMenu);

    auto *editMenu = new QMenu(tr("&Edit"), this);
    QToolBar *editToolBar = addToolBar(tr("Edit"));
    cutAction
        = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-cut"), QIcon(":/images/editcut.png")), tr("Cu&t"));
    cutAction->setShortcut(QKeySequence(tr("Ctrl+X")));
    copyAction = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(":/images/editcopy.png")),
                                     tr("&Copy"));
    copyAction->setShortcut(QKeySequence(tr("Ctrl+C")));
    pasteAction = editMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-paste"), QIcon(":/images/editpaste.png")),
                                      tr("&Paste"));
    pasteAction->setShortcut(QKeySequence(tr("Ctrl+V")));
    editMenu->addSeparator();
    deleteAction = editMenu->addAction(
        QIcon::fromTheme(QStringLiteral("edit-delete"), QIcon(":/images/editdelete.png")), tr("&Delete"));
    deleteAction->setShortcut(QKeySequence(tr("Del")));

    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(pasteAction);
    editToolBar->addSeparator();
    editToolBar->addAction(deleteAction);
    editToolBar->addSeparator();
    editToolBar->addAction(chuserAction);
    menuBar()->addMenu(editMenu);

    auto *helpMenu = new QMenu(tr("&Help"), this);
    aboutAction = helpMenu->addAction(QIcon(":/images/job-scheduler.svg"), tr("&About"));
    helpAction = helpMenu->addAction(QIcon::fromTheme(QStringLiteral("help")), tr("&Help"));
    helpAction->setShortcut(QKeySequence(QStringLiteral("F1")));
    menuBar()->addMenu(helpMenu);

    saveAction->setEnabled(false);
    pasteAction->setEnabled(false);
}

void MainWindow::displayHelp()
{
    const QString helpPath = QStringLiteral("/usr/share/doc/job-scheduler/help.html");
    displayHelpDoc(helpPath, tr("Job Scheduler"));
}

void MainWindow::initCron()
{
    std::vector<std::unique_ptr<Crontab>> tables;

    QStringList readErrors;
    const auto loadTable = [&readErrors](const QString &owner) {
        auto table = std::make_unique<Crontab>(owner);
        if (!table->estr.isEmpty()) {
            readErrors << QStringLiteral("%1: %2").arg(owner, table->estr);
        }
        return table;
    };
    QString user = Clib::uName();
    auto cron = loadTable(user);
    if (cron->estr.isEmpty() && cron->getTCommands().empty() && cron->getComment().isEmpty() && cron->getVariables().empty()) {
        cron->setComment(QLatin1String(""));
        cron->getVariables().push_back(
            std::make_unique<Variable>(QStringLiteral("HOME"), Clib::uHome(), QStringLiteral("Home")));
        cron->getVariables().push_back(
            std::make_unique<Variable>(QStringLiteral("PATH"), Clib::getEnv("PATH"), QStringLiteral("Path")));
        cron->getVariables().push_back(
            std::make_unique<Variable>(QStringLiteral("SHELL"), Clib::uShell(), QStringLiteral("Shell")));
    }
    tables.push_back(std::move(cron));
    if (Clib::uId() == 0) {
        auto etcCron = loadTable(QStringLiteral("/etc/crontab"));
        tables.push_back(std::move(etcCron));

        // Scan /etc/cron.d/ for system cron fragment files
        QDir cronDdir(QStringLiteral("/etc/cron.d"));
        const auto cronDFiles = cronDdir.entryList(QDir::Files | QDir::Readable);
        for (const auto &cf : cronDFiles) {
            if (cf.endsWith(QLatin1Char('~')) || cf.startsWith(QLatin1Char('.'))) {
                continue;
            }
            auto cronFile = loadTable(QStringLiteral("/etc/cron.d/") + cf);
            if (!cronFile->getTCommands().empty() || !cronFile->getVariables().empty()) {
                tables.push_back(std::move(cronFile));
            }
        }

        for (const auto &s : Clib::allUsers()) {
            if (s == user) {
                continue;
            }

            auto userCron = loadTable(s);
            if (!userCron->getTCommands().empty()) {
                tables.push_back(std::move(userCron));
            }
        }
    } else {
        cronView->hideUser();
    }
    replaceTables(tables);

    saveAction->setEnabled(false);
    if (!readErrors.isEmpty()) {
        QMessageBox::warning(this, tr("Could not load schedules"), readErrors.join(QStringLiteral("\n\n")));
    }
}

void MainWindow::replaceTables(std::vector<std::unique_ptr<Crontab>> &tables)
{
    executeList->clearForReload();
    cronModel->replaceTables(tables);
    cronView->resetView();
    executeList->dataChanged();
    // The caller retains the old tables until all dependent views have rebound.
}

void MainWindow::reloadCron()
{

    if (saveAction->isEnabled()
        && QMessageBox::question(this, tr("Job Scheduler"), tr("Not saved since last change.\nAre you OK to reload?"),
                                 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }
    initCron();
}

void MainWindow::saveCron()
{

    std::vector<std::unique_ptr<Crontab>> replacements(crontabs.size());
    bool saved = false;
    bool notSaved = false;

    for (size_t i = 0; i < crontabs.size(); ++i) {
        auto &cron = crontabs[i];
        if (cron->isChanged()) {
            if (!cron->saveRestriction().isEmpty()) {
                QMessageBox::warning(this, cron->getCronOwner(), cron->saveRestriction());
                notSaved = true;
                continue;
            }
            SaveDialog dialog(cron->getCronOwner(), cron->cronText());
            if (dialog.exec() == QDialog::Accepted) {
                bool ret = cron->putCrontab(dialog.getText());
                if (!ret) {
                    QMessageBox::critical(this, tr("Job Scheduler"), cron->estr);
                    notSaved = true;
                } else {
                    replacements[i] = std::make_unique<Crontab>(cron->getCronOwner());
                    saved = true;
                }
            } else {
                notSaved = true;
            }
        }
    }

    if (saved) {
        replaceTables(replacements);
    }
    saveAction->setEnabled(notSaved);
}

void MainWindow::dataChanged()
{
    auto *cron = cronView->getCurrentCrontab();
    cron->setChanged(true);
    saveAction->setEnabled(true);
    cronView->resizeColumnToContents(0);
}

void MainWindow::changeCurrent(Crontab *cron, TCommand *cmnd)
{
    bool flg = (cmnd != nullptr);
    const bool editable = cron && cron->saveRestriction().isEmpty();
    statusBar()->showMessage(cron ? cron->saveRestriction() : QString());
    newAction->setEnabled(editable);
    pasteAction->setEnabled(editable && cronView->canPaste());
    cutAction->setEnabled(flg && editable);
    copyAction->setEnabled(flg);
    deleteAction->setEnabled(flg && editable);
}

void MainWindow::readSettings()
{
    settings.beginGroup(QStringLiteral("Main"));
    exeMaxNum = settings.value(QStringLiteral("MaxListNum"), JobScheduler::MAINWINDOW_DEFAULT_MAX_LIST_NUM).toInt();
    exeMaxDate = settings.value(QStringLiteral("MaxListDate"), JobScheduler::MAINWINDOW_DEFAULT_MAX_LIST_DATE).toInt();
    viewSize = settings.value(QStringLiteral("ViewSize"),
                              QSize(JobScheduler::MAINWINDOW_DEFAULT_VIEW_WIDTH,
                                    JobScheduler::MAINWINDOW_DEFAULT_VIEW_HEIGHT))
                   .toSize();
    restoreGeometry(settings.value(QStringLiteral("Geometry")).toByteArray());
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    settings.beginGroup(QStringLiteral("Main"));
    settings.setValue(QStringLiteral("MaxListNum"), executeList->maxNum);
    settings.setValue(QStringLiteral("MaxListDate"), executeList->maxDate);
    settings.setValue(QStringLiteral("Geometry"), saveGeometry());
    settings.setValue(QStringLiteral("ViewSize"), cronView->size());
    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    bool changed = false;
    for (const auto &cron : crontabs) {
        if (cron->isChanged()) {
            changed = true;
            break;
        }
    }
    if (changed
        && QMessageBox::question(this, tr("Job Scheduler"), tr("Not saved since last change.\nAre you OK to exit?"),
                                 QMessageBox::Ok, QMessageBox::Cancel)
               == QMessageBox::Cancel) {
        event->ignore();
        return;
    }
    writeSettings();
    event->accept();
}

void MainWindow::aboutJobScheduler()
{
    hide();
    displayAboutMsgBox(tr("About Job Scheduler"),
                       tr("<b>Job Scheduler</b>") + " - " + tr("Version: %1").arg(QString(VERSION)) + "<p>"
                           + tr("Job Scheduler is based upon qroneko 0.5.4, released in 2005 by korewaisai (<a "
                                "href=\"mailto:korewaisai@yahoo.co.jp\">korewaisai@yahoo.co.jp</a>)")
                           + "<p>"
                           + tr("Original project page: %1")
                                 .arg(QStringLiteral(
                                     "<a href=\"https://qroneko.sourceforge.net\">https://qroneko.sourceforge.net</a>"))
                           + "<p>"
                           + tr("MX project page: %1")
                                 .arg(QStringLiteral("<a "
                                                     "href=\"https://github.com/mx-linux/job-scheduler\">https://"
                                                     "github.com/mx-linux/job-scheduler</a>")),
                       QStringLiteral("/usr/share/doc/job-scheduler/license.html"),
                       tr("%1 License").arg(windowTitle()));
    show();
}
