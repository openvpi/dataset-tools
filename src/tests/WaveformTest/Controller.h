//
// Created by FlutyDeer on 2023/12/1.
//

#ifndef TRACKSCONTROLLER_H
#define TRACKSCONTROLLER_H

#include <QObject>

#include "Controls/PianoRoll/PianoRollGraphicsView.h"
#include "Controls/TracksEditor/TracksGraphicsView.h"
#include "Model/DsModel.h"
#include "Views/TracksView.h"

class Controller final : public QObject {
    Q_OBJECT

public:
    explicit Controller();
    ~Controller() override;

    TracksView *tracksView() const;
    PianoRollGraphicsView *pianoRollView() const;

public slots:
    void onNewTrack();
    void openProject(const QString &filePath);
    void addAudioClipToNewTrack(const QString &filePath);
    void onSelectedClipChanged(int trackIndex, int clipIndex);
    void onTrackPropertyChanged(const QString &name, const DsTrackControl &control, int index);

private:
    // Model
    DsModel m_model;

    // Views
    TracksView *m_tracksView;
    PianoRollGraphicsView *m_pianoRollView;
};



#endif // TRACKSCONTROLLER_H
