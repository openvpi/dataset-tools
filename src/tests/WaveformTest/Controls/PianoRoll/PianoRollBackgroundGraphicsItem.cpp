//
// Created by fluty on 2024/1/24.
//

#include <QPainter>

#include "PianoRollBackgroundGraphicsItem.h"
#include "PianoRollGlobal.h"

using namespace PianoRollGlobal;

void PianoRollBackgroundGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                            QWidget *widget) {
    // Draw background
    auto backgroundColor = QColor(42, 43, 44);
    painter->setBrush(backgroundColor);
    painter->drawRect(boundingRect());

    auto lineColor = QColor(57, 59, 61);
    auto keyIndexTextColor = QColor(160, 160, 160);
    auto penWidth = 1;

    QPen pen;
    pen.setWidthF(penWidth);
    pen.setColor(lineColor);
    painter->setPen(pen);
    // painter->setRenderHint(QPainter::Antialiasing);

    auto sceneYToKeyIndex = [&](const double y) { return 127 - y / scaleY() / noteHeight; };

    auto keyIndexToSceneY = [&](const double index) { return (127 - index) * scaleY() * noteHeight; };

    auto sceneYToItemY = [&](const double y) { return mapFromScene(QPointF(0, y)).y(); };

    auto isWhiteKey = [](const int midiKey) -> bool {
        int index = midiKey % 12;
        bool pianoKeys[] = {true, false, true, false, true, true, false, true, false, true, false, true};
        return pianoKeys[index];
    };

    auto toNoteName = [](const int &midiKey) {
        QString noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int index = qAbs(midiKey) % 12;
        int octave = midiKey / 12 - 1;
        QString noteName = noteNames[index] + QString::number(octave);
        return noteName;
    };

    auto startKeyIndex = sceneYToKeyIndex(visibleRect().top());
    auto endKeyIndex = sceneYToKeyIndex(visibleRect().bottom());
    auto prevKeyIndex = static_cast<int>(startKeyIndex) + 1;
    for (int i = prevKeyIndex; i > endKeyIndex; i--) {
        auto y = sceneYToItemY(keyIndexToSceneY(i));

        painter->setBrush(isWhiteKey(i) ? QColor(42, 43, 44) : QColor(35, 36, 37));
        auto gridRect = QRectF(0, y, visibleRect().width(), noteHeight * scaleY());
        painter->setPen(Qt::NoPen);
        painter->drawRect(gridRect);

        pen.setColor(keyIndexTextColor);
        painter->setPen(pen);
        painter->drawText(gridRect, toNoteName(i), QTextOption(Qt::AlignVCenter));

        pen.setColor(lineColor);
        painter->setPen(pen);
        auto line = QLineF(0, y, visibleRect().width(), y);
        painter->drawLine(line);
    }

    TimeGridGraphicsItem::paint(painter, option, widget);
}