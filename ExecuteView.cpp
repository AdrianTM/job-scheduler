/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/
#include <QHeaderView>
#include <QScrollBar>

#include "Execute.h"
#include "ExecuteView.h"
#include "ExecuteModel.h"

ExecuteView::ExecuteView(ExecuteModel *model)
    : executeModel(model)
{
    setModel(executeModel);

    header()->setSortIndicatorShown(true);
    //header()->setClickable(true);
    setRootIsDecorated(false);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &ExecuteView::selectChanged);
}

void ExecuteView::resetView()
{
    //	reset();
    resizeColumnToContents( 0 );
    resizeColumnToContents( 1 );
    resizeColumnToContents( 2 );
    resizeColumnToContents( 3 );
}


void ExecuteView::selectChanged(const QModelIndex &cur, const QModelIndex&)
{
    if (cur.isValid()) {
        Execute *e = executeModel->getExecute(cur);
        emit viewSelected(e->tCommands);
    }
}

void ExecuteView::scrollTo(const QModelIndex &idx, ScrollHint)
{
    QRect area = viewport()->rect();
    QRect rect = visualRect(idx);
    if (rect.height() == 0) return;
    double step = static_cast<double>(verticalStepsPerItem()) / rect.height();
    if (rect.top() < 0)
        verticalScrollBar()->setValue(
                    verticalScrollBar()->value() + static_cast<int>(rect.top() * step) );
    else if (rect.bottom() > area.bottom())
        verticalScrollBar()->setValue(
                    verticalScrollBar()->value() +
                    static_cast<int>((rect.bottom() - area.bottom()) * step) + 5);
}
