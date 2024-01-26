//
// Created by fluty on 2024/1/22.
//

#include <QPainter>

#include "TracksBackgroundGraphicsItem.h"
#include "TracksEditorGlobal.h"

using namespace TracksEditorGlobal;

void TracksBackgroundGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw background
    auto backgroundColor = QColor(42, 43, 44);
    painter->setBrush(backgroundColor);
    painter->drawRect(boundingRect());

    TimeGridGraphicsItem::paint(painter, option, widget);

    // Draw track divider
    auto lineColor = QColor(72, 75, 78);
    auto penWidth = 1;

    QPen pen;
    pen.setWidthF(penWidth);
    pen.setColor(lineColor);
    painter->setPen(pen);
    // painter->setRenderHint(QPainter::Antialiasing);

    auto sceneYToTrackIndex = [&](const double y) { return y / scaleY() / trackHeight; };

    auto trackIndexToSceneY = [&](const double index) { return index * scaleY() * trackHeight; };

    auto sceneYToItemY = [&](const double y) { return mapFromScene(QPointF(0, y)).y(); };

    auto startTrackIndex = sceneYToTrackIndex(visibleRect().top());
    auto endTrackIndex = sceneYToTrackIndex(visibleRect().bottom());
    auto prevLineTrackIndex = static_cast<int>(startTrackIndex);
    for (int i = prevLineTrackIndex; i < endTrackIndex; i++) {
        auto y = sceneYToItemY(trackIndexToSceneY(i));
        auto line = QLineF(0, y, visibleRect().width(), y);
        painter->drawLine(line);
    }
}