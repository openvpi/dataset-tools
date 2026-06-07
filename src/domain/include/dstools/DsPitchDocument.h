#pragma once

#include <QJsonObject>
#include <QString>
#include <dstools/F0Curve.h>
#include <dsfw/Result.h>
#include <dsfw/TimePos.h>
#include <optional>
#include <vector>

namespace dstools {

struct Phone {
    QString symbol;
    dsfw::TimePos start = 0;
    dsfw::TimePos duration = 0;

    dsfw::TimePos end() const {
        return start + duration;
    }
};

struct Note {
    QString name;
    dsfw::TimePos duration = 0;
    int slur = 0;
    QString glide;
    dsfw::TimePos start = 0;

    dsfw::TimePos end() const {
        return start + duration;
    }
    bool isRest() const {
        return name.trimmed().toLower() == "rest";
    }
    bool isSlur() const {
        return slur == 1;
    }
};

class DsPitchDocument {
public:
    DsPitchDocument();

    dsfw::TimePos offset = 0;
    QString text;
    std::vector<Phone> phones;
    std::vector<Note> notes;
    F0Curve f0;

    bool modified = false;

    void recomputeNoteStarts();
    void markModified();

    int getNoteCount() const;
    dsfw::TimePos getTotalDuration() const;

    QString filePath() const;
    void setFilePath(const QString& path);

    static QString serializeNote(const Note& note);
    static std::optional<Note> deserializeNote(const QString& json);

    [[nodiscard]] dsfw::Result<void> validate() const;

private:
    QString m_filePath;
};

}  // namespace dstools

namespace dstools::pitchlabeler {
using ::dstools::DsPitchDocument;
using ::dstools::Note;
using ::dstools::Phone;
}  // namespace dstools::pitchlabeler