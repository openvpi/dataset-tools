//
// Created by fluty on 2024/1/22.
//

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

#include "SingingClipGraphicsItem.h"

SingingClipGraphicsItem::SingingClipGraphicsItem(int itemId, QGraphicsItem *parent) : ClipGraphicsItem(itemId, parent) {
    m_clipTypeStr = "[Singing] ";
    m_canResizeLength = true;
    auto loadProjectFile = [](const QString &filename, QJsonObject *jsonObj) {
        QFile loadFile(filename);
        if (!loadFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open project file";
            return false;
        }
        QByteArray allData = loadFile.readAll();
        loadFile.close();
        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(allData, &err);
        if (err.error != QJsonParseError::NoError)
            return false;
        if (json.isObject()) {
            *jsonObj = json.object();
        }
        return true;
    };

    auto loadNotes = [](const QJsonObject &obj) {
        auto arrTracks = obj.value("tracks").toArray();
        auto firstTrack = arrTracks.first().toObject();
        auto arrPatterns = firstTrack.value("patterns").toArray();
        auto firstPattern = arrPatterns.first().toObject();
        auto notes = firstPattern.value("notes").toArray();

        auto decodeNotes = [](const QJsonArray &arrNotes) {
            QVector<Note> notes;
            for (const auto valNote : qAsConst(arrNotes)) {
                auto objNote = valNote.toObject();
                Note note;
                note.start = objNote.value("pos").toInt();
                note.length = objNote.value("dur").toInt();
                note.keyIndex = objNote.value("pitch").toInt();
                note.lyric = objNote.value("lyric").toString();
                notes.append(note);
            }
            return notes;
        };

        return decodeNotes(notes);
    };

    auto filename = "E:/Test/Param/小小.json";
    QJsonObject jsonObj;
    if (loadProjectFile(filename, &jsonObj)) {
        notes = loadNotes(jsonObj);
        auto endTick = notes.last().start + notes.last().length;
        setLength(endTick);
        setClipLen(endTick);
    }
}
QString SingingClipGraphicsItem::audioCachePath() const {
    return m_audioCachePath;
}
void SingingClipGraphicsItem::setAudioCachePath(const QString &path) {
    m_audioCachePath = path;
}
void SingingClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    ClipGraphicsItem::paint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    auto rectLeft = previewRect().left();
    auto rectTop = previewRect().top();
    auto rectWidth = previewRect().width();
    auto rectHeight = previewRect().height();

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
    for (const auto &note : notes) {
        auto keyIndex = note.keyIndex;
        if (keyIndex < lowestKeyIndex)
            lowestKeyIndex = keyIndex;
        if (keyIndex > highestKeyIndex)
            highestKeyIndex = keyIndex;
    }

    int divideCount = highestKeyIndex - lowestKeyIndex + 1;
    auto noteHeight = (rectHeight - rectTop) / divideCount;

    auto tickToSceneX = [&](const double tick) { return tick * m_scaleX * pixelPerQuarterNote / 480; };
    auto sceneXToItemX = [&](const double x) { return mapFromScene(QPointF(x, 0)).x(); };

    for (const auto &note : notes) {
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