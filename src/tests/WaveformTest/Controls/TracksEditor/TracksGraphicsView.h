//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
#define DATASET_TOOLS_TRACKSGRAPHICSVIEW_H

#include "../Base/CommonGraphicsView.h"
#include "AbstractClipGraphicsItem.h"
#include "Model/DsModel.h"
#include "TracksBackgroundGraphicsItem.h"
#include "TracksGraphicsScene.h"

class TracksGraphicsView final : public CommonGraphicsView {
public:
    explicit TracksGraphicsView();

public slots:
    void updateView(const DsModel &model);

private:
    TracksGraphicsScene *m_tracksScene;
    int m_trackCount = 0;
    QVector<AbstractClipGraphicsItem *> m_clips;

    void reset();
};

#endif // DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
