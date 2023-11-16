//
// Created by fluty on 2023/11/14.
//
#include <QDebug>
#include <QScrollBar>
#include <QWheelEvent>

#include "TracksGraphicsView.h"
TracksGraphicsView::TracksGraphicsView(QWidget *parent) {
    setRenderHint(QPainter::Antialiasing);
    // setCacheMode(QGraphicsView::CacheNone);
    // setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}
TracksGraphicsView::~TracksGraphicsView() {
}
void TracksGraphicsView::setScaleX(const qreal sx) {
    emit scaleChanged(m_scaleX, m_scaleY);
}
void TracksGraphicsView::setScaleY(const qreal sy) {
    emit scaleChanged(m_scaleX, m_scaleY);
}
void TracksGraphicsView::wheelEvent(QWheelEvent *event) {
    // auto cursorPosF = event->position();
    // auto cursorPos = QPoint(cursorPosF.x(), cursorPosF.y());
    // auto scenePos = mapToScene(cursorPos);
    //
    // auto viewWidth = viewport()->width();
    // auto viewHeight = viewport()->height();
    //
    // auto hScale = cursorPosF.x() / viewWidth;
    // auto vScale = cursorPosF.y() / viewHeight;

    auto deltaY = event->angleDelta().y();

    if (event->modifiers() == Qt::ControlModifier) {
        if (deltaY > 0)
            m_scaleX = m_scaleX * 1.2;
        else if (deltaY < 0)
            m_scaleX = m_scaleX / 1.2;
        setScaleX(m_scaleX);

        // auto viewPoint = transform().map((scenePos));
        // horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        // verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else if (event->modifiers() == Qt::ShiftModifier) {
        if (deltaY > 0)
            m_scaleY = m_scaleY * 1.2;
        else if (deltaY < 0)
            m_scaleY = m_scaleY / 1.2;
        setScaleY(m_scaleY);

        // auto viewPoint = transform().map((scenePos));
        // horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        // verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else {
        QGraphicsView::wheelEvent(event);
    }
}
void TracksGraphicsView::drawBackground(QPainter *painter, const QRectF &rect) {
    // QPen pen;
    // pen.setWidthF(1);
    //
    // auto scaleX = m_scaleX;
    // auto notePenWidth = 2.0f;
    //
    // auto left = rect.left() * scaleX;
    // auto top = rect.top();
    // auto width = rect.width() * scaleX;
    // auto height = rect.height();
    // auto scaledRect = QRectF(left, top, width, height);
    //
    // const int pixelPerQuarterNote = 64;
    // auto scaledQuarterNoteWidth = scaleX * pixelPerQuarterNote;
    // auto sceneWidth = scene()->width() * scaleX;
    // auto sceneHeight = scene()->height();
    //
    // //    int tick = 0;
    // int curBar = 1;
    // int curBeat = 1;
    // int num = 3;
    // int deno = 4;
    // for (qreal i = 0; i <= sceneWidth; i += scaledQuarterNoteWidth) {
    //     pen.setColor(QColor(210, 210, 210));
    //     painter->setPen(pen);
    //     painter->drawLine(QPointF(i, 0), QPointF(i, sceneHeight));
    //
    //     // draw 1/8 note line
    //     auto x = i + scaledQuarterNoteWidth / 2;
    //     painter->drawLine(QPointF(x, 0), QPointF(x, sceneHeight));
    //
    //     pen.setColor(curBeat == 1 ? QColor(89, 89, 89) : QColor(160, 160, 160));
    //     painter->setPen(pen);
    //     painter->drawText(QPointF(i, 10 + top), QString::number(curBar) + "." + QString::number(curBeat));
    //     //        tick += 480;
    //     curBeat += 1;
    //     if (curBeat > num) {
    //         curBar++;
    //         curBeat = 1;
    //     }
    // }
    QGraphicsView::drawBackground(painter, rect);
}
bool TracksGraphicsView::eventFilter(QObject *object, QEvent *event) {
    return QAbstractScrollArea::eventFilter(object, event);
}
void TracksGraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
}
