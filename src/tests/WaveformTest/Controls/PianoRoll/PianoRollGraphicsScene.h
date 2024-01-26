//
// Created by fluty on 2024/1/23.
//

#ifndef PIANOROLLGRAPHICSSCENE_H
#define PIANOROLLGRAPHICSSCENE_H

#include "Controls/Base/CommonGraphicsScene.h"

class PianoRollGraphicsScene final : public CommonGraphicsScene {
public:
    explicit PianoRollGraphicsScene();

private:
    const double noteHeight = 24;
};

#endif // PIANOROLLGRAPHICSSCENE_H
