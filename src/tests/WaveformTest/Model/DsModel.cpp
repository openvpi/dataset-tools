//
// Created by fluty on 2024/1/27.
//

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "DsModel.h"

bool DsModel::loadAProject(const QString &filename) {
    auto openJsonFile = [](const QString &filename, QJsonObject *jsonObj) {
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

    auto decodeNotes = [](const QJsonArray &arrNotes) {
        QList<DsNote> notes;
        for (const auto valNote : qAsConst(arrNotes)) {
            auto objNote = valNote.toObject();
            DsNote note;
            note.setStart(objNote.value("pos").toInt());
            note.setLength(objNote.value("dur").toInt());
            note.setKeyIndex(objNote.value("pitch").toInt());
            note.setLyric(objNote.value("lyric").toString());
            notes.append(note);
        }
        return notes;
    };

    auto decodeClips = [&](const QJsonArray &arrClips, QList<DsClipPtr> &clips, const QString &type) {
        for (const auto &valClip : qAsConst(arrClips)) {
            auto objClip = valClip.toObject();
            // auto clip = DsClipPtr(new DsClip);
            // clip->setName("Clip");
            // clip->setStart(objClip.value("pos").toInt());
            // clip->setClipStart(objClip.value("clipPos").toInt());
            // clip->setLength(objClip.value("dur").toInt());
            // clip->setClipLen(objClip.value("clipDur").toInt());

            if (type == "sing") {
                // auto singingClip = clip.dynamicCast<DsSingingClip>();
                auto singingClip = DsSingingClipPtr(new DsSingingClip);
                singingClip->setName("Clip");
                singingClip->setStart(objClip.value("pos").toInt());
                singingClip->setClipStart(objClip.value("clipPos").toInt());
                singingClip->setLength(objClip.value("dur").toInt());
                singingClip->setClipLen(objClip.value("clipDur").toInt());
                auto arrNotes = objClip.value("notes").toArray();
                singingClip->notes.append(decodeNotes(arrNotes));
                clips.append(singingClip);
            }
        }
    };

    auto decodeTracks = [&](const QJsonArray &arrTracks, QList<DsTrack> &tracks) {
        for (const auto &valTrack : qAsConst(arrTracks)) {
            auto objTrack = valTrack.toObject();
            auto type = objTrack.value("type").toString();
            DsTrack track;
            decodeClips(objTrack.value("patterns").toArray(), track.clips, type);
            tracks.append(track);
        }
    };

    QJsonObject objAProject;
    if (openJsonFile(filename, &objAProject)) {
        numerator = objAProject.value("beatsPerBar").toInt();
        decodeTracks(objAProject.value("tracks").toArray(), tracks);
        emit modelChanged();
        return true;
    }
    return false;
}