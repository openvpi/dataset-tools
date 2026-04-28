#pragma once

#include <QString>
#include <vector>
#include <memory>
#include <filesystem>

#include <nlohmann/json.hpp>

namespace dstools {
namespace pitchlabeler {

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

/// Represents F0 (fundamental frequency) curve data
struct F0Curve {
    std::vector<double> values;
    double timestep = 0.0;  // seconds between samples

    double totalDuration() const;
    double getValueAtTime(double time) const;
    std::vector<double> getRange(double startTime, double endTime) const;
    void setRange(double startTime, const std::vector<double> &newValues);
};

/// In-memory representation of a DS file
class DSFile {
public:
    DSFile();

    // Load/save
    static std::pair<std::shared_ptr<DSFile>, QString> load(const QString &path);
    std::pair<bool, QString> save(const QString &path = QString());
    std::pair<bool, QString> saveAs(const QString &path);

    // Data access
    double offset = 0.0;
    QString text;
    std::vector<Phone> phones;
    std::vector<Note> notes;
    F0Curve f0;

    // Raw preserved fields (never modified)
    QString rawPhSeq;
    QString rawPhDur;
    QString rawPhNum;

    // Extra fields preserved round-trip
    nlohmann::json m_rawJson;  // original JSON object, updated in-place on save

    // State
    bool modified = false;
    std::filesystem::path filePath;

    // Helpers
    void recomputeNoteStarts();
    void markModified();
    nlohmann::json toJson() const;
    static std::shared_ptr<DSFile> fromJson(const nlohmann::json &data);

    int getNoteCount() const;
    double getTotalDuration() const;

private:
    void loadFromJson(const nlohmann::json &data);
};

} // namespace pitchlabeler
} // namespace dstools