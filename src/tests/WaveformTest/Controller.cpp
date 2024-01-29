//
// Created by FlutyDeer on 2023/12/1.
//

#include <QDebug>

#include "Controller.h"

Controller::Controller() {
    // Init views
    m_tracksView = new TracksGraphicsView;
    m_pianoRollView = new PianoRollGraphicsView;

    connect(&m_model, &DsModel::modelChanged, m_tracksView, &TracksGraphicsView::onModelChanged);
    connect(&m_model, &DsModel::tracksChanged, m_tracksView, &TracksGraphicsView::onTracksChanged);
    connect(&m_model, &DsModel::modelChanged, m_pianoRollView, &PianoRollGraphicsView::updateView);
    connect(m_tracksView, &TracksGraphicsView::selectedClipChanged, &m_model, &DsModel::onSelectedClipChanged);
    connect(&m_model, &DsModel::selectedClipChanged, m_pianoRollView, &PianoRollGraphicsView::onSelectedClipChanged);
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
    m_model.insertTrack(newTrack, m_model.tracks().count());
}