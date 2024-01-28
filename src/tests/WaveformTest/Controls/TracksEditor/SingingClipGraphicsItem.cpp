//
// Created by fluty on 2024/1/22.
//

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

#include "SingingClipGraphicsItem.h"
#include "TracksEditorGlobal.h"

using namespace TracksEditorGlobal;

SingingClipGraphicsItem::SingingClipGraphicsItem(int itemId, QGraphicsItem *parent)
    : AbstractClipGraphicsItem(itemId, parent) {
    setCanResizeLength(true);
    // setName("New Pattern");
}
void SingingClipGraphicsItem::loadNotes(const QList<DsNote> &notes) {
    m_notes.clear();
    for (const auto &dsNote : notes) {
        Note note;
        note.start = dsNote.start();
        note.length = dsNote.length();
        note.keyIndex = dsNote.keyIndex();
        m_notes.append(note);
    }
    auto endTick = m_notes.last().start + m_notes.last().length + 1920;
    setLength(endTick);
    setClipLen(endTick);
    update();
}
QString SingingClipGraphicsItem::audioCachePath() const {
    return m_audioCachePath;
}
void SingingClipGraphicsItem::setAudioCachePath(const QString &path) {
    m_audioCachePath = path;
}
void SingingClipGraphicsItem::drawPreviewArea(QPainter *painter, const QRectF &previewRect, int opacity) {
    painter->setRenderHint(QPainter::Antialiasing, false);

    auto rectLeft = previewRect.left();
    auto rectTop = previewRect.top();
    auto rectWidth = previewRect.width();
    auto rectHeight = previewRect.height();

    if (rectHeight < 32 || rectWidth < 16)
        return;
    auto widthHeightMin = rectWidth < rectHeight ? rectWidth : rectHeight;
    auto colorAlpha = rectHeight <= 48 ? 255 * (rectHeight - 32) / (48 - 32) : 255;
    auto noteColor = QColor(10, 10, 10, static_cast<int>(colorAlpha));

    painter->setPen(Qt::NoPen);
    painter->setBrush(noteColor);

    // find lowest and highest pitch
    int lowestKeyIndex = 127;
    int highestKeyIndex = 0;
    for (const auto &note : m_notes) {
        auto keyIndex = note.keyIndex;
        if (keyIndex < lowestKeyIndex)
            lowestKeyIndex = keyIndex;
        if (keyIndex > highestKeyIndex)
            highestKeyIndex = keyIndex;
    }

    int divideCount = highestKeyIndex - lowestKeyIndex + 1;
    auto noteHeight = (rectHeight - rectTop) / divideCount;

    for (const auto &note : m_notes) {
        auto clipLeft = start() + clipStart();
        auto clipRight = clipLeft + clipLen();
        if (note.start + note.length < clipLeft)
            continue;
        if (note.start >= clipRight)
            break;

        auto leftScene = tickToSceneX(note.start);
        auto left = sceneXToItemX(leftScene);
        auto width = tickToSceneX(note.length);
        if (note.start < clipLeft) {
            left = sceneXToItemX(tickToSceneX(clipLeft));
            width = sceneXToItemX(tickToSceneX(note.start + note.length)) - left;
            // qDebug() << left << width << note.lyric;
        } else if (note.start + note.length >= clipRight)
            width = tickToSceneX(clipRight - note.start);
        auto top = -(note.keyIndex - highestKeyIndex) * noteHeight + rectTop;
        painter->drawRect(QRectF(left, top, width, noteHeight));
    }
}