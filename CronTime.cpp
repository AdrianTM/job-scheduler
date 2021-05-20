/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QtCore>

#include "CronTime.h"

static const QStringList WeekNames = { "sun", "mon", "tue", "wed", "thu", "fri", "sat", "sun" };
static const QStringList MonthNames = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };


inline QString upcaseHead( const QString &str )
{
    return str.at(0).toUpper() + str.mid(1);
}

QString toNumeric(const QString &str, const QStringList &names, int start)
{
    QString ret = str;
    for (int i = 0; i < names.size(); ++i)
        ret = ret.replace(names.at(i), QString::number(i+start));
    return ret;
}

inline QString toMonthNumeric(const QString &str)
{
    return toNumeric(str, MonthNames, 1);
}

inline QString toWeekNumeric(const QString &str)
{
    return toNumeric(str, WeekNames, 0);
}


CronTime::CronTime(const QString &tstr)
{
    bValid = false;
    QString str = tstr.toLower();
    if (str.at(0) == '@') {
        QString s = str.mid(1);
        if (s == "hourly")
            str = "0 * * * *";
        else if (s == "daily" || s == "midnight")
            str = "0 0 * * *";
        else if (s == "weekly")
            str = "0 0 * * 0";
        else if (s == "monthly")
            str = "0 0 1 * *";
        else if (s == "yearly" || s == "annually")
            str = "0 0 1 1 *";
        else if (s == "reboot")
            str = "0 0 * * *";
    }
    QStringList slist = str.split(' ');

    if (slist.count() != 5) {
        qDebug() << "CronTime::CronTime Invalid Item count:" << slist.count();
        return;
    }
    minute = toBit(0, 60, slist.at(0));
    if (minute.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Minute Format:" << slist.at(0);
        return;
    }
    hour = toBit(0, 24, slist.at(1));
    if (hour.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Hour Format:" << slist.at(1);
        return;
    }
    day = toBit(1, 31, slist.at(2));
    if (day.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Day Format:" << slist.at(2);
        return;
    }
    month = toBit(1, 12, toMonthNumeric(slist.at(3)));
    if (month.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Month Format:" << slist.at(3);
        return;
    }
    week = toBit(0, 8, toWeekNumeric(slist.at(4)));
    if (week.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Week Format:" << slist.at(4);
        return;
    }
    if (week.at(7)) week[0] = true;
    if (week.at(0)) week[7] = true;

    bValid = true;
}

bool CronTime::isValid()
{
    return bValid;
}

QString CronTime::toString(bool literal)
{

    QString ret;
    if (literal)
        ret	= toString(minute, 0) + " " +  toString(hour,0) + " " +
                toString(day, 1) + " " +
                toMonthLiteral(toString(month, 1)) + " " +
                toWeekLiteral(toString(week, 0));
    else
        ret	= toString(minute, 0) + " " +  toString(hour,0) + " " +
                toString(day, 1) + " " + toString(month, 1) + " " +
                toString(week, 0);

    return ret;
}

QBitArray CronTime::toBit(int start, int num, const QString &str)
{
    QRegularExpression reg("^\\d+$");
    QBitArray ret(num);
    QBitArray NG(0);
    int fp, ep;
    for (auto& s : str.split(',')) {
        int deg = 1;

        if (s.contains('/')) {
            deg = (s.section('/', 1, 1)).toInt();
            if (deg == 0)
                return NG;
            s = s.section('/', 0, 0);
            if (s.isEmpty())
                return NG;
        }
        if (s.contains('-')) {
            QString fs = s.section('-', 0, 0);
            QString es = s.section('-', 1, 1);
            if ( fs.isEmpty() || fs == "*" ) fp = 0;
            else {
                if (!fs.contains(reg))
                    return NG;
                fp = fs.toInt() - start;
            }
            if ( es.isEmpty() || es == "*" ) ep = num - 1;
            else {
                if (!es.contains(reg))
                    return NG;
                ep = es.toInt() - start;
            }
            if ( fp > ep )
                return NG;
        } else if (s == "*") {
            fp = 0;
            ep = num - 1;
        } else {
            if (!s.contains(reg))
                return NG;
            fp = s.toInt() - start;
            ep = fp;
        }
        if ( fp < 0 )
            return NG;
        for (int i = fp; i <= ep && i < num; i += deg)
            ret[i] = true;
    }
    return ret;
}

