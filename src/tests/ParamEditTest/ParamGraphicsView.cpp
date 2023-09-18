//
// Created by fluty on 2023/9/16.
//

#include <QDebug>
#include <QWheelEvent>

#include "ParamGraphicsView.h"

void ParamGraphicsView::wheelEvent(QWheelEvent *event) {
    auto deltaY = event->angleDelta().y();
    qreal scaleX = 1;
    qreal scaleY = 1;

    if (event->modifiers() == Qt::ControlModifier) {
        if (deltaY > 0)
            scaleX = 1.2;
        else if (deltaY < 0)
            scaleX = 1.0 / 1.2;
    }

    if (event->modifiers() == Qt::ShiftModifier) {
        if (deltaY > 0)
            scaleY = 1.2;
        else if (deltaY < 0)
            scaleY = 1.0 / 1.2;
    }

    scale(scaleX, scaleY);
    QGraphicsView::wheelEvent(event);
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

    auto right = left + width;
    auto bottom = top + height;

    const int quarterNoteHeight = 24;
    auto scaledQuarterNoteHeight = scaleY * quarterNoteHeight;

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

    QGraphicsView::drawBackground(painter, rect);
}

bool ParamGraphicsView::isWhiteKey(const int &midiKey) {
    int index = midiKey % 12;
    bool pianoKeys[] = {true, false, true, false, true, true, false, true, false, true, false, true};
    return pianoKeys[index];
}

QString ParamGraphicsView::toNoteName(const int &midiKey) {
    QString noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int index = midiKey % 12;
    int octave = midiKey / 12 - 1;
    QString noteName = noteNames[index] + QString::number(octave);
    return noteName;
}

ParamGraphicsView::ParamGraphicsView(QWidget *parent) {
    m_timer.setInterval(16);
    QTimer::connect(&m_timer, &QTimer::timeout, this, [&]() {
        setBackgroundBrush(QBrush());
    });
    m_timer.start();
}
