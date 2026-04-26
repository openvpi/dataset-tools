#include <dstools/DsPitchDocument.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace dstools {

    DsPitchDocument::DsPitchDocument() = default;

    void DsPitchDocument::recomputeNoteStarts() {
        TimePos t = offset;
        for (Note &note : notes) {
            note.start = t;
            t += note.duration;
        }
    }

    void DsPitchDocument::markModified() {
        modified = true;
    }

    int DsPitchDocument::getNoteCount() const {
        int count = 0;
        for (const Note &note : notes) {
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

    void DsPitchDocument::setFilePath(const QString &path) {
        m_filePath = path;
    }

    QString DsPitchDocument::serializeNote(const Note &note) {
        QJsonObject obj;
        obj["n"] = note.name;
        obj["d"] = static_cast<qint64>(note.duration);
        if (note.slur != 0)
            obj["s"] = note.slur;
        if (!note.glide.isEmpty())
            obj["g"] = note.glide;
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    std::optional<Note> DsPitchDocument::deserializeNote(const QString &json) {
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