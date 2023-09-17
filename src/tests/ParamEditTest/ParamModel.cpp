//
// Created by fluty on 2023/9/5.
//

#include <QJsonArray>
#include <QDebug>

#include "ParamModel.h"

ParamModel::ParamModel() {
}

ParamModel::~ParamModel() {
}

ParamModel ParamModel::load(const QJsonObject &obj, bool *ok) {
    ParamModel model;

    auto arrTracks = obj.value("tracks").toArray();
    auto firstTrack = arrTracks.first().toObject();
    auto arrPatterns = firstTrack.value("patterns").toArray();
    auto firstPattern = arrPatterns.first().toObject();
    auto notes = firstPattern.value("notes").toArray();
    auto params = firstPattern.value("parameters").toObject();
    auto realBreathParams = params.value("realBreathiness").toArray();
    auto firstRealBreathParam = realBreathParams.first().toObject();

    auto decodeNotes = [](const QJsonArray &arrNotes) {
        QList<Note> notes;
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

    auto decodeRealParam = [](const QJsonObject &obj) {
        RealParam param;
        param.offset = obj.value("offset").toInt();
        auto arrValues = obj.value("values").toArray();
        for (const auto objValue : qAsConst(arrValues)) {
            auto value = objValue.toString().toDouble();
            param.values.append(value);
        }
        return param;
    };

    model.notes = decodeNotes(notes);
    model.realBreathiness = decodeRealParam(firstRealBreathParam);

    return model;
}
