#pragma once

#include <QString>
#include <vector>
#include <memory>

#include <dstools/DsDocument.h>
#include <dstools/F0Curve.h>
#include <dstools/TimePos.h>

namespace dstools {
namespace pitchlabeler {

using dstools::F0Curve;
using dstools::TimePos;

struct Phone {
    QString symbol;
    TimePos start = 0;
    TimePos duration = 0;

    TimePos end() const { return start + duration; }
};

struct Note {
    QString name;
    TimePos duration = 0;
    int slur = 0;
    QString glide;
    TimePos start = 0;

    TimePos end() const { return start + duration; }
    bool isRest() const { return name.trimmed().toLower() == "rest"; }
    bool isSlur() const { return slur == 1; }
};

class DSFile {
public:
    DSFile();

    static std::pair<std::shared_ptr<DSFile>, QString> load(const QString &path);
    std::pair<bool, QString> save(const QString &path = QString());

    TimePos offset = 0;
    QString text;
    std::vector<Phone> phones;
    std::vector<Note> notes;
    F0Curve f0;

    QString rawPhSeq;
    QString rawPhDur;
    QString rawPhNum;

    bool modified = false;

    void recomputeNoteStarts();
    void markModified();

    int getNoteCount() const;
    TimePos getTotalDuration() const;

    QString filePath() const;
    void setFilePath(const QString &path);

private:
    void loadFromJson(const nlohmann::json &data);
    void writeBackToJson(nlohmann::json &obj) const;

    DsDocument m_doc;
    QString m_filePath;
};

} // namespace pitchlabeler
} // namespace dstools
