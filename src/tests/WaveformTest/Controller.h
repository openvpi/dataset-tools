//
// Created by FlutyDeer on 2023/12/1.
//

#ifndef TRACKSCONTROLLER_H
#define TRACKSCONTROLLER_H

#include <QObject>

#include "Controls/PianoRoll/PianoRollGraphicsScene.h"
#include "Controls/PianoRoll/PianoRollGraphicsView.h"
#include "Controls/TracksEditor/TracksGraphicsScene.h"
#include "Controls/TracksEditor/TracksGraphicsView.h"
#include "Model/DsModel.h"

class Controller final : public QObject {
    Q_OBJECT

public:
    explicit Controller();
    ~Controller() override;

    TracksGraphicsScene *tracksScene() const;
    TracksGraphicsView *tracksView() const;
    PianoRollGraphicsView *pianoRollView() const;

    int trackCount() const;

signals:
    void modelUpdated(const DsModel &model);

public slots:
    // void addTrack();
    void addAudioClipToNewTrack(const QString &filePath);
    void onModelChanged();

private:
    // Model
    DsModel m_model;

    // Views
    TracksGraphicsScene *m_tracksScene;
    TracksGraphicsView *m_tracksView;
    PianoRollGraphicsView *m_pianoRollView;

    // test
    int m_trackIndex = 0;
    int m_clipItemIndex = 0;
};



#endif // TRACKSCONTROLLER_H
