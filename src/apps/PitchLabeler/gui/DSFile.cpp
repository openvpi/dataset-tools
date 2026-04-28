#include "DSFile.h"

#include <dstools/JsonHelper.h>
#include <QStringList>

#include <cmath>

namespace dstools {
namespace pitchlabeler {

// ============================================================================
// DSFile
// ============================================================================

DSFile::DSFile() = default;

void DSFile::loadFromJson(const nlohmann::json &data) {
    // offset and f0_timestep may be number or string — use DsDocument helper
    offset = DsDocument::numberOrString(data, "offset", 0.0);
    text = JsonHelper::get(data, "text", QString(""));

    // Parse phones
    QString phSeqStr = JsonHelper::get(data, "ph_seq", QString(""));
    QString phDurStr = JsonHelper::get(data, "ph_dur", QString(""));
    rawPhSeq = phSeqStr;
    rawPhDur = phDurStr;

    QStringList phSeq = phSeqStr.split(' ', Qt::SkipEmptyParts);
    QStringList phDurList = phDurStr.split(' ', Qt::SkipEmptyParts);

    phones.clear();
    double t = offset;
    for (int i = 0; i < phSeq.size(); ++i) {
        Phone phone;
        phone.symbol = phSeq[i];
        phone.duration = (i < phDurList.size()) ? phDurList[i].toDouble() : 0.0;
        phone.start = t;
        t += phone.duration;
        phones.push_back(phone);
    }

    // Parse notes
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
        note.duration = (i < noteDurList.size()) ? noteDurList[i].toDouble() : 0.0;
        note.slur = (i < noteSlurList.size()) ? noteSlurList[i].toInt() : 0;
        note.glide = (i < noteGlideList.size()) ? noteGlideList[i] : QString("none");
        note.start = t;
        t += note.duration;
        notes.push_back(note);
    }

    // Parse ph_num (preserved verbatim)
    rawPhNum = JsonHelper::get(data, "ph_num", QString(""));

    // Parse F0
    QString f0Str = JsonHelper::get(data, "f0_seq", QString(""));
    if (!f0Str.isEmpty()) {
        QStringList f0List = f0Str.split(' ', Qt::SkipEmptyParts);
        f0.values.clear();
        f0.values.reserve(f0List.size());
        for (const QString &v : f0List) {
            double freq = v.toDouble();
            // Convert frequency (Hz) to MIDI note number during loading
            if (freq > 0.0) {
                f0.values.push_back(69.0 + 12.0 * std::log2(freq / 440.0));
            } else {
                f0.values.push_back(0.0);
            }
        }
    }
    f0.timestep = DsDocument::numberOrString(data, "f0_timestep", 0.0);

    modified = false;
}

void DSFile::writeBackToJson(nlohmann::json &obj) const {
    // Only update fields that PitchLabeler modifies (note fields and f0_seq).
    // All other fields (offset, text, ph_*, f0_timestep, unknown fields)
    // are preserved from the original JSON with their original types.
    QStringList noteSeq, noteDur, noteSlur, noteGlide;
    for (const Note &note : notes) {
        noteSeq.append(note.name);
        noteDur.append(QString::number(note.duration, 'g', 10));
        noteSlur.append(QString::number(note.slur));
        noteGlide.append(note.glide);
    }
    obj["note_seq"] = noteSeq.join(' ').toStdString();
    obj["note_dur"] = noteDur.join(' ').toStdString();
    obj["note_slur"] = noteSlur.join(' ').toStdString();
    obj["note_glide"] = noteGlide.join(' ').toStdString();

    // F0 - convert MIDI note numbers back to Hz for saving
    QStringList f0List;
    for (double midi : f0.values) {
        if (midi > 0.0) {
            double freq = 440.0 * std::pow(2.0, (midi - 69.0) / 12.0);
            f0List.append(QString::number(freq, 'g', 12));
        } else {
            f0List.append("0");
        }
    }
    obj["f0_seq"] = f0List.join(' ').toStdString();
}

std::pair<std::shared_ptr<DSFile>, QString> DSFile::load(const QString &path) {
    QString error;
    auto doc = DsDocument::load(path, error);
    if (doc.isEmpty()) {
        return {nullptr, error};
    }

    auto ds = std::make_shared<DSFile>();
    ds->m_doc = std::move(doc);
    ds->loadFromJson(ds->m_doc.sentence(0));
    return {ds, QString()};
}

std::pair<bool, QString> DSFile::save(const QString &path) {
    // Write modified fields back into the original JSON sentence
    writeBackToJson(m_doc.sentence(0));

    QString error;
    if (!m_doc.save(path, error)) {
        return {false, error};
    }

    modified = false;
    return {true, QString()};
}

void DSFile::recomputeNoteStarts() {
    double t = offset;
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

double DSFile::getTotalDuration() const {
    if (notes.empty()) return 0.0;
    return notes.back().end();
}

QString DSFile::filePath() const {
    return m_doc.filePath();
}

} // namespace pitchlabeler
} // namespace dstools
