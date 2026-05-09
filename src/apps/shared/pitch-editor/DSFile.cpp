#include "DSFile.h"

#include <dsfw/JsonHelper.h>
#include <dstools/CurveTools.h>
#include <QStringList>

#include <cmath>

namespace dstools {
namespace pitchlabeler {

DSFile::DSFile() = default;

void DSFile::loadFromJson(const nlohmann::json &data) {
    offset = secToUs(DsDocument::numberOrString(data, "offset", 0.0));
    text = JsonHelper::get(data, "text", QString(""));

    QString phSeqStr = JsonHelper::get(data, "ph_seq", QString(""));
    QString phDurStr = JsonHelper::get(data, "ph_dur", QString(""));
    rawPhSeq = phSeqStr;
    rawPhDur = phDurStr;

    QStringList phSeq = phSeqStr.split(' ', Qt::SkipEmptyParts);
    QStringList phDurList = phDurStr.split(' ', Qt::SkipEmptyParts);

    phones.clear();
    TimePos t = offset;
    for (int i = 0; i < phSeq.size(); ++i) {
        Phone phone;
        phone.symbol = phSeq[i];
        phone.duration = secToUs((i < phDurList.size()) ? phDurList[i].toDouble() : 0.0);
        phone.start = t;
        t += phone.duration;
        phones.push_back(phone);
    }

    QString noteSeqStr = JsonHelper::get(data, "note_seq", QString(""));
    QString noteDurStr = JsonHelper::get(data, "note_dur", QString(""));
    QString noteSlurStr = JsonHelper::get(data, "note_slur", QString(""));
    QString noteGlideStr = JsonHelper::get(data, "note_glide", QString(""));

    QStringList noteSeq = noteSeqStr.split(' ', Qt::SkipEmptyParts);
    QStringList noteDurList = noteDurStr.split(' ', Qt::SkipEmptyParts);
    QStringList noteSlurList = noteSlurStr.split(' ', Qt::SkipEmptyParts);
    QStringList noteGlideList = noteGlideStr.split(' ', Qt::SkipEmptyParts);

    notes.clear();
    t = offset;
    for (int i = 0; i < noteSeq.size(); ++i) {
        Note note;
        note.name = noteSeq[i];
        note.duration = secToUs((i < noteDurList.size()) ? noteDurList[i].toDouble() : 0.0);
        note.slur = (i < noteSlurList.size()) ? noteSlurList[i].toInt() : 0;
        note.glide = (i < noteGlideList.size()) ? noteGlideList[i] : QString("none");
        note.start = t;
        t += note.duration;
        notes.push_back(note);
    }

    rawPhNum = JsonHelper::get(data, "ph_num", QString(""));

    QString f0Str = JsonHelper::get(data, "f0_seq", QString(""));
    if (!f0Str.isEmpty()) {
        QStringList f0List = f0Str.split(' ', Qt::SkipEmptyParts);
        std::vector<double> hzValues;
        hzValues.reserve(f0List.size());
        for (const QString &v : f0List) {
            hzValues.push_back(v.toDouble());
        }
        f0.values = hzToMhzBatch(hzValues);
    }
    f0.timestep = secToUs(DsDocument::numberOrString(data, "f0_timestep", 0.0));

    modified = false;
}

void DSFile::writeBackToJson(nlohmann::json &obj) const {
    QStringList noteSeq, noteDur, noteSlur, noteGlide;
    for (const Note &note : notes) {
        noteSeq.append(note.name);
        noteDur.append(QString::number(usToSec(note.duration), 'g', 10));
        noteSlur.append(QString::number(note.slur));
        noteGlide.append(note.glide);
    }
    obj["note_seq"] = noteSeq.join(' ').toStdString();
    obj["note_dur"] = noteDur.join(' ').toStdString();
    obj["note_slur"] = noteSlur.join(' ').toStdString();
    obj["note_glide"] = noteGlide.join(' ').toStdString();

    auto hzValues = mhzToHzBatch(f0.values);
    QStringList f0List;
    for (double hz : hzValues) {
        if (hz > 0.0) {
            f0List.append(QString::number(hz, 'g', 12));
        } else {
            f0List.append("0");
        }
    }
    obj["f0_seq"] = f0List.join(' ').toStdString();
}

std::pair<std::shared_ptr<DSFile>, QString> DSFile::load(const QString &path) {
    auto docResult = DsDocument::loadFile(path);
    if (!docResult) {
        return {nullptr, QString::fromStdString(docResult.error())};
    }
    auto doc = std::move(*docResult);
    if (doc.isEmpty()) {
        return {nullptr, QStringLiteral("Empty document")};
    }

    auto ds = std::make_shared<DSFile>();
    ds->m_doc = std::move(doc);
    if (ds->m_doc.sentenceCount() <= 0) {
        return {nullptr, QString("No sentences found in file")};
    }
    ds->loadFromJson(ds->m_doc.sentence(0));
    return {ds, QString()};
}

std::pair<bool, QString> DSFile::save(const QString &path) {
    writeBackToJson(m_doc.sentence(0));

    auto saveResult = m_doc.saveFile(path);
    if (!saveResult) {
        return {false, QString::fromStdString(saveResult.error())};
    }

    modified = false;
    return {true, QString()};
}

void DSFile::recomputeNoteStarts() {
    TimePos t = offset;
    for (Note &note : notes) {
        note.start = t;
        t += note.duration;
    }
}

void DSFile::markModified() {
    modified = true;
}

int DSFile::getNoteCount() const {
    int count = 0;
    for (const Note &note : notes) {
        if (!note.isRest()) ++count;
    }
    return count;
}

TimePos DSFile::getTotalDuration() const {
    if (notes.empty()) return 0;
    return notes.back().end();
}

QString DSFile::filePath() const {
    QString docPath = m_doc.filePath();
    if (!docPath.isEmpty())
        return docPath;
    return m_filePath;
}

void DSFile::setFilePath(const QString &path) {
    m_filePath = path;
}

} // namespace pitchlabeler
} // namespace dstools
