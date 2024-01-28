//
// Created by fluty on 2023/11/14.
//

#include "TracksGraphicsScene.h"
#include "TracksEditorGlobal.h"

using namespace TracksEditorGlobal;

TracksGraphicsScene::TracksGraphicsScene() {
    setSceneSize(QSizeF(1920 / 480 * pixelsPerQuarterNote * 100, 2000));
}