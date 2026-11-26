/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include "CronTime.h"

#include <QtCore>
#include <algorithm>
#include <ranges>

static const QStringList WeekNames = {"sun", "mon", "tue", "wed", "thu", "fri", "sat", "sun"};
static const QStringList MonthNames
    = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

inline QString upcaseHead(const QString &str)
{
    return str.at(0).toUpper() + str.mid(1);
}

CronTime::CronTime(const QString &tstr)
{
    bValid = false;
    QString str = tstr.toLower();
    if (!str.isEmpty() && str.at(0) == '@') {
        QString s = str.mid(1);
        if (s == QLatin1String("hourly")) {
            str = QStringLiteral("0 * * * *");
        } else if (s == QLatin1String("daily") || s == QLatin1String("midnight")) {
            str = QStringLiteral("0 0 * * *");
        } else if (s == QLatin1String("weekly")) {
            str = QStringLiteral("0 0 * * 0");
        } else if (s == QLatin1String("monthly")) {
            str = QStringLiteral("0 0 1 * *");
        } else if (s == QLatin1String("yearly") || s == QLatin1String("annually")) {
            str = QStringLiteral("0 0 1 1 *");
        } else if (s == QLatin1String("reboot")) {
            str = QStringLiteral("0 0 * * *");
        }
    }
    static const QRegularExpression separator(QStringLiteral("[ \\t]+"));
    QStringList slist = str.split(separator, Qt::SkipEmptyParts);

    if (slist.size() != 5) {
        qDebug() << "CronTime::CronTime Invalid Item count:" << slist.size();
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
    month = toBit(1, 12, slist.at(3), MonthNames);
    if (month.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Month Format:" << slist.at(3);
        return;
    }
    week = toBit(0, 8, slist.at(4), WeekNames);
    if (week.isEmpty()) {
        qDebug() << "CronTime::CronTime Invalid Week Format:" << slist.at(4);
        return;
    }
    if (week.at(7)) {
        week[0] = true;
    }
    if (week.at(0)) {
        week[7] = true;
    }

    bValid = true;
}

bool CronTime::isValid() const
{
    return bValid;
}

QString CronTime::toString(bool literal) const
{

    QString ret;
    if (literal) {
        ret = toString(minute, 0) + " " + toString(hour, 0) + " " + toString(day, 1) + " "
              + toMonthLiteral(toString(month, 1)) + " " + toWeekLiteral(toString(week, 0));
    } else {
        ret = toString(minute, 0) + " " + toString(hour, 0) + " " + toString(day, 1) + " " + toString(month, 1) + " "
              + toString(week, 0);
    }

    return ret;
}

QBitArray CronTime::toBit(int start, int num, const QString &field, const QStringList &names)
{
    static const QRegularExpression term(
        QStringLiteral(R"(\A(\*|[0-9]+|[a-z]{3})(?:-([0-9]+|[a-z]{3}))?(?:/([0-9]+))?\z)"));
    const auto endpoint = [&](const QString &token) {
        bool ok = false;
        const int value = token.toInt(&ok);
        if (ok) {
            return value - start;
        }
        return static_cast<int>(names.indexOf(token));
    };

    QBitArray bits(num);
    for (const QString &part : field.split(',')) {
        const auto match = term.match(part);
        if (!match.hasMatch()) {
            return {};
        }
        const bool wildcard = match.captured(1) == QLatin1String("*");
        const bool range = !match.captured(2).isEmpty();
        const bool stepped = !match.captured(3).isEmpty();
        if ((wildcard && range) || (stepped && !wildcard && !range)) {
            return {};
        }
        const int first = wildcard ? 0 : endpoint(match.captured(1));
        int last = wildcard ? num - 1 : first;
        if (range) {
            last = endpoint(match.captured(2));
        }
        const int step = stepped ? match.captured(3).toInt() : 1;
        if (first < 0 || last < first || last >= num || step < 1) {
            return {};
        }
        for (int i = first; i <= last; ++i) {
            if ((i - first) % step == 0) {
                bits.setBit(i);
            }
        }
    }
    return bits;
}

QString CronTime::toTimeString(int start, int cnt, int interval)
{

    if (cnt == 1) {
        return QStringLiteral("%1").arg(start);
    }

    if (cnt == 2) {
        return QStringLiteral("%1,%2").arg(start).arg(start + interval);
    }

    if (interval == 1) {
        return QStringLiteral("%1-%2").arg(start).arg(start + cnt - 1);
    }

    return QStringLiteral("%1-%2/%3").arg(start).arg(start + (cnt - 1) * interval).arg(interval);
}

QString CronTime::toString(const QBitArray &bit, int start)
{
    if (isFill(bit)) {
        return "*";
    }

    QStringList lst;
    int size = bit.size();
    if (size == 8) {
        size = 7;
    }
    int cnt = 0;
    int space_cnt = 0;
    int interval_cnt = 0;
    int first_pnt = 0;
    for (int i : std::views::iota(0, size)) {
        if (bit.at(i)) {
            if (cnt == 0) {
                first_pnt = i;
            } else if (cnt == 1) {
                interval_cnt = space_cnt;
            } else if (space_cnt != interval_cnt) {
                lst << toTimeString(first_pnt + start, cnt, interval_cnt + 1);
                first_pnt = i;
                interval_cnt = 0;
                cnt = 0;
            }
            cnt++;
            space_cnt = 0;
        } else {
            space_cnt++;
        }
    }
    if (first_pnt == 0 && cnt > 2 && ((interval_cnt + 1) * cnt) >= size) {
        lst << QStringLiteral("*/%1").arg(interval_cnt + 1);
    } else {
        lst << toTimeString(first_pnt + start, cnt, interval_cnt + 1);
    }

    QString ret;
    for (const QString &s : std::as_const(lst)) {
        if (!ret.isEmpty()) {
            ret += ',';
        }
        ret += s;
    }

    return ret;
}

bool CronTime::isFill(const QBitArray &bit)
{
    bool ret = true;
    for (int i : std::views::iota(0, bit.size())) {
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
QDateTime CronTime::getNextTime(const QDateTime &dtime) const
{
    QDateTime tm = dtime.addSecs(60 - dtime.time().second());
    QTime clear(0, 0);
    // Standard cron semantics: when both day-of-month and day-of-week are
    // restricted, the day matches if EITHER field matches. When only one is
    // restricted (the other is "*"), only that field applies; since a "*"
    // field always matches, AND of both fields covers those cases. These two
    // flags depend only on the (const) bit arrays, so compute them once.
    const bool dowRestricted = !isFill(week);
    const bool bothDayFieldsRestricted = !isFill(day) && dowRestricted;

    // Bail out early on schedules that can never fire. When day-of-week is
    // unrestricted, a match depends solely on the day-of-month landing inside a
    // selected month; if no selected day fits any selected month (e.g. "30 2 *",
    // February 30th), spinning to maxIterations would only waste CPU before
    // returning an invalid time. (When day-of-week is restricted, the OR rule
    // means a weekday match can still fire, so the schedule is satisfiable.)
    if (!dowRestricted) {
        static constexpr int maxDaysInMonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool possible = false;
        for (int m = 0; m < 12 && !possible; ++m) {
            if (!month.at(m)) {
                continue;
            }
            for (int d = 0; d < maxDaysInMonth[m]; ++d) {
                if (day.at(d)) {
                    possible = true;
                    break;
                }
            }
        }
        if (!possible) {
            return QDateTime();
        }
    }

    const int maxIterations = 10000;
    for (int i : std::views::iota(0, maxIterations)) {
        if (!month.at(tm.date().month() - 1)) {
            tm = tm.addMonths(1);
            tm.setTime(clear);
            tm.setDate(QDate(tm.date().year(), tm.date().month(), 1));
            continue;
        }

        const bool domMatch = day.at(tm.date().day() - 1);
        const bool dowMatch = week.at(tm.date().dayOfWeek());
        const bool dayMatch = bothDayFieldsRestricted ? (domMatch || dowMatch) : (domMatch && dowMatch);

        if (!dayMatch) {
            tm = tm.addDays(1);
            tm.setTime(clear);
            continue;
        } else if (!hour.at(tm.time().hour())) {
            tm = tm.addSecs(60 * (60 - tm.time().minute()));
            continue;
        } else if (!minute.at(tm.time().minute())) {
            tm = tm.addSecs(60);
            continue;
        }
        return tm;
    }
    return QDateTime();
}

QString CronTime::toWeekLiteral(const QString &str)
{
    QString ret = str;
    if (!str.contains(QRegularExpression(QStringLiteral("[/*]")))) {
        int sp = 0;
        int ep = 0;
        while (true) {
            if ((sp = ret.indexOf(QRegularExpression(QStringLiteral("[0-9]")))) == -1) {
                break;
            }
            if ((ep = ret.indexOf(QRegularExpression(QStringLiteral("[,-]")), sp)) == -1) {
                ep = ret.length();
            }
            int n = ret.mid(sp, ep - sp).toInt();
            ret = ret.replace(sp, ep - sp, upcaseHead(WeekNames.at(n)));
        }
    }
    return ret;
}

QString CronTime::toMonthLiteral(const QString &str)
{
    QString ret = str;
    if (!str.contains(QRegularExpression(QStringLiteral("[/*]")))) {
        int sp = 0;
        int ep = 0;
        while (true) {
            if ((sp = ret.indexOf(QRegularExpression(QStringLiteral("[0-9]")))) == -1) {
                break;
            }
            if ((ep = ret.indexOf(QRegularExpression(QStringLiteral("[,-]")), sp)) == -1) {
                ep = ret.length();
            }

            int n = ret.mid(sp, ep - sp).toInt() - 1;
            ret = ret.replace(sp, ep - sp, upcaseHead(MonthNames.at(n)));
        }
    }
    return ret;
}
