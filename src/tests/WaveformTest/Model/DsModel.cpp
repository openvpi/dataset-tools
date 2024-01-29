//
// Created by fluty on 2024/1/27.
//

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "DsModel.h"

double DsModel::tempo() const {
    return m_tempo;
}
void DsModel::setTempo(double tempo) {
    m_tempo = tempo;
    emit tempoChanged(m_tempo);
}
QList<DsTrack> DsModel::tracks() const {
    return m_tracks;
}
void DsModel::insertTrack(const DsTrack &track, int index) {
    m_tracks.insert(index, track);
    emit tracksChanged(Insert, *this, index);
}
void DsModel::removeTrack(int index) {
    m_tracks.removeAt(index);
    emit tracksChanged(Remove, *this, index);
}
bool DsModel::loadAProject(const QString &filename) {
    reset();

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
            note.setPronunciation("la");
            notes.append(note);
        }
        return notes;
    };

    auto decodeClips = [&](const QJsonArray &arrClips, QList<DsClipPtr> &clips, const QString &type) {
        for (const auto &valClip : qAsConst(arrClips)) {
            auto objClip = valClip.toObject();
            if (type == "sing") {
                auto singingClip = DsSingingClipPtr(new DsSingingClip);
                singingClip->setName("Clip");
                singingClip->setStart(objClip.value("pos").toInt());
                singingClip->setClipStart(objClip.value("clipPos").toInt());
                singingClip->setLength(objClip.value("dur").toInt());
                singingClip->setClipLen(objClip.value("clipDur").toInt());
                auto arrNotes = objClip.value("notes").toArray();
                singingClip->notes.append(decodeNotes(arrNotes));
                clips.append(singingClip);
            } else if (type == "audio") {
                auto audioClip = DsAudioClipPtr(new DsAudioClip);
                audioClip->setName("Clip");
                audioClip->setStart(objClip.value("pos").toInt());
                audioClip->setClipStart(objClip.value("clipPos").toInt());
                audioClip->setLength(objClip.value("dur").toInt());
                audioClip->setClipLen(objClip.value("clipDur").toInt());
                audioClip->setPath(objClip.value("path").toString());
                clips.append(audioClip);
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
        m_tempo = objAProject.value("tempos").toArray().first().toObject().value("bpm").toDouble();
        decodeTracks(objAProject.value("tracks").toArray(), m_tracks);
        // auto clip = tracks().first().clips.first().dynamicCast<DsSingingClip>();
        // qDebug() << clip->notes.count();
        runG2p();
        emit modelChanged(*this);
        return true;
    }
    return false;
}
void DsModel::onTrackUpdated(int index) {
    emit tracksChanged(Update, *this, index);
}
void DsModel::onSelectedClipChanged(int trackIndex, int clipIndex) {
    m_selectedClipTrackIndex = trackIndex;
    m_selectedClipIndex = clipIndex;
    emit selectedClipChanged(*this, m_selectedClipTrackIndex, m_selectedClipIndex);
}
void DsModel::reset() {
    m_tracks.clear();
}
void DsModel::runG2p() {
    for (const auto &track : m_tracks) {
        for (const auto &clip : track.clips) {
            if (clip->type() == DsClip::Singing) {
                auto singingClip = clip.dynamicCast<DsSingingClip>();
            }
        }
    }
}