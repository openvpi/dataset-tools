#pragma once

#include <QString>
#include <vector>
#include <memory>

#include <dstools/DsDocument.h>
#include <dstools/F0Curve.h>

namespace dstools {
namespace pitchlabeler {

using dstools::F0Curve;

/// Represents a single phoneme with its time interval
struct Phone {
    QString symbol;     // e.g. 'sh', 'uo', 'SP', 'AP'
    double start;      // start time in seconds
    double duration;   // duration in seconds

    double end() const { return start + duration; }
};

/// Represents a single note from the DS file
struct Note {
    QString name;      // e.g. 'C#4+12', 'rest'
    double duration;   // duration in seconds
    int slur;         // 0 or 1
    QString glide;     // e.g. 'none'
    double start;      // computed from cumulative durations

    double end() const { return start + duration; }
    bool isRest() const { return name.trimmed().toLower() == "rest"; }
    bool isSlur() const { return slur == 1; }
};

/// In-memory representation of a DS file.
/// Uses DsDocument for file I/O (path encoding, atomic writes, round-trip safety).
class DSFile {
public:
    DSFile();

    // Load/save
    static std::pair<std::shared_ptr<DSFile>, QString> load(const QString &path);
    std::pair<bool, QString> save(const QString &path = QString());

    // Data access
    double offset = 0.0;
    QString text;
    std::vector<Phone> phones;
    std::vector<Note> notes;
    F0Curve f0;

    // Raw preserved fields (never modified by PitchLabeler)
    QString rawPhSeq;
    QString rawPhDur;
    QString rawPhNum;

    // State
    bool modified = false;

    // Helpers
    void recomputeNoteStarts();
    void markModified();

    int getNoteCount() const;
    double getTotalDuration() const;

    /// File path of the loaded document.
    QString filePath() const;

private:
    void loadFromJson(const nlohmann::json &data);
    void writeBackToJson(nlohmann::json &obj) const;

    DsDocument m_doc;  // underlying document for I/O and round-trip preservation
};

} // namespace pitchlabeler
} // namespace dstools
