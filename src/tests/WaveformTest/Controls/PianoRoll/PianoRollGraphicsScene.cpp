//
// Created by fluty on 2024/1/23.
//

#include "PianoRollGraphicsScene.h"

#include "PianoRollGlobal.h"

using namespace PianoRollGlobal;

PianoRollGraphicsScene::PianoRollGraphicsScene() {
    auto w = 1920 / 480 * pixelsPerQuarterNote * 100;
    auto h = 128 * noteHeight;
    setSceneSize(QSizeF(w, h));
}