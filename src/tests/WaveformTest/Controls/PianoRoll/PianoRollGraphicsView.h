//
// Created by fluty on 2024/1/23.
//

#ifndef PIANOROLLGRAPHICSVIEW_H
#define PIANOROLLGRAPHICSVIEW_H

#include "Controls/Base/CommonGraphicsView.h"
#include "Model/DsModel.h"

class PianoRollGraphicsView final : public CommonGraphicsView {
    Q_OBJECT

public:
    explicit PianoRollGraphicsView();

public slots:
    void updateView(const DsModel &model);
};

#endif // PIANOROLLGRAPHICSVIEW_H
