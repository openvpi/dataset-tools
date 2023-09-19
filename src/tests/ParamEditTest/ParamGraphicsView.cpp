//
// Created by fluty on 2023/9/16.
//

#include <QDebug>
#include <QWheelEvent>
#include <QScrollBar>

#include "ParamGraphicsView.h"

void ParamGraphicsView::wheelEvent(QWheelEvent *event) {
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
        scale(scaleX, scaleY);

        auto viewPoint = transform().map((scenePos));
        horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else if (event->modifiers() == Qt::ShiftModifier) {
        if (deltaY > 0)
            scaleY = 1.2;
        else if (deltaY < 0)
            scaleY = 1.0 / 1.2;
        scale(scaleX, scaleY);

        auto viewPoint = transform().map((scenePos));
        horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ParamGraphicsView::drawBackground(QPainter *painter, const QRectF &rect) {
    QPen pen;
    pen.setWidthF(1);

    auto scaleX = painter->transform().m11();
    auto scaleY = painter->transform().m22();
    auto notePenWidth = 2.0f;

    auto left = rect.left() * scaleX;
    auto top = rect.top() * scaleY;
    auto width = rect.width() * scaleX;
    auto height = rect.height() * scaleY;
    auto scaledRect = QRectF(left, top, width, height);

    const int quarterNoteHeight = 24;
    auto scaledQuarterNoteHeight = scaleY * quarterNoteHeight;
    const int quarterNoteWidth = 62;
    auto scaledQuarterNoteWidth = scaleX * quarterNoteWidth;
    auto sceneWidth = scene()->width() * scaleX;
    auto sceneHeight = scene()->height() * scaleY;

    painter->scale(1.0 / scaleX, 1.0 / scaleY);

    auto gridHeight = scaledQuarterNoteHeight;
    int keyMin = 36;
    int keyMax = 107;
    for (int i = keyMax; i >= keyMin; i--) {
        pen.setColor(QColor(240, 240, 240));
        painter->setBrush(isWhiteKey(i)
                              ? QColor(255, 255, 255)
                              : QColor(240, 240, 240));
        QRectF gridRect = QRectF(left, (keyMax - i) * gridHeight, width, gridHeight);
        painter->setPen(pen);
        painter->drawRect(gridRect);

        pen.setColor(QColor(89, 89, 89));
        painter->setPen(pen);
        painter->drawText(gridRect, toNoteName(i), QTextOption(Qt::AlignVCenter));
    }

//    int tick = 0;
    int curBar = 1;
    int curBeat = 1;
    int num = 3;
    int deno = 4;
    for (qreal i = 0; i <= sceneWidth; i += scaledQuarterNoteWidth) {
        pen.setColor(QColor(210, 210, 210));
        painter->setPen(pen);
        painter->drawLine(QPointF(i, 0), QPointF(i, sceneHeight));
        pen.setColor(curBeat == 1 ? QColor(89, 89, 89) : QColor(160, 160, 160));
        painter->setPen(pen);
        painter->drawText(QPointF(i, 10 + top), QString::number(curBar) + "." + QString::number(curBeat));
//        tick += 480;
        curBeat += 1;
        if (curBeat > num) {
            curBar++;
            curBeat = 1;
        }
    }

    QGraphicsView::drawBackground(painter, rect);
}

bool ParamGraphicsView::isWhiteKey(const int &midiKey) {
    int index = midiKey % 12;
    bool pianoKeys[] = {true, false, true, false, true, true, false, true, false, true, false, true};
    return pianoKeys[index];
}

QString ParamGraphicsView::toNoteName(const int &midiKey) {
    QString noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int index = midiKey % 12;
    int octave = midiKey / 12 - 1;
    QString noteName = noteNames[index] + QString::number(octave);
    return noteName;
}

ParamGraphicsView::ParamGraphicsView(QWidget *parent) {
    viewport()->installEventFilter(this);
    setDragMode(QGraphicsView::RubberBandDrag);
//    m_timer.setInterval(32);
//    QTimer::connect(&m_timer, &QTimer::timeout, this, [&]() {
//        setBackgroundBrush(QBrush());
//    });
//    m_timer.start();
}

bool ParamGraphicsView::eventFilter(QObject *object, QEvent *event) {
    return QAbstractScrollArea::eventFilter(object, event);
}