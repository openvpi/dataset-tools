//
// Created by fluty on 2023/11/14.
//

#include "TracksGraphicsView.h"
#include "AudioClipGraphicsItem.h"
#include "SingingClipGraphicsItem.h"
#include "TracksEditorGlobal.h"

TracksGraphicsView::TracksGraphicsView() {
    m_tracksScene = new TracksGraphicsScene;
    setScene(m_tracksScene);
    centerOn(0, 0);
    connect(this, &TracksGraphicsView::scaleChanged, m_tracksScene, &TracksGraphicsScene::setScale);

    auto gridItem = new TracksBackgroundGraphicsItem;
    gridItem->setPixelsPerQuarterNote(TracksEditorGlobal::pixelsPerQuarterNote);
    connect(this, &TracksGraphicsView::visibleRectChanged, gridItem, &TimeGridGraphicsItem::setVisibleRect);
    connect(this, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    m_tracksScene->addItem(gridItem);
}
void TracksGraphicsView::updateView(const DsModel &model) {
    if (m_tracksScene == nullptr)
        return;

    reset();
    for (const auto &track : model.tracks()) {
        for (const auto &clip : track.clips) {
            auto start = clip->start();
            auto clipStart = clip->clipStart();
            auto length = clip->length();
            auto clipLen = clip->clipLen();
            if (clip->type() == DsClip::Audio) {
                auto audioClip = clip.dynamicCast<DsAudioClip>();
                auto clipItem = new AudioClipGraphicsItem(m_trackCount);
                clipItem->setStart(start);
                clipItem->setClipStart(clipStart);
                clipItem->setLength(length);
                clipItem->setClipLen(clipLen);
                clipItem->setGain(1.0);
                clipItem->setTrackIndex(m_trackCount);
                clipItem->openFile(audioClip->path());
                clipItem->setVisibleRect(this->visibleRect());
                clipItem->setScaleX(this->scaleX());
                clipItem->setScaleY(this->scaleY());
                m_tracksScene->addItem(clipItem);
                connect(this, &TracksGraphicsView::scaleChanged, clipItem, &AudioClipGraphicsItem::setScale);
                connect(this, &TracksGraphicsView::visibleRectChanged, clipItem,
                        &AudioClipGraphicsItem::setVisibleRect);
                m_clips.append(clipItem);
            } else if (clip->type() == DsClip::Singing) {
                auto singingClip = clip.dynamicCast<DsSingingClip>();
                auto clipItem = new SingingClipGraphicsItem(m_trackCount);
                clipItem->setStart(start);
                clipItem->setClipStart(clipStart);
                clipItem->setLength(length);
                clipItem->setClipLen(clipLen);
                clipItem->setGain(1.0);
                clipItem->setTrackIndex(m_trackCount);
                clipItem->loadNotes(singingClip->notes);
                clipItem->setVisibleRect(this->visibleRect());
                clipItem->setScaleX(this->scaleX());
                clipItem->setScaleY(this->scaleY());
                m_tracksScene->addItem(clipItem);
                connect(this, &TracksGraphicsView::scaleChanged, clipItem, &SingingClipGraphicsItem::setScale);
                connect(this, &TracksGraphicsView::visibleRectChanged, clipItem,
                        &SingingClipGraphicsItem::setVisibleRect);
                m_clips.append(clipItem);
            }
        }
        m_trackCount++;
    }
}
void TracksGraphicsView::reset() {
    m_trackCount = 0;
    for (auto clip : m_clips) {
        m_tracksScene->removeItem(clip);
        delete clip;
    }
    m_clips.clear();
}