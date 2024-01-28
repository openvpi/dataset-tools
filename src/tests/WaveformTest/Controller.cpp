//
// Created by FlutyDeer on 2023/12/1.
//

#include <QDebug>
// #include <QOpenGLWidget>

#include "Controller.h"
#include "Controls/Base/TimeGridGraphicsItem.h"
#include "Controls/TracksEditor/AudioClipGraphicsItem.h"
#include "Controls/TracksEditor/SingingClipGraphicsItem.h"
#include "Controls/TracksEditor/TracksBackgroundGraphicsItem.h"

Controller::Controller() {
    // Init views
    m_tracksView = new TracksGraphicsView;
    m_pianoRollView = new PianoRollGraphicsView;

    connect(&m_model, &DsModel::modelChanged, this, &Controller::onModelChanged);
    connect(this, &Controller::modelUpdated, m_tracksView, &TracksGraphicsView::updateView);
    connect(this, &Controller::modelUpdated, m_pianoRollView, &PianoRollGraphicsView::updateView);

}
Controller::~Controller() {
}
TracksGraphicsView *Controller::tracksView() const {
    return m_tracksView;
}
PianoRollGraphicsView *Controller::pianoRollView() const {
    return m_pianoRollView;
}
void Controller::openProject(const QString &filePath) {
    m_model.loadAProject(filePath);
}
void Controller::addAudioClipToNewTrack(const QString &filePath) {
    auto audioClip = DsAudioClipPtr(new DsAudioClip);
    audioClip->setPath(filePath);
    DsTrack newTrack;
    newTrack.clips.append(audioClip);
    m_model.addTrack(newTrack);
}
void Controller::onModelChanged() {
    qDebug() << "model changed";
    emit modelUpdated(m_model);
}