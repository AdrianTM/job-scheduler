/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#pragma once

#include <QBitArray>
#include <QDateTime>

class CronTime
{
public:
    explicit CronTime(const QString &tstr);

    [[nodiscard]] QDateTime getNextTime(const QDateTime &dtime) const;
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString toString(bool literal = false) const;

    QBitArray minute;
    QBitArray hour;
    QBitArray day;
    QBitArray month;
    QBitArray week;
    static bool isFill(const QBitArray &bit);

private:
    bool bValid;

    static QBitArray toBit(int start, int num, const QString &str);
    static QString toString(const QBitArray &bit, int start);
    static QString toTimeString(int start, int cnt, int interval);
    static QString toWeekLiteral(const QString &str);
    static QString toMonthLiteral(const QString &str);
};
