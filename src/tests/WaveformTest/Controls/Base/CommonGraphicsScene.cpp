//
// Created by fluty on 2024/1/23.
//

#include "CommonGraphicsScene.h"

CommonGraphicsScene::CommonGraphicsScene() {
    setSceneRect(0, 0, m_sceneSize.width(), m_sceneSize.height());
}
QSizeF CommonGraphicsScene::sceneSize() const {
    return m_sceneSize;
}
void CommonGraphicsScene::setSceneSize(const QSizeF &size) {
    m_sceneSize = size;
    updateSceneRect();
}

void CommonGraphicsScene::setScale(qreal sx, qreal sy) {
    m_scaleX = sx;
    m_scaleY = sy;
    updateSceneRect();
}
void CommonGraphicsScene::updateSceneRect()  {
    auto scaledWidth = m_sceneSize.width() * m_scaleX;
    auto scaledHeight = m_sceneSize.height() * m_scaleY;
    setSceneRect(0, 0, scaledWidth, scaledHeight);
}