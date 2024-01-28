//
// Created by fluty on 2024/1/23.
//

#include "PianoRollGraphicsView.h"

#include "NoteGraphicsItem.h"
#include "PianoRollBackgroundGraphicsItem.h"
#include "PianoRollGlobal.h"
#include "PianoRollGraphicsScene.h"

PianoRollGraphicsView::PianoRollGraphicsView() {
    setScaleXMax(5);
    setDragMode(RubberBandDrag);

    m_pianoRollScene = new PianoRollGraphicsScene;
    setScene(m_pianoRollScene);
    connect(this, &PianoRollGraphicsView::scaleChanged, m_pianoRollScene, &PianoRollGraphicsScene::setScale);

    auto gridItem = new PianoRollBackgroundGraphicsItem;
    gridItem->setPixelsPerQuarterNote(PianoRollGlobal::pixelsPerQuarterNote);
    connect(this, &PianoRollGraphicsView::visibleRectChanged, gridItem, &TimeGridGraphicsItem::setVisibleRect);
    connect(this, &PianoRollGraphicsView::scaleChanged, gridItem, &PianoRollBackgroundGraphicsItem::setScale);
    m_pianoRollScene->addItem(gridItem);

    // auto pitchItem = new PitchEditorGraphicsItem;
    // QObject::connect(pianoRollView, &TracksGraphicsView::visibleRectChanged, pitchItem,
    //                  &PitchEditorGraphicsItem::setVisibleRect);
    // QObject::connect(pianoRollView, &TracksGraphicsView::scaleChanged, pitchItem, &PitchEditorGraphicsItem::setScale);
    // pianoRollScene->addItem(pitchItem);
}
void PianoRollGraphicsView::updateView(const DsModel &model) {
    reset();

    auto clip = model.tracks().first().clips.first().dynamicCast<DsSingingClip>();
    for (const auto &note : clip->notes) {
        auto noteItem = new NoteGraphicsItem(m_noteId);
        noteItem->setStart(note.start());
        noteItem->setLength(note.length());
        noteItem->setKeyIndex(note.keyIndex());
        noteItem->setLyric(note.lyric());
        noteItem->setPronunciation(note.pronunciation());
        noteItem->setVisibleRect(this->visibleRect());
        noteItem->setScaleX(this->scaleX());
        noteItem->setScaleY(this->scaleY());
        m_pianoRollScene->addItem(noteItem);
        connect(this, &PianoRollGraphicsView::scaleChanged, noteItem, &NoteGraphicsItem::setScale);
        connect(this, &PianoRollGraphicsView::visibleRectChanged, noteItem, &NoteGraphicsItem::setVisibleRect);
        m_noteItems.append(noteItem);
        if (m_noteId == 0)
            this->centerOn(noteItem);
        m_noteId++;
    }
}
void PianoRollGraphicsView::reset() {
    m_noteId = 0;
    for (auto note : m_noteItems) {
        m_pianoRollScene->removeItem(note);
        delete note;
    }
    m_noteItems.clear();
}