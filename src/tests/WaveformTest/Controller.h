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

    TracksGraphicsView *tracksView() const;
    PianoRollGraphicsView *pianoRollView() const;

signals:
    void modelUpdated(const DsModel &model);

public slots:
    // void addTrack();
    void openProject(const QString &filePath);
    void addAudioClipToNewTrack(const QString &filePath);
    void onModelChanged();

private:
    // Model
    DsModel m_model;

    // Views
    TracksGraphicsView *m_tracksView;
    PianoRollGraphicsView *m_pianoRollView;
};



#endif // TRACKSCONTROLLER_H
