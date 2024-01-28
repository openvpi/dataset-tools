//
// Created by fluty on 2024/1/23.
//

#include "PianoRollGraphicsScene.h"
#include "PianoRollGlobal.h"

using namespace PianoRollGlobal;

PianoRollGraphicsScene::PianoRollGraphicsScene() {
    m_sceneHeight = 128 * noteHeight;
    setSceneRect(0, 0, 1920 / 480 * pixelsPerQuarterNote * 100, m_sceneHeight);
}