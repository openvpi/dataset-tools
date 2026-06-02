#include <dstools/DsPitchDocument.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace dstools {

DsPitchDocument::DsPitchDocument() = default;

void DsPitchDocument::recomputeNoteStarts() {
    TimePos t = offset;
    for (Note& note : notes) {
        note.start = t;
        t += note.duration;
    }
}

void DsPitchDocument::markModified() {
    modified = true;
}

int DsPitchDocument::getNoteCount() const {
    int count = 0;
    for (const Note& note : notes) {
        if (!note.isRest())
            ++count;
    }
    return count;
}

TimePos DsPitchDocument::getTotalDuration() const {
    if (notes.empty())
        return 0;
    return notes.back().end();
}

QString DsPitchDocument::filePath() const {
    return m_filePath;
}

void DsPitchDocument::setFilePath(const QString& path) {
    m_filePath = path;
}

QString DsPitchDocument::serializeNote(const Note& note) {
    QJsonObject obj;
    obj["n"] = note.name;
    obj["d"] = static_cast<qint64>(note.duration);
    if (note.slur != 0)
        obj["s"] = note.slur;
    if (!note.glide.isEmpty())
        obj["g"] = note.glide;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

std::optional<Note> DsPitchDocument::deserializeNote(const QString& json) {
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return std::nullopt;
    QJsonObject obj = doc.object();

    Note note;
    note.name = obj["n"].toString();
    note.duration = static_cast<TimePos>(obj["d"].toDouble());
    note.slur = obj["s"].toInt(0);
    note.glide = obj["g"].toString();
    return note;
}

} // namespace dstools

Result<void> DsPitchDocument::validate() const {
    for (size_t i = 0; i < notes.size(); ++i) {
        const auto& note = notes[i];
        if (note.name.isEmpty()) {
            return Result<void>::Error("notes[" + std::to_string(i) + "].name is empty");
        }
        if (note.duration <= 0) {
            return Result<void>::Error("notes[" + std::to_string(i) + "].duration is non-positive");
        }
        if (note.start < 0) {
            return Result<void>::Error("notes[" + std::to_string(i) + "].start is negative");
        }
    }

    for (size_t i = 0; i < phones.size(); ++i) {
        const auto& phone = phones[i];
        if (phone.symbol.isEmpty()) {
            return Result<void>::Error("phones[" + std::to_string(i) + "].symbol is empty");
        }
        if (phone.duration <= 0) {
            return Result<void>::Error("phones[" + std::to_string(i) + "].duration is non-positive");
        }
        if (phone.start < 0) {
            return Result<void>::Error("phones[" + std::to_string(i) + "].start is negative");
        }
    }

    if (f0.timestep < 0) {
        return Result<void>::Error("f0.timestep is negative");
    }

    for (size_t i = 0; i < f0.values.size(); ++i) {
        if (f0.values[i] < 0.0f) {
            return Result<void>::Error("f0.values[" + std::to_string(i) + "] is negative");
        }
    }

    return Result<void>::Ok();
}