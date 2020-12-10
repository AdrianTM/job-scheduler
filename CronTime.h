/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#ifndef CRONTIME_H
#define CRONTIME_H

#include <QDateTime>
#include <QBitArray>

class CronTime
{
public:
    CronTime(const QString &tstr);

    QDateTime getNextTime(const QDateTime &dtime);
    bool isValid();
    QString toString(bool literal = false);

    QBitArray minute;
    QBitArray hour;
    QBitArray day;
    QBitArray month;
    QBitArray week;
    static bool isFill(const QBitArray &bit);

private:
    bool bValid;

    QBitArray toBit(int start, int num, const QString &str);
    QString toString(const QBitArray &bit, int start);
    QString toTimeString(int first_pnt, int cnt, int iterval);
    QString toWeekLiteral(const QString &str);
    QString toMonthLiteral(const QString &str);

};

#endif
