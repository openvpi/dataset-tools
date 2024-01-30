//
// Created by fluty on 2024/1/29.
//

#ifndef TRACKSVIEW_H
#define TRACKSVIEW_H

#include <QListWidget>

#include "Controls/TracksEditor/TracksGraphicsScene.h"
#include "Controls/TracksEditor/TracksGraphicsView.h"

class TracksView final : public QWidget {
    Q_OBJECT

public:
    explicit TracksView();

public slots:
    void onModelChanged(const DsModel &model);
    void onTempoChanged(double tempo);
    void onTrackChanged(DsModel::ChangeType type, const DsModel &model, int index);

signals:
    void selectedClipChanged(int trackIndex, int clipIndex);
    void trackPropertyChanged(const QString &name, const DsTrackControl &control, int index);
    void insertNewTrackTriggered(int index);
    void removeTrackTriggerd(int index);
    void tempoChanged(double tempo);

private slots:
    void onSceneSelectionChanged();

private:
    QListWidget *m_trackListWidget;
    TracksGraphicsView *m_graphicsView;
    TracksGraphicsScene *m_tracksScene;

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

    TracksViewModel m_tracksModel;
    double m_tempo = 120;

    void insertTrackToView(const DsTrack &dsTrack, int index);
    void removeTrackFromView(int index);
    void reset();
    void initGraphicsView();
};



#endif // TRACKSVIEW_H