QString CronTime::toTimeString(int start, int cnt, int interval)
{

    if (cnt == 1)
        return QString("%1").arg(start);

    if (cnt == 2)
        return QString("%1,%2").arg(start).arg(start + interval);

    if (interval == 1)
        return QString("%1-%2").arg(start).arg(start + cnt - 1);

    return  QString("%1-%2/%3").arg(start)
            .arg(start + (cnt - 1) * interval)
            .arg(interval);
}

QString CronTime::toString(const QBitArray &bit, int start)
{
    if (isFill(bit))
        return ("*");

    QStringList lst;
    int size = bit.size();
    if ( size == 8 ) size = 7;
    int cnt = 0;
    int space_cnt = 0;
    int interval_cnt = 0;
    int first_pnt = 0;
    for (int i = 0; i < size; i++) {
        if (bit.at(i)) {
            if (cnt == 0)
                first_pnt = i;
            else if (cnt == 1)
                interval_cnt = space_cnt;
            else if (space_cnt != interval_cnt) {
                lst << toTimeString(first_pnt + start, cnt, interval_cnt+1);
                first_pnt = i;
                interval_cnt = 0;
                cnt = 0;
            }
            cnt++;
            space_cnt = 0;
        } else
            space_cnt++;

    }
    if (first_pnt == 0 && cnt > 2 && ((interval_cnt + 1) * cnt) >= size )
        lst << QString("*/%1").arg(interval_cnt + 1);
    else
        lst << toTimeString(first_pnt + start, cnt, interval_cnt + 1);

    QString ret;
    for (const QString &s : lst) {
        if (!ret.isEmpty())
            ret += ',';
        ret += s;
    }

    return ret;
}

bool CronTime::isFill(const QBitArray &bit)
{
    bool ret = true;
    for (int i = 0; i < bit.size(); i++) {
        if (!bit.at(i)) {
            ret = false;
            break;
        }
    }
    return ret;
}

//
// this module is key of overall performance,
// but i won't change so far.
//
QDateTime CronTime::getNextTime(const QDateTime &chk)
{
    QDateTime tm = chk.addSecs(60 - chk.time().second());
    QTime clear(0,0);
    while (true) {
        if (!month.at(tm.date().month() - 1)) {
            tm = tm.addMonths(1);
            tm.setTime(clear);
            tm.setDate(QDate(tm.date().year(), tm.date().month(), 1));
            continue;
        } else if (!day.at(tm.date().day() - 1) || !week.at(tm.date().dayOfWeek())) {
            tm = tm.addDays(1);
            tm.setTime(clear);
            continue;
        } else if (!hour.at(tm.time().hour())) {
            tm = tm.addSecs(60 * (60 - tm.time().minute()));
            continue;
        } else if (!minute.at(tm.time().minute())) {
            tm = tm.addSecs(60);
        } else {
            break;
        }

    }
    return tm;
}

QString CronTime::toWeekLiteral(const QString &str)
{
    QString ret = str;
    if (!str.contains(QRegularExpression( "[/*]"))) {
        int sp, ep;
        while (true) {
            if ((sp = ret.indexOf(QRegularExpression("[0-9]"))) == -1)
                break;
            if ((ep = ret.indexOf(QRegularExpression("[,-]"),sp)) == -1)
                ep = ret.length();
            int n = ret.mid(sp, ep - sp).toInt();
            ret = ret.replace(sp, ep - sp, upcaseHead(WeekNames.at(n)));
        }
    }
    return ret;
}

QString CronTime::toMonthLiteral(const QString &str)
{
    QString ret = str;
    if (!str.contains(QRegularExpression( "[/*]"))) {
        int sp, ep;
        while (true) {
            if ((sp = ret.indexOf(QRegularExpression("[0-9]"))) == -1)
                break;
            if ((ep = ret.indexOf(QRegularExpression("[,-]"),sp)) == -1)
                ep = ret.length();

            int n = ret.mid(sp, ep - sp).toInt() - 1;
            ret = ret.replace(sp, ep - sp, upcaseHead(MonthNames.at(n)));
        }
    }
    return ret;
}
