//
// Created by fluty on 2023/11/14.
//

#include <QDebug>

#include "AudioClipGraphicsItem.h"
#include "SingingClipGraphicsItem.h"
#include "TracksBackgroundGraphicsItem.h"
#include "TracksEditorGlobal.h"
#include "TracksGraphicsView.h"

TracksGraphicsView::TracksGraphicsView() {
    m_tracksScene = new TracksGraphicsScene;
    setScene(m_tracksScene);
    centerOn(0, 0);
    connect(this, &TracksGraphicsView::scaleChanged, m_tracksScene, &TracksGraphicsScene::setScale);
    connect(m_tracksScene, &TracksGraphicsScene::selectionChanged, this, &TracksGraphicsView::onSceneSelectionChanged);

    auto gridItem = new TracksBackgroundGraphicsItem;
    gridItem->setPixelsPerQuarterNote(TracksEditorGlobal::pixelsPerQuarterNote);
    connect(this, &TracksGraphicsView::visibleRectChanged, gridItem, &TimeGridGraphicsItem::setVisibleRect);
    connect(this, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    m_tracksScene->addItem(gridItem);
}
void TracksGraphicsView::onModelChanged(const DsModel &model) {
    qDebug() << "onModelChanged";
    if (m_tracksScene == nullptr)
        return;

    reset();
    m_tempo = model.tempo();
    int index = 0;
    for (const auto &track : model.tracks()) {
        insertTrack(track, index);
        index++;
    }
}
void TracksGraphicsView::onTempoChanged(double tempo) {
    // notify audio clips
    m_tempo = tempo;
    emit tempoChanged(tempo);
}
void TracksGraphicsView::onTracksChanged(DsModel::ChangeType type, const DsModel &model, int index) {
    switch (type) {
        case DsModel::Insert:
            qDebug() << "on track inserted" << index;
            insertTrack(model.tracks().at(index), index);
            break;
        case DsModel::Update:
            break;
        case DsModel::Remove:
            break;
    }
}
void TracksGraphicsView::onSceneSelectionChanged() {
    qDebug() << "onSceneSelectionChanged";
    // find selected clip (the first one)
    bool foundSelectedClip = false;
    for (int i = 0; i < m_tracksModel.tracks.count(); i++) {
        auto track = m_tracksModel.tracks.at(i);
        for (int j = 0; j < track.clips.count(); j++) {
            auto clip = track.clips.at(j);
            if (clip->isSelected()) {
                foundSelectedClip = true;
                emit selectedClipChanged(i, j);
                break;
            }
        }
        if (foundSelectedClip)
            break;
    }
    if (!foundSelectedClip)
        emit selectedClipChanged(-1, -1);
}
void TracksGraphicsView::insertTrack(const DsTrack &dsTrack, int index) {
    Track track;
    for (const auto &clip : dsTrack.clips) {
        auto start = clip->start();
        auto clipStart = clip->clipStart();
        auto length = clip->length();
        auto clipLen = clip->clipLen();
        if (clip->type() == DsClip::Audio) {
            auto audioClip = clip.dynamicCast<DsAudioClip>();
            auto clipItem = new AudioClipGraphicsItem(0);
            clipItem->setStart(start);
            clipItem->setClipStart(clipStart);
            clipItem->setLength(length);
            clipItem->setClipLen(clipLen);
            clipItem->setGain(1.0);
            clipItem->setTrackIndex(index);
            clipItem->setPath(audioClip->path());
            clipItem->setTempo(m_tempo);
            clipItem->setVisibleRect(visibleRect());
            clipItem->setScaleX(scaleX());
            clipItem->setScaleY(scaleY());
            m_tracksScene->addItem(clipItem);
            connect(this, &TracksGraphicsView::scaleChanged, clipItem, &AudioClipGraphicsItem::setScale);
            connect(this, &TracksGraphicsView::visibleRectChanged, clipItem, &AudioClipGraphicsItem::setVisibleRect);
            connect(this, &TracksGraphicsView::tempoChanged, clipItem, &AudioClipGraphicsItem::onTempoChange);
            track.clips.append(clipItem); // TODO: insert by clip start pos
        } else if (clip->type() == DsClip::Singing) {
            auto singingClip = clip.dynamicCast<DsSingingClip>();
            auto clipItem = new SingingClipGraphicsItem(0);
            clipItem->setStart(start);
            clipItem->setClipStart(clipStart);
            clipItem->setLength(length);
            clipItem->setClipLen(clipLen);
            clipItem->setGain(1.0);
            clipItem->setTrackIndex(index);
            clipItem->loadNotes(singingClip->notes);
            clipItem->setVisibleRect(visibleRect());
            clipItem->setScaleX(scaleX());
            clipItem->setScaleY(scaleY());
            m_tracksScene->addItem(clipItem);
            connect(this, &TracksGraphicsView::scaleChanged, clipItem, &SingingClipGraphicsItem::setScale);
            connect(this, &TracksGraphicsView::visibleRectChanged, clipItem, &SingingClipGraphicsItem::setVisibleRect);
            track.clips.append(clipItem);
        }
    }
    m_tracksModel.tracks.insert(index, track);
}
void TracksGraphicsView::reset() {
    for (auto &track : m_tracksModel.tracks)
        for (auto clip : track.clips) {
            m_tracksScene->removeItem(clip);
            delete clip;
        }
    m_tracksModel.tracks.clear();
}