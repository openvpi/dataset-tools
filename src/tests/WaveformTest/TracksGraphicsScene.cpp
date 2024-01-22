//
// Created by fluty on 2023/11/14.
//

#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include "ClipGraphicsItem.h"
#include "TracksGraphicsScene.h"

TracksGraphicsScene::TracksGraphicsScene() {
    setSceneRect(0, 0, m_sceneWidth, m_sceneHeight);
}
void TracksGraphicsScene::setScale(qreal sx, qreal sy) {
    auto scaledWidth = m_sceneWidth * sx;
    auto scaledHeight = m_sceneHeight * sy;
    setSceneRect(0, 0, scaledWidth, scaledHeight);
}