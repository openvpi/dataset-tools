//
// Created by fluty on 2024/1/29.
//

#include "TracksView.h"

// #include <QSplitter>
#include <QDebug>
#include <QScroller>

#include "Controls/TracksEditor/AudioClipGraphicsItem.h"
#include "Controls/TracksEditor/SingingClipGraphicsItem.h"
#include "Controls/TracksEditor/TrackControlWidget.h"
#include "Controls/TracksEditor/TracksBackgroundGraphicsItem.h"
#include "Controls/TracksEditor/TracksEditorGlobal.h"

TracksView::TracksView() {
    m_trackListWidget = new QListWidget;
    // tracklist->setMinimumWidth(120);
    m_trackListWidget->setMinimumWidth(360);
    m_trackListWidget->setMaximumWidth(360);
    m_trackListWidget->setViewMode(QListView::ListMode);
    m_trackListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_trackListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    QScroller::grabGesture(m_trackListWidget, QScroller::TouchGesture);
    // m_trackListWidget->setStyleSheet("QListWidget::item{ height: 72px }");

    m_graphicsView = new TracksGraphicsView;
    QScroller::grabGesture(m_graphicsView, QScroller::TouchGesture);
    initGraphicsView();
    // auto splitter = new QSplitter;
    // splitter->setOrientation(Qt::Horizontal);
    // splitter->addWidget(tracklist);
    // splitter->addWidget(m_graphicsView);
    auto layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    // layout->addWidget(splitter);
    layout->addWidget(m_trackListWidget);
    layout->addWidget(m_graphicsView);
    setLayout(layout);
}
void TracksView::onModelChanged(const DsModel &model) {
    if (m_tracksScene == nullptr)
        return;

    reset();
    m_tempo = model.tempo();
    int index = 0;
    for (const auto &track : model.tracks()) {
        insertTrackToView(track, index);
        index++;
    }
}
void TracksView::onTempoChanged(double tempo) {
    // notify audio clips
    m_tempo = tempo;
    emit tempoChanged(tempo);
}
void TracksView::onTrackChanged(DsModel::ChangeType type, const DsModel &model, int index) {
    switch (type) {
        case DsModel::Insert:
            qDebug() << "on track inserted" << index;
            insertTrackToView(model.tracks().at(index), index);
            break;
        case DsModel::Update:
            // TODO: update view
            break;
        case DsModel::Remove:
            qDebug() << "on track removed" << index;
            removeTrackFromView(index);
            break;
    }
}
void TracksView::onSceneSelectionChanged() {
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
void TracksView::insertTrackToView(const DsTrack &dsTrack, int index) {
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
            clipItem->setVisibleRect(m_graphicsView->visibleRect());
            clipItem->setScaleX(m_graphicsView->scaleX());
            clipItem->setScaleY(m_graphicsView->scaleY());
            m_tracksScene->addItem(clipItem);
            connect(m_graphicsView, &TracksGraphicsView::scaleChanged, clipItem, &AudioClipGraphicsItem::setScale);
            connect(m_graphicsView, &TracksGraphicsView::visibleRectChanged, clipItem,
                    &AudioClipGraphicsItem::setVisibleRect);
            connect(this, &TracksView::tempoChanged, clipItem, &AudioClipGraphicsItem::onTempoChange);
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
            clipItem->setVisibleRect(m_graphicsView->visibleRect());
            clipItem->setScaleX(m_graphicsView->scaleX());
            clipItem->setScaleY(m_graphicsView->scaleY());
            m_tracksScene->addItem(clipItem);
            connect(m_graphicsView, &TracksGraphicsView::scaleChanged, clipItem, &SingingClipGraphicsItem::setScale);
            connect(m_graphicsView, &TracksGraphicsView::visibleRectChanged, clipItem,
                    &SingingClipGraphicsItem::setVisibleRect);
            track.clips.append(clipItem);
        }
    }
    auto newTrackItem = new QListWidgetItem;
    auto newTrackWidget = new TrackControlWidget;
    newTrackItem->setSizeHint(QSize(360, 72));
    newTrackWidget->setTrackIndex(index + 1);
    newTrackWidget->setName(dsTrack.name().isEmpty() ? " New Track": dsTrack.name());
    newTrackWidget->setControl(dsTrack.control());
    newTrackWidget->setFixedHeight(72);
    m_trackListWidget->insertItem(index, newTrackItem);
    m_trackListWidget->setItemWidget(newTrackItem, newTrackWidget);

    connect(newTrackWidget, &TrackControlWidget::propertyChanged, this, [=] {
        auto name = newTrackWidget->name();
        auto control = newTrackWidget->control();
        auto i = m_trackListWidget->row(newTrackItem);
        emit trackPropertyChanged(name, control, index);
    });
    connect(newTrackWidget, &TrackControlWidget::insertNewTrackTriggered, this, [=] {
        auto i = m_trackListWidget->row(newTrackItem);
        emit insertNewTrackTriggered(i + 1); // insert after current track
    });
    connect(newTrackWidget, &TrackControlWidget::removeTrackTriggerd, this, [=] {
        auto i = m_trackListWidget->row(newTrackItem);
        emit removeTrackTriggerd(i);
    });
    m_tracksModel.tracks.insert(index, track);
    if (index < m_tracksModel.tracks.count()) // needs to update existed tracks' index
        for (int i = index + 1; i < m_tracksModel.tracks.count(); i++) {
            // Update track list items' index
            auto item = m_trackListWidget->item(i);
            auto widget = m_trackListWidget->itemWidget(item);
            auto trackWidget = dynamic_cast<TrackControlWidget *>(widget);
            trackWidget->setTrackIndex(i + 1);
            // Update clips' index
            for (auto &clipItem : m_tracksModel.tracks.at(i).clips) {
                clipItem->setTrackIndex(i);
            }
        }
}
void TracksView::removeTrackFromView(int index) {
    // remove from view
    auto track = m_tracksModel.tracks.at(index);
    for (auto clip : track.clips) {
        m_tracksScene->removeItem(clip);
        delete clip;
    }
    auto item = m_trackListWidget->takeItem(index);
    m_trackListWidget->removeItemWidget(item);
    // remove from viewmodel
    m_tracksModel.tracks.removeAt(index);
    //update index
    if (index < m_tracksModel.tracks.count()) // needs to update existed tracks' index
        for (int i = index; i < m_tracksModel.tracks.count(); i++) {
            // Update track list items' index
            auto item = m_trackListWidget->item(i);
            auto widget = m_trackListWidget->itemWidget(item);
            auto trackWidget = dynamic_cast<TrackControlWidget *>(widget);
            trackWidget->setTrackIndex(i + 1);
            // Update clips' index
            for (auto &clipItem : m_tracksModel.tracks.at(i).clips) {
                clipItem->setTrackIndex(i);
            }
        }
}
void TracksView::reset() {
    for (auto &track : m_tracksModel.tracks)
        for (auto clip : track.clips) {
            m_tracksScene->removeItem(clip);
            delete clip;
        }
    m_trackListWidget->clear();
    m_tracksModel.tracks.clear();
}
void TracksView::initGraphicsView() {
    m_tracksScene = new TracksGraphicsScene;
    m_graphicsView->setScene(m_tracksScene);
    m_graphicsView->centerOn(0, 0);
    connect(m_graphicsView, &TracksGraphicsView::scaleChanged, m_tracksScene, &TracksGraphicsScene::setScale);
    connect(m_tracksScene, &TracksGraphicsScene::selectionChanged, this, &TracksView::onSceneSelectionChanged);

    auto gridItem = new TracksBackgroundGraphicsItem;
    gridItem->setPixelsPerQuarterNote(TracksEditorGlobal::pixelsPerQuarterNote);
    connect(m_graphicsView, &TracksGraphicsView::visibleRectChanged, gridItem, &TimeGridGraphicsItem::setVisibleRect);
    connect(m_graphicsView, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    m_tracksScene->addItem(gridItem);
}