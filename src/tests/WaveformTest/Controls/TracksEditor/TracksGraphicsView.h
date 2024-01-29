//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
#define DATASET_TOOLS_TRACKSGRAPHICSVIEW_H

#include "../Base/CommonGraphicsView.h"
#include "AbstractClipGraphicsItem.h"
#include "Model/DsModel.h"
#include "TracksGraphicsScene.h"

class TracksGraphicsView final : public CommonGraphicsView {
    Q_OBJECT

public:
    explicit TracksGraphicsView();

public slots:
    void onModelChanged(const DsModel &model);
    void onTempoChanged(double tempo);
    void onTracksChanged(DsModel::ChangeType type, const DsModel &model, int index);

signals:
    void selectedClipChanged(int trackIndex, int clipIndex);
    void tempoChanged(double tempo);

private slots:
    void onSceneSelectionChanged();

private:
    class Track {
    public:
        // properties
        bool isSelected;
        // clips
        QVector<AbstractClipGraphicsItem *> clips;
    };

    class TracksViewModel {
    public:
        QList<Track> tracks;
    };

    TracksGraphicsScene *m_tracksScene;
    TracksViewModel m_tracksModel;
    double m_tempo = 120;

    void insertTrack(const DsTrack &dsTrack, int index);
    void reset();
};

#endif // DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
