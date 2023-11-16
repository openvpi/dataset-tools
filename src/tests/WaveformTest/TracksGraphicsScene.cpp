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
TracksGraphicsScene::~TracksGraphicsScene() {
}
void TracksGraphicsScene::setScale(qreal sx, qreal sy) {
    auto scaledWidth = m_sceneWidth * sx;
    auto scaledHeight = m_sceneHeight * sy;
    setSceneRect(0, 0, scaledWidth, scaledHeight);
    for (const auto item : items()){
        const auto clip = dynamic_cast<ClipGraphicsItem *>(item);
        if (clip != nullptr){
            clip->setScaleX(sx);
            clip->setScaleY(sy);
        }
    }
}
void TracksGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // for (auto item : items()){
    //     auto note = dynamic_cast<ClipGraphicsItem *>(item);
    //     if (note != nullptr){
    //         qDebug() << note->pos();
    //     }
    // }

    QGraphicsScene::mousePressEvent(event);
}
