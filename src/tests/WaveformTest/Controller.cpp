//
// Created by FlutyDeer on 2023/12/1.
//

#include <QDebug>
// #include <QOpenGLWidget>

#include "Controller.h"
#include "Controls/Base/TimeGridGraphicsItem.h"
#include "Controls/PianoRoll/PianoRollBackgroundGraphicsItem.h"
#include "Controls/TracksEditor/AudioClipGraphicsItem.h"
#include "Controls/TracksEditor/SingingClipGraphicsItem.h"
#include "Controls/TracksEditor/TracksBackgroundGraphicsItem.h"

Controller::Controller() {
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
            &TimeGridGraphicsItem::setVisibleRect);
    connect(m_tracksView, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    m_tracksScene->addItem(gridItem);

    auto pianoRollScene = new PianoRollGraphicsScene;
    m_pianoRollView = new PianoRollGraphicsView;
    m_pianoRollView->setMinimumHeight(150);
    m_pianoRollView->setScene(pianoRollScene);
    m_pianoRollView->centerOn(0, 0);
    m_pianoRollView->setStyleSheet("QGraphicsView { border: none }");
    QObject::connect(m_pianoRollView, &TracksGraphicsView::scaleChanged, pianoRollScene, &TracksGraphicsScene::setScale);

    auto pianoGridItem = new PianoRollBackgroundGraphicsItem;
    QObject::connect(m_pianoRollView, &TracksGraphicsView::visibleRectChanged, pianoGridItem,
                     &TimeGridGraphicsItem::setVisibleRect);
    QObject::connect(m_pianoRollView, &TracksGraphicsView::scaleChanged, pianoGridItem, &TimeGridGraphicsItem::setScale);
    pianoRollScene->addItem(pianoGridItem);

    connect(&m_model, &DsModel::modelChanged, this, &Controller::onModelChanged);
    connect(this, &Controller::modelUpdated, m_pianoRollView, &PianoRollGraphicsView::updateView);

    // auto testSingingClip = new SingingClipGraphicsItem(0);
    // testSingingClip->setTrackIndex(0);
    // m_tracksScene->addItem(testSingingClip);
    // connect(m_tracksView, &TracksGraphicsView::scaleChanged, testSingingClip, &AbstractClipGraphicsItem::setScale);
    // connect(m_tracksView, &TracksGraphicsView::visibleRectChanged, testSingingClip, &AbstractClipGraphicsItem::setVisibleRect);
    // m_trackIndex++;

    auto filename = "E:/Test/Param/小小.json";
    m_model.loadAProject(filename);
}
Controller::~Controller() {
}
TracksGraphicsScene *Controller::tracksScene() const {
    return m_tracksScene;
}
TracksGraphicsView *Controller::tracksView() const {
    return m_tracksView;
}
PianoRollGraphicsView *Controller::pianoRollView() const {
    return m_pianoRollView;
}
int Controller::trackCount() const {
    return m_trackIndex;
}
void Controller::addAudioClipToNewTrack(const QString &filePath) {
    int start = 0;
    double gain = 1.0;
    int trackIndex = m_trackIndex;

    auto clip = new AudioClipGraphicsItem(m_clipItemIndex);
    clip->setStart(start);
    clip->setGain(gain);
    clip->setTrackIndex(trackIndex);
    clip->openFile(filePath);
    clip->setVisibleRect(m_tracksView->visibleRect());
    clip->setScaleX(m_tracksView->scaleX());
    clip->setScaleY(m_tracksView->scaleY());
    m_tracksScene->addItem(clip);
    connect(m_tracksView, &TracksGraphicsView::scaleChanged, clip, &AbstractClipGraphicsItem::setScale);
    connect(m_tracksView, &TracksGraphicsView::visibleRectChanged, clip, &AbstractClipGraphicsItem::setVisibleRect);

    m_trackIndex++;
    m_clipItemIndex++;
}
void Controller::onModelChanged() {
    qDebug() << "model changed";
    emit modelUpdated(m_model);
}