#include "DSFile.h"

#include <dstools/JsonHelper.h>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QFileInfo>

#include <cmath>

namespace dstools {
namespace pitchlabeler {

// ============================================================================
// F0Curve
// ============================================================================

double F0Curve::totalDuration() const {
    if (values.empty()) return 0.0;
    return values.size() * timestep;
}

double F0Curve::getValueAtTime(double time) const {
    if (values.empty() || timestep <= 0) return 0.0;
    double index = time / timestep;
    int i = static_cast<int>(index);
    if (i < 0) return values.front();
    if (i >= static_cast<int>(values.size()) - 1) return values.back();
    double frac = index - i;
    return values[i] * (1.0 - frac) + values[i + 1] * frac;
}

std::vector<double> F0Curve::getRange(double startTime, double endTime) const {
    if (values.empty() || timestep <= 0) return {};
    int startIdx = static_cast<int>(startTime / timestep);
    int endIdx = static_cast<int>(endTime / timestep) + 1;
    startIdx = qMax(0, startIdx);
    endIdx = qMin(static_cast<int>(values.size()), endIdx);
    if (startIdx >= endIdx) return {};
    return std::vector<double>(values.begin() + startIdx, values.begin() + endIdx);
}

void F0Curve::setRange(double startTime, const std::vector<double> &newValues) {
    if (values.empty() || timestep <= 0) return;
    int startIdx = static_cast<int>(startTime / timestep);
    for (size_t i = 0; i < newValues.size(); ++i) {
        int idx = startIdx + static_cast<int>(i);
        if (idx >= 0 && idx < static_cast<int>(values.size())) {
            values[idx] = newValues[i];
        }
    }
}

// ============================================================================
// DSFile
// ============================================================================

DSFile::DSFile() = default;

void DSFile::loadFromJson(const nlohmann::json &data) {
    using namespace dstools;

    // offset may be stored as number or string in DS files.
    if (data.contains("offset")) {
        const auto &node = data["offset"];
        if (node.is_number()) {
            offset = node.get<double>();
        } else if (node.is_string()) {
            offset = QString::fromStdString(node.get<std::string>()).toDouble();
        }
    }
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
            // Convert frequency (Hz) to MIDI note number during loading,
            // consistent with SlurCutter: midi = 69 + 12 * log2(f / 440)
            if (freq > 0.0) {
                f0.values.push_back(69.0 + 12.0 * std::log2(freq / 440.0));
            } else {
                f0.values.push_back(0.0);
            }
        }
    }
    // f0_timestep may be stored as number or string in DS files.
    // SlurCutter reads it as string then converts; we handle both.
    if (data.contains("f0_timestep")) {
        const auto &node = data["f0_timestep"];
        if (node.is_number()) {
            f0.timestep = node.get<double>();
        } else if (node.is_string()) {
            f0.timestep = QString::fromStdString(node.get<std::string>()).toDouble();
        }
    }

    // Capture extra fields
    extraFields.clear();
    QStringList knownFields = {"offset", "text", "ph_seq", "ph_dur", "ph_num",
                               "note_seq", "note_dur", "note_slur", "note_glide",
                               "f0_seq", "f0_timestep"};
    for (auto &item : data.items()) {
        if (!knownFields.contains(QString::fromStdString(item.key()))) {
            extraFields[item.key()] = item.value();
        }
    }

    modified = false;
}

nlohmann::json DSFile::toJson() const {
    nlohmann::json obj;

    obj["offset"] = offset;
    obj["text"] = text.toStdString();
    obj["ph_seq"] = rawPhSeq.toStdString();
    obj["ph_dur"] = rawPhDur.toStdString();
    obj["ph_num"] = rawPhNum.toStdString();

    // Note fields (what we modify)
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
    obj["f0_timestep"] = f0.timestep;

    // Extra fields
    obj.insert(extraFields.begin(), extraFields.end());

    return obj;
}

std::shared_ptr<DSFile> DSFile::fromJson(const nlohmann::json &data) {
    auto ds = std::make_shared<DSFile>();
    ds->loadFromJson(data);
    return ds;
}

std::pair<std::shared_ptr<DSFile>, QString> DSFile::load(const QString &path) {
    using namespace dstools;

    std::string error;
    auto json = JsonHelper::loadFile(path.toStdString(), error);
    if (!error.empty()) {
        return {nullptr, QString::fromStdString(error)};
    }

    if (!json.is_array() || json.empty()) {
        return {nullptr, "DS file must be a JSON array with at least one object"};
    }

    auto ds = fromJson(json[0]);
    ds->filePath = path.toStdString();
    return {ds, QString()};
}

std::pair<bool, QString> DSFile::save(const QString &path) {
    QString targetPath = path.isEmpty() ? QString::fromStdString(filePath.string()) : path;
    if (targetPath.isEmpty()) {
        return {false, "No file path specified"};
    }
    return saveAs(targetPath);
}

std::pair<bool, QString> DSFile::saveAs(const QString &path) {
    using namespace dstools;

    nlohmann::json data = nlohmann::json::array({toJson()});

    std::string error;
    if (!JsonHelper::saveFile(path.toStdString(), data, error)) {
        return {false, QString::fromStdString(error)};
    }

    modified = false;
    filePath = path.toStdString();
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

} // namespace pitchlabeler
} // namespace dstools