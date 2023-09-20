//
// Created by fluty on 2023/9/19.
//

#include <QWheelEvent>
#include <QScrollBar>
#include <QDebug>
#include <QGraphicsItem>

#include "ParamEditGraphicsView.h"

ParamEditGraphicsView::ParamEditGraphicsView(QWidget *parent) {
    setRenderHint(QPainter::Antialiasing);
//    setCacheMode(QGraphicsView::CacheNone);
//    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ParamEditGraphicsView::wheelEvent(QWheelEvent *event) {
    auto cursorPosF = event->position();
    auto cursorPos = QPoint(cursorPosF.x(), cursorPosF.y());
    auto scenePos = mapToScene(cursorPos);

    auto viewWidth = viewport()->width();
    auto viewHeight = viewport()->height();

    auto hScale = cursorPosF.x() / viewWidth;
    auto vScale = cursorPosF.y() / viewHeight;

    auto deltaY = event->angleDelta().y();
    qreal scaleX = 1;
    qreal scaleY = 1;

    if (event->modifiers() == Qt::ControlModifier) {
        if (deltaY > 0)
            scaleX = 1.2;
        else if (deltaY < 0)
            scaleX = 1.0 / 1.2;
//        scale(scaleX, scaleY);

        auto viewPoint = transform().map((scenePos));
        horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else if (event->modifiers() == Qt::ShiftModifier) {
        if (deltaY > 0)
            scaleY = 1.2;
        else if (deltaY < 0)
            scaleY = 1.0 / 1.2;
//        scale(scaleX, scaleY);

        auto viewPoint = transform().map((scenePos));
        horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ParamEditGraphicsView::drawBackground(QPainter *painter, const QRectF &rect) {
    QGraphicsView::drawBackground(painter, rect);
}

bool ParamEditGraphicsView::eventFilter(QObject *object, QEvent *event) {
    return QAbstractScrollArea::eventFilter(object, event);
}

void ParamEditGraphicsView::resizeEvent(QResizeEvent *event) {
    auto hBarHeight = horizontalScrollBar()->height();

    auto rectHeight = rect().height() - hBarHeight;
    auto availableRect = QRect(rect().left(), rect().top(),
                               sceneRect().width(), rect().height());
    setSceneRect(availableRect);
    QGraphicsView::resizeEvent(event);
}
