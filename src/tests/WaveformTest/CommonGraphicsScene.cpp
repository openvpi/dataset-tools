//
// Created by fluty on 2024/1/23.
//

#include "CommonGraphicsScene.h"

CommonGraphicsScene::CommonGraphicsScene() {
    setSceneRect(0, 0, m_sceneWidth, m_sceneHeight);
}

void CommonGraphicsScene::setScale(qreal sx, qreal sy) {
    auto scaledWidth = m_sceneWidth * sx;
    auto scaledHeight = m_sceneHeight * sy;
    setSceneRect(0, 0, scaledWidth, scaledHeight);
}