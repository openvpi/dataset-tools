#pragma once

#include <QJsonObject>
#include <QString>
#include <dstools/F0Curve.h>
#include <dstools/TimePos.h>
#include <optional>
#include <vector>

namespace dstools {

    struct Phone {
        QString symbol;
        TimePos start = 0;
        TimePos duration = 0;

        TimePos end() const {
            return start + duration;
        }
    };

    struct Note {
        QString name;
        TimePos duration = 0;
        int slur = 0;
        QString glide;
        TimePos start = 0;

        TimePos end() const {
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

        TimePos offset = 0;
        QString text;
        std::vector<Phone> phones;
        std::vector<Note> notes;
        F0Curve f0;

        bool modified = false;

        void recomputeNoteStarts();
        void markModified();

        int getNoteCount() const;
        TimePos getTotalDuration() const;

        QString filePath() const;
        void setFilePath(const QString &path);

        static QString serializeNote(const Note &note);
        static std::optional<Note> deserializeNote(const QString &json);

    private:
        QString m_filePath;
    };

} // namespace dstools

namespace dstools::pitchlabeler {
    using dstools::DsPitchDocument;
    using dstools::Phone;
    using dstools::Note;
} // namespace dstools::pitchlabeler