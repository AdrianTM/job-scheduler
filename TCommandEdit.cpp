/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QtGui>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>

#include "TCommandEdit.h"
#include "Crontab.h"
#include "CronTime.h"
#include "TimeDialog.h"
#include "Clib.h"

TCommandEdit::TCommandEdit(QWidget *parent)
    : QWidget(parent)
{

    QPushButton *timeButton;
    QPushButton *exeButton;
    //	QPushButton *commandButton;
    QGroupBox *exeBox;
    QHBoxLayout *h;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    {
        mainLayout->addLayout((h = new QHBoxLayout));
        {
            h->addWidget(new QLabel(tr("User:")));
            h->addWidget((userCombo = new QComboBox()));
            h->addWidget((userLabel = new QLabel("")));
            h->addStretch();
        }
        mainLayout->addLayout((h = new QHBoxLayout));
        {
            h->addWidget(new QLabel(tr("Time:")));
            h->addWidget((timeEdit = new QLineEdit()));
            h->addWidget((timeButton =
                    new QPushButton(QIcon::fromTheme("xfce-edit", QIcon(":/images/edit_small.png")), tr("E&dit"))));
        }
        mainLayout->addWidget(new QLabel(tr("Command:")));
        mainLayout->addLayout((h = new QHBoxLayout));
        {
            h->addWidget((commandEdit = new QLineEdit()));
            //			h->addWidget((commandButton =
            //					new QPushButton(QIcon(":/images/open_small.png"), "")));
        }
        mainLayout->addWidget(new QLabel(tr("Comment:")));
        mainLayout->addWidget((commentEdit = new QTextEdit()));
        mainLayout->addWidget((exeBox = new QGroupBox(tr("Execute"))));
        {
            exeBox->setLayout((h = new QHBoxLayout));
            {
                h->addWidget((exeLabel = new QLabel("\n\n\n\n\n")));
                h->addWidget((exeButton =
                        new QPushButton(QIcon::fromTheme("undo", QIcon(":/images/undo_small.png")), tr("&Reset"))));
            }
        }
    }
    setLayout(mainLayout);

    exeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    userLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    userCombo->addItems(Clib::allUsers());
    userCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    userLabel->hide();

    commentEdit->setAutoFormatting(QTextEdit::AutoNone);

    QSizePolicy sp = exeButton->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Fixed);
    exeButton->setSizePolicy(sp);

    viewChanging = true;

    connect(commandEdit, &QLineEdit::textEdited, this, &TCommandEdit::commandEdited);
    connect(timeEdit, &QLineEdit::textEdited, this, &TCommandEdit::timeEdited);
    connect(commentEdit, &QTextEdit::textChanged, this, &TCommandEdit::commentEdited);
    connect(userCombo, QOverload<int>::of(&QComboBox::activated), this, &TCommandEdit::userChanged);
    connect(exeButton, &QPushButton::clicked, this, &TCommandEdit::resetExeTime);
    connect(timeButton, &QPushButton::clicked, this, &TCommandEdit::doTimeDialog);
}

void TCommandEdit::changeCurrent(Crontab *cron, TCommand *cmnd)
{
    viewChanging = true;
    tCommand = cmnd;
    if (cmnd == nullptr) {
        setEnabled(false);
    } else {
        setEnabled(true);
        timeEdit->setText(tCommand->time);
        timeEdit->setCursorPosition(0);
        if (cron->cronOwner == "/etc/crontab") {
            userCombo->setCurrentIndex(userCombo->findText(tCommand->user));
            userCombo->show();
            userLabel->hide();
        } else {
            userLabel->setText("  " + cron->cronOwner + "  ");
            userCombo->hide();
            userLabel->show();
        }
        commandEdit->setText(tCommand->command);
        commandEdit->setCursorPosition(0);
        commentEdit->setPlainText(tCommand->comment);
        setExecuteList(tCommand->time);
    }
    viewChanging = false;
}

void TCommandEdit::setExecuteList(const QString &time)
{

    CronTime cronTime(time);
    if (!cronTime.isValid()) {
        exeLabel->setText("\n\n   " + tr("Time Format Error") + "\n\n\n");
        return;
    }
    QDate today = QDate::currentDate();
    QDate tommorow = today.addDays(1);
    QDateTime cur(QDateTime::currentDateTime());
    QDateTime dt = cur;
    QString str;
    for (int i = 0; i < 5; i++) {
        if (str != "" ) str += '\n';
        dt = cronTime.getNextTime(dt);
        qint64 sec = cur.secsTo(dt);
        str += QString( "%1 - %2:%3:%4 later" )
                .arg(dt.toString("yyyy-MM-dd(ddd) hh:mm"))
                .arg(sec / (60 * 60))
                .arg((sec / 60) % 60 , 2, 10, QChar('0'))
                .arg(sec % 60, 2, 10, QChar('0'));
        if (dt.date() == today)
            str += " - Today";
        else if (dt.date() == tommorow)
            str += " - Tomorrow";
    }
    exeLabel->setText(str);
}

void TCommandEdit::commandEdited(const QString &str)
{
    tCommand->command = str;
    emit dataChanged();
}

void TCommandEdit::timeEdited(const QString &str)
{
    tCommand->time = str;
    emit dataChanged();
    setExecuteList(str);
}

void TCommandEdit::commentEdited()
{
    if (!viewChanging) {
        tCommand->comment = commentEdit->toPlainText();
        emit dataChanged();
    }
}

void TCommandEdit::userChanged(int index)
{

    tCommand->user = userCombo->itemText(index);
    emit dataChanged();

}
void TCommandEdit::resetExeTime()
{
    setExecuteList(timeEdit->text());
}
void TCommandEdit::doTimeDialog()
{
    TimeDialog dialog(timeEdit->text(), this);
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        QString s = dialog.time();
        if (timeEdit->text() != s) {
            timeEdit->setText(s);
            setExecuteList(s);
            tCommand->time = s;
            emit dataChanged();
        }
    }
}
