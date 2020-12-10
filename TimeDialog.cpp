/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QtGui>
#include <QGroupBox>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QStyle>

#include "TimeDialog.h"
#include "CronTime.h"

TimeButton::TimeButton(const QString &label, QWidget *parent)
    : QPushButton(label, parent)
{
    init();
}
TimeButton::TimeButton(int label, QWidget *parent)
    : QPushButton(parent)
{
    setText(QString::number(label));
    setFixedSize( 27, 27 );
    init();
}
void TimeButton::init()
{
    setCheckable(true);
    connect(this, SIGNAL(toggled(bool)),
            this, SLOT(buttonToggled(bool)));

}

void TimeButton::buttonToggled(bool chk)
{
    if (chk) {
        QPalette plt = palette();
        plt.setColor(QPalette::Button, QColor( 163, 194, 186 ));
        plt.setColor(QPalette::ButtonText, QColor( 0, 66, 0 ));
        setPalette(plt);
    }else{
        setPalette(style()->standardPalette());
    }
}

const QStringList MonthName = { " January ", "February", "March", "April", "May", "June",
                                "July", "August", "September", "October", "November", "December" };
const QStringList WeekName = { "Sunday", "Monday", "Tesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const QStringList SimpleName = { "@hourly", "@daily", "@weekly", "@monthly", "@yearly", "@reboot" };


TimeDialog::TimeDialog(const QString &time, QWidget *parent)
    : QDialog(parent), inTime(time)
{
    QHBoxLayout *h;
    QGridLayout *g;
    QVBoxLayout *v;
    QGroupBox *b;
    QPushButton *resetButton;
    QPushButton *okButton;
    QPushButton *cancelButton;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    {
        mainLayout->addLayout((h = new QHBoxLayout));	// time text
        {
            h->addWidget(new QLabel(tr("time")));
            h->addWidget((timeEdit = new QLineEdit()));
        }
        mainLayout->addLayout((h = new QHBoxLayout));	// minutes, hour, day
        {
            h->addWidget((b = new QGroupBox(tr("Minute"))));
            b->setLayout((g = new QGridLayout()));
            minuteBGroup = new QButtonGroup();
            minuteBGroup->setExclusive(false);
            {
                g->setSpacing(0);
                for (int i = 0; i < 60; i++) {
                    QPushButton *btn = new TimeButton(i);
                    g->addWidget(btn, i/10, i%10, 1, 1);
                    minuteBGroup->addButton(btn);
                }
            }
            h->addWidget((b = new QGroupBox(tr("Hour"))));
            b->setLayout((g = new QGridLayout()));
            hourBGroup = new QButtonGroup();
            hourBGroup->setExclusive(false);
            {
                g->setSpacing(0);
                g->addWidget(new QLabel(tr("AM ")), 0, 0, 1, 6, Qt::AlignRight);
                for (int i = 0; i < 12; i++) {
                    QPushButton *btn = new TimeButton(i);
                    g->addWidget(btn, i/6+1, i%6, 1, 1);
                    hourBGroup->addButton(btn);
                }
                g->addWidget(new QLabel(tr("PM ")), 3, 0, 1, 6, Qt::AlignRight);
                for (int i = 12; i < 24; i++) {
                    QPushButton *btn = new TimeButton(i);
                    g->addWidget(btn, i/6+2, i%6, 1, 1);
                    hourBGroup->addButton(btn);
                }
            }
            h->addWidget((b = new QGroupBox(tr("Day"))));
            b->setLayout((g = new QGridLayout()));
            dayBGroup = new QButtonGroup();
            dayBGroup->setExclusive(false);
            {
                g->setSpacing(0);
                for (int i = 0; i < 31; i++) {
                    QPushButton *btn = new TimeButton(i+1);
                    g->addWidget(btn, i/7, i%7, 1, 1);
                    dayBGroup->addButton(btn);
                }
            }
        }
        mainLayout->addLayout((h = new QHBoxLayout));	// month, week, Simple
        {
            h->addWidget((b = new QGroupBox(tr("Month"))));
            b->setLayout((g = new QGridLayout()));
            monthBGroup = new QButtonGroup();
            monthBGroup->setExclusive(false);
            {
                g->setColumnMinimumWidth(0,120);
                g->setColumnMinimumWidth(1,120);
                for (int i = 0; i < 12; i++) {
                    QString str = QString("%1(%2)").arg(MonthName.at(i)).arg(i+1);
                    QPushButton *btn = new TimeButton(str);
                    g->addWidget(btn, i%6, i/6);
                    monthBGroup->addButton(btn);
                }
            }
            h->addWidget((b = new QGroupBox(tr("Week"))));
            b->setLayout((v = new QVBoxLayout()));
            weekBGroup = new QButtonGroup();
            weekBGroup->setExclusive(false);
            {
                for (int i = 0; i < 7; i++) {
                    QString str = QString("%1(%2)").arg(WeekName.at(i)).arg(i);
                    QPushButton *btn = new TimeButton(str);
                    v->addWidget(btn);
                    weekBGroup->addButton(btn);
                }
            }
            h->addWidget((b = new QGroupBox(tr("Simple"))));
            b->setLayout((v = new QVBoxLayout()));
            simpleBGroup = new QButtonGroup();
            simpleBGroup->setExclusive(false);
            {
                for (int i = 0; i < 6; i++) {
                    QPushButton *btn = new TimeButton(SimpleName.at(i));
                    v->addWidget(btn);
                    simpleBGroup->addButton(btn);
                }
            }
            h->addLayout((v = new QVBoxLayout));
            {
                v->addStretch();
                v->addWidget((litCheckBox =new QCheckBox(tr("Enable Literal"))));
                v->addStretch();
                v->addWidget((resetButton = new QPushButton(tr("Reset"))));
                v->addWidget((cancelButton = new QPushButton(tr("Cancel"))));
                v->addWidget((okButton = new QPushButton(tr("Ok"))));
            }
        }
    }
    setLayout(mainLayout);

    outTime = inTime;
    if (!CronTime(outTime).isValid()) {
        timeEdit->setText("Time format error");
        outTime = "* * * * *";
    } else {
        timeEdit->setText(outTime);
    }

    useLiteral = true;
    litCheckBox->setChecked(useLiteral);
    initButtons(outTime);

    connect(minuteBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::minuteButtonClicked);
    connect(hourBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::hourButtonClicked);
    connect(dayBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::dayButtonClicked);
    connect(monthBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::monthButtonClicked);
    connect(weekBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::weekButtonClicked);
    connect(simpleBGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, &TimeDialog::simpleButtonClicked);
    connect(litCheckBox, &QCheckBox::stateChanged, this, &TimeDialog::litCheckBoxChanged);
    connect(resetButton, &QPushButton::clicked, this, &TimeDialog::resetClicked);
    connect(okButton, &QPushButton::clicked, this, &TimeDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &TimeDialog::reject);
}

void TimeDialog::initButtons(const QString &time)
{
    CronTime ctime(time);
    QList<QAbstractButton *> btn;
    btn = minuteBGroup->buttons();
    if (CronTime::isFill(ctime.minute))
        for (int i = 0; i < 60; i++)
            btn[i]->setChecked(false);
    else
        for (int i = 0; i < 60; i++)
            btn[i]->setChecked(ctime.minute[i]);

    btn = hourBGroup->buttons();
    if (CronTime::isFill(ctime.hour))
        for (int i = 0; i < 24; i++)
            btn[i]->setChecked(false);
    else
        for (int i = 0; i < 24; i++)
            btn[i]->setChecked(ctime.hour[i]);

    btn = dayBGroup->buttons();
    if (CronTime::isFill(ctime.day))
        for (int i = 0; i < 31; i++)
            btn[i]->setChecked(false);
    else
        for (int i = 0; i < 31; i++)
            btn[i]->setChecked(ctime.day[i]);

    btn = monthBGroup->buttons();
    if (CronTime::isFill(ctime.month))
        for (int i = 0; i < 12; i++)
            btn[i]->setChecked(false);
    else
        for (int i = 0; i < 12; i++)
            btn[i]->setChecked(ctime.month[i]);

    btn = weekBGroup->buttons();
    if (CronTime::isFill(ctime.week))
        for (int i = 0; i < 7; i++)
            btn[i]->setChecked(false);
    else
        for (int i = 0; i < 7; i++)
            btn[i]->setChecked(ctime.week[i]);
}

void TimeDialog::minuteButtonClicked(QAbstractButton *)
{
    CronTime ctime(outTime);
    QList<QAbstractButton *> btn = minuteBGroup->buttons();
    int cnt = 0;
    for (int i = 0; i < 60; i++)
        if (btn[i]->isChecked())
            cnt++;
    for (int i = 0; i < 60; i++) {
        if (cnt == 0)
            ctime.minute[i] = true;
        else
            ctime.minute[i] = btn[i]->isChecked();
    }

    btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i]->isChecked())
            btn[i]->setChecked(false);

    outTime = ctime.toString(useLiteral);
    timeEdit->setText(outTime);
}
void TimeDialog::hourButtonClicked(QAbstractButton *)
{
    CronTime ctime(outTime);
    QList<QAbstractButton *> btn = hourBGroup->buttons();
    int cnt = 0;
    for (int i = 0; i < 24; i++)
        if (btn[i]->isChecked())
            cnt++;
    for (int i = 0; i < 24; i++)
        if (cnt == 0)
            ctime.hour[i] = true;
        else
            ctime.hour[i] = btn[i]->isChecked();


    btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i]->isChecked())
            btn[i]->setChecked(false);

    outTime = ctime.toString(useLiteral);
    timeEdit->setText(outTime);
}
void TimeDialog::dayButtonClicked(QAbstractButton *)
{
    CronTime ctime(outTime);
    QList<QAbstractButton *> btn = dayBGroup->buttons();
    int cnt = 0;
    for (int i = 0; i < 31; i++)
        if (btn[i]->isChecked())
            cnt++;
    for (int i = 0; i < 31; i++)
        if (cnt == 0)
            ctime.day[i] = true;
        else
            ctime.day[i] = btn[i]->isChecked();


    btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i]->isChecked())
            btn[i]->setChecked(false);

    outTime = ctime.toString(useLiteral);
    timeEdit->setText(outTime);
}
void TimeDialog::monthButtonClicked(QAbstractButton *)
{
    CronTime ctime(outTime);
    QList<QAbstractButton *> btn = monthBGroup->buttons();
    int cnt = 0;
    for (int i = 0; i < 12; i++)
        if ( btn[i]->isChecked() )
            cnt++;
    for (int i = 0; i < 12; i++)
        if (cnt == 0)
            ctime.month[i] = true;
        else
            ctime.month[i] = btn[i]->isChecked();

    btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i]->isChecked())
            btn[i]->setChecked(false);

    outTime = ctime.toString(useLiteral);
    timeEdit->setText(outTime);
}
void TimeDialog::weekButtonClicked(QAbstractButton *)
{
    CronTime ctime(outTime);
    QList<QAbstractButton *> btn = weekBGroup->buttons();
    int cnt = 0;
    for (int i = 0; i < 7; i++)
        if (btn[i]->isChecked())
            cnt++;
    for (int i = 0; i < 7; i++)
        if (cnt == 0)
            ctime.week[i] = true;
        else
            ctime.week[i] = btn[i]->isChecked();

    if (ctime.week[0]) ctime.week[7] = true;

    btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i]->isChecked())
            btn[i]->setChecked(false);


    outTime = ctime.toString(useLiteral);
    timeEdit->setText(outTime);
}
void TimeDialog::simpleButtonClicked(QAbstractButton *b)
{
    QList<QAbstractButton *> btn = simpleBGroup->buttons();
    for (int i = 0; i < 6; i++)
        if (btn[i] == b)
            outTime = btn[i]->text();
        else
            btn[i]->setChecked(false);

    //	QString numeric;
    //	if ( outTime == "@Hourly" ) numeric = "0 * * * *";
    //	else if (outTime == "@Daily") numeric = "0 0 * * *";
    //	else if (outTime == "@Weekly") numeric = "0 0 * * 0";
    //	else if (outTime == "@Monthly") numeric = "0 0 1 * *";
    //	else if (outTime == "@Yearly") numeric = "0 0 1 1 *";
    //	else if (outTime == "@Reboot") numeric = "0 0 * * *";
    //
    //	initButtons(numeric);
    //	if (!useLiteral) {
    //		outTime = numeric;
    //	}
    if (!useLiteral)
        outTime = CronTime(outTime).toString(false);
    initButtons(outTime);

    timeEdit->setText(outTime);

}
void TimeDialog::litCheckBoxChanged(int state)
{
    useLiteral = (state == Qt::Checked) ? true : false;
}
void TimeDialog::resetClicked()
{
    outTime = inTime;
    timeEdit->setText(outTime);
    initButtons(outTime);
}
