//
// Created by FlutyDeer on 2023/12/1.
//

#include <QDebug>
// #include <QOpenGLWidget>

#include "AudioClipGraphicsItem.h"
#include "TimeGridGraphicsItem.h"
#include "TracksBackgroundGraphicsItem.h"
#include "TracksController.h"

#include "SingingClipGraphicsItem.h"
#include "TracksModel.h"

TracksController::TracksController() {
    // Create scene and view
    m_tracksScene = new TracksGraphicsScene;
    m_tracksView = new TracksGraphicsView;
    // m_tracksView->setViewport(new QOpenGLWidget());
    m_tracksView->setMinimumHeight(150);
    m_tracksView->setScene(m_tracksScene);
    m_tracksView->centerOn(0, 0);
    m_tracksView->setStyleSheet("QGraphicsView { border: none }");

    connect(m_tracksView, &TracksGraphicsView::scaleChanged, m_tracksScene, &TracksGraphicsScene::setScale);

    auto gridItem = new TracksBackgroundGraphicsItem;
    connect(m_tracksView, &TracksGraphicsView::visibleRectChanged, gridItem,
            &TimeGridGraphicsItem::onVisibleRectChanged);
    connect(m_tracksView, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    m_tracksScene->addItem(gridItem);

    auto testSingingClip = new SingingClipGraphicsItem(0);
    testSingingClip->setTrackIndex(0);
    m_tracksScene->addItem(testSingingClip);
    connect(m_tracksView, &TracksGraphicsView::scaleChanged, testSingingClip, &ClipGraphicsItem::setScale);
    connect(m_tracksView, &TracksGraphicsView::visibleRectChanged, testSingingClip, &ClipGraphicsItem::setVisibleRect);
    m_trackIndex++;
}
TracksController::~TracksController() {
}
TracksGraphicsScene *TracksController::tracksScene() const {
    return m_tracksScene;
}
TracksGraphicsView *TracksController::tracksView() const {
    return m_tracksView;
}
int TracksController::trackCount() const {
    return m_trackIndex;
}
void TracksController::addAudioClipToNewTrack(const QString &filePath) {
    int start = 0;
    double gain = 1.0;
    int trackIndex = m_trackIndex;

    auto trackModel = new TracksModel::Track;
    trackModel->setName("New Track");
    auto clipModel = new TracksModel::AudioClip;
    clipModel->setStart(0);
    clipModel->setGain(gain);
    clipModel->setPath(filePath);
    m_tracksModel.tracks.append(*trackModel);

    auto clip = new AudioClipGraphicsItem(m_clipItemIndex);
    clip->setStart(start);
    clip->setGain(gain);
    clip->setTrackIndex(trackIndex);
    clip->openFile(filePath);
    clip->setVisibleRect(m_tracksView->visibleRect());
    clip->setScaleX(m_tracksView->scaleX());
    clip->setScaleY(m_tracksView->scaleY());
    // clip->setScale(m_tracksView->scaleX(), m_tracksView->scaleY());
    m_tracksScene->addItem(clip);
    connect(m_tracksView, &TracksGraphicsView::scaleChanged, clip, &ClipGraphicsItem::setScale);
    connect(m_tracksView, &TracksGraphicsView::visibleRectChanged, clip, &ClipGraphicsItem::setVisibleRect);
    connect(clip, &ClipGraphicsItem::startChanged, this, &TracksController::onStartChanged);

    m_trackIndex++;
    m_clipItemIndex++;
}
void TracksController::onStartChanged(int clipId, int start) {
    // qDebug() << "Clip start changed to" << start;
    // for (auto i = m_tracksModel.tracks.begin(); i !=  m_tracksModel.tracks.end(); ++i) {
    //     auto track = *i;
    //     qDebug() << track.name();
    //     for (auto  j = track.clips.begin(); j != track.clips.end(); ++j) {
    //         auto clip = *j;
    //         if (clip->clipId == clipId) {
    //             // clip->setStart(start);
    //             qDebug() << "model changed";
    //             break;
    //         }
    //     }
    //     break;
    // }
}