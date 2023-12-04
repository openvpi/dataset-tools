//
// Created by FlutyDeer on 2023/12/1.
//

#ifndef TRACKSCONTROLLER_H
#define TRACKSCONTROLLER_H

#include <QObject>

#include "TracksGraphicsScene.h"
#include "TracksGraphicsView.h"
#include "TracksModel.h"

class TracksController final : public QObject {
    Q_OBJECT

public:
    explicit TracksController();
    ~TracksController() override;

    TracksGraphicsScene *tracksScene() const;
    TracksGraphicsView *tracksView() const;

    int trackCount() const;

public slots:
    // void addTrack();
    void addAudioClipToNewTrack(const QString &filePath);

private slots:
    void onStartChanged(int clipId, int start);

private:
    // Model
    TracksModel m_tracksModel;

    // Views
    TracksGraphicsScene *m_tracksScene;
    TracksGraphicsView *m_tracksView;

    // test
    int m_trackIndex = 0;
    int m_clipItemIndex = 0;
};



#endif //TRACKSCONTROLLER_H
