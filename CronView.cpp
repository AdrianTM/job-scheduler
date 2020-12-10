/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QtGui>
#include <QScrollBar>

#include "Crontab.h"
#include "CronModel.h"
#include "CronView.h"

void dumpIndex(const QModelIndex &idx, QString h)
{

    if (idx.isValid()) {
        qDebug() << h << ": row=" << idx.row() << " column=" << idx.column();
        dumpIndex(idx.parent(), h + " parent");
    } else {
        qDebug() << h << ": invalid";
    }

}

CronView::CronView(CronModel *model)
    : QTreeView()
{

    cronModel = model;
    pasteData = nullptr;
    setModel(cronModel);
    setDragEnabled(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAcceptDrops(true);


    //	setDropIndicatorShown(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &CronView::selectChanged);
    connect(cronModel, &CronModel::moveTCommand, this, &CronView::TCommandMoved);
}

void CronView::resetView()
{

    clearSelection();
    reset();

    setRootIsDecorated(!cronModel->isOneUser());

    for (int i = 0; i < cronModel->rowCount(QModelIndex()); ++i)
        setExpanded(cronModel->index(i, 0), true);

    for (int i = 0; i < cronModel->columnCount(QModelIndex()); ++i)
        resizeColumnToContents(i);

    QModelIndex idx = cronModel->index(0,0);

    if (idx.isValid())
        setCurrentIndex(idx);
    else
        emit viewSelected(getCurrentCrontab(), nullptr);

}

void CronView::selectChanged(const QModelIndex &cur, const QModelIndex &)
{
    emit viewSelected(cronModel->getCrontab(cur), cronModel->getTCommand(cur));
}

void CronView::tCommandChanged()
{
    cronModel->tCommandChanged(currentIndex());
    //	for (int i = 0; i < cronModel->columnCount(QModelIndex()); ++i)
    //		resizeColumnToContents(i);
}

void CronView::removeTCommand()
{
    QModelIndex index = currentIndex();


    QModelIndex next = cronModel->removeCComand(index);
    if (next.isValid())
        setCurrentIndex(next);
    else
        emit viewSelected(getCurrentCrontab(), nullptr);

    for (int i = 0; i < cronModel->columnCount(QModelIndex()); ++i)
        resizeColumnToContents(i);

    emit dataChanged();

}

void CronView::insertTCommand(TCommand *cmnd)
{
    QModelIndex index = currentIndex();

    QModelIndex next = cronModel->insertTCommand(index, cmnd);
    //	dumpIndex(next, "insert next");
    setCurrentIndex(next);

    for (int i = 0; i < cronModel->columnCount(QModelIndex()); ++i)
        resizeColumnToContents(i);

    emit dataChanged();
}

void CronView::copyTCommand()
{
    if (!pasteData)
        pasteData = new TCommand();
    *pasteData = *getCurrentTCommand();
    emit pasted();
}

void CronView::cutTCommand()
{
    copyTCommand();
    removeTCommand();
}

void CronView::newTCommand()
{
    Crontab *cron = getCurrentCrontab();
    QString u;
    if ( cron->cronOwner == "/etc/crontab" )
        u = "root";
    else
        u = cron->cronOwner;

    TCommand *cmnd =  new TCommand("0 * * * *", u, "", "", cron );
    insertTCommand(cmnd);
}

void CronView::pasteTCommand()
{
    Crontab *cron = getCurrentCrontab();
    TCommand *cmnd = new TCommand;
    *cmnd = *pasteData;
    if (cron->cronOwner != "/etc/crontab")
        cmnd->user = cron->cronOwner;

    cmnd->parent = cron;
    insertTCommand(cmnd);
}

void CronView::changeCurrent(TCommand *cmnd)
{
    if (cmnd != nullptr) {
        QModelIndex idx = cronModel->searchTCommand(cmnd);
        if (idx.isValid())
            setCurrentIndex(idx);
    }
}
void CronView::TCommandMoved(TCommand *cmnd)
{
    changeCurrent(cmnd);
    emit dataChanged();
}

Crontab *CronView::getCurrentCrontab()
{
    return cronModel->getCrontab(currentIndex());
}

TCommand *CronView::getCurrentTCommand()
{
    return cronModel->getTCommand(currentIndex());
}

void CronView::scrollTo(const QModelIndex &idx, ScrollHint)
{
    QRect rect = visualRect(idx);
    if (rect.height() == 0)
        return;
    QRect area = viewport()->rect();
    double step = static_cast<double>(verticalStepsPerItem()) / rect.height();
    if (rect.top() < 0)
        verticalScrollBar()->setValue(
                    verticalScrollBar()->value() + static_cast<int>(rect.top() * step) );
    else if (rect.bottom() > area.bottom())
        verticalScrollBar()->setValue(
                    verticalScrollBar()->value() +
                    static_cast<int>((rect.bottom() - area.bottom()) * step) + 5);
}

void CronView::startDrag(Qt::DropActions supportedActions)
{
    cronModel->dragTCommand(currentIndex());

    QTreeView::startDrag(supportedActions);
}

