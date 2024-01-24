//
// Created by fluty on 2024/1/23.
//

#include "PianoRollGraphicsScene.h"
PianoRollGraphicsScene::PianoRollGraphicsScene() {
    m_sceneHeight = 128 * noteHeight;
    setSceneRect(0, 0, m_sceneWidth, m_sceneHeight);
}