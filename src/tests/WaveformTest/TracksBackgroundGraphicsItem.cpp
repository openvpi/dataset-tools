//
// Created by fluty on 2024/1/22.
//

#include <QPainter>

#include "TracksBackgroundGraphicsItem.h"

void TracksBackgroundGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    TimeGridGraphicsItem::paint(painter, option, widget);

    // Draw track divider
    auto lineColor = QColor(72, 75, 78);
    auto penWidth = 1;

    QPen pen;
    pen.setWidthF(penWidth);
    pen.setColor(lineColor);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);

    auto sceneYToTrackIndex = [&](const double y) {
        return y / m_scaleY / trackHeight;
    };

    auto trackIndexToSceneY = [&](const double index) {
        return index * m_scaleY * trackHeight;
    };

    auto sceneYToItemY = [&](const double y) {
        return mapFromScene(QPointF(0, y)).y();
    };

    auto startTrackIndex = sceneYToTrackIndex(m_visibleRect.top());
    auto endTrackIndex = sceneYToTrackIndex(m_visibleRect.bottom());
    auto prevLineTrackIndex = static_cast<int>(startTrackIndex);
    for (int i = prevLineTrackIndex; i < endTrackIndex; i++) {
        auto y = sceneYToItemY(trackIndexToSceneY(i));
        auto line = QLineF(0, y, m_visibleRect.width(), y);
        painter->drawLine(line);
    }
}