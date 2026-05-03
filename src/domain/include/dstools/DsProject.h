#pragma once

/// @file DsProject.h
/// @brief .dsproj project file data model for DiffSinger Labeler.
///
/// Stores project-level settings: working directory, default model paths,
/// inference parameters, items and slices. Preserves unknown JSON fields
/// for forward compatibility.

#include <nlohmann/json.hpp>

#include <QString>
#include <QStringList>

#include <map>
#include <vector>

namespace dstools {

/// @brief Configuration for a task's model backend.
struct TaskModelConfig {
    QString processorId;        ///< Processor implementation (e.g. "hubert-fa", "rmvpe").
    QString modelPath;          ///< Path to model file or directory.
    QString provider = "cpu";   ///< Execution provider: "cpu", "dml", or "cuda".
    int deviceId = 0;           ///< GPU device index.
    bool forceCpu = false;      ///< Override global provider to force CPU for this model.
    nlohmann::json extra;       ///< Engine-specific parameters.
};

/// @brief Preload configuration for a task.
struct PreloadConfig {
    bool enabled = false;
    int count = 10;
};

/// @brief Export configuration.
struct ExportConfig {
    QStringList formats;
    int hopSize = 512;
    int sampleRate = 44100;
    int resampleRate = 44100;
    bool includeDiscarded = false;
};

/// Default model paths and inference parameters stored in a .dsproj file.
struct DsProjectDefaults {
    QString globalProvider = QStringLiteral("cpu");  ///< Global inference provider.
    int deviceIndex = 0;                              ///< Global GPU device index.
    std::map<QString, TaskModelConfig> taskModels;    ///< Task name → model config.
    std::map<QString, PreloadConfig> preload;          ///< Task name → preload config.
    ExportConfig exportConfig;
};

/// @brief A single slice within an item.
struct Slice {
    QString id;
    QString name;
    int64_t inPos = 0;          ///< Start position in microseconds.
    int64_t outPos = 0;         ///< End position in microseconds.
    QString status = QStringLiteral("active");  ///< "active", "discarded", "error"
    QString discardReason;
    QString discardedAt;        ///< Step at which the slice was discarded.
    nlohmann::json extra;       ///< Preserve unknown fields.
};

/// @brief An audio item containing one or more slices.
struct Item {
    QString id;
    QString name;
    QString speaker;
    QString language;
    QString audioSource;        ///< Relative path to audio file (POSIX).
    std::vector<Slice> slices;
    nlohmann::json extra;       ///< Preserve unknown fields.
};

/// In-memory representation of a .dsproj project file.
class DsProject {
public:
    DsProject() = default;

    // ── File I/O ──────────────────────────────────────────────────────

    /// Load a .dsproj file. Returns a default project on failure.
    static DsProject load(const QString &path, QString &error);

    /// Save the project to a file path. Empty path = use original path.
    bool save(const QString &path, QString &error) const;
    bool save(QString &error) const;

    // ── Properties ────────────────────────────────────────────────────

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &dir);

    DsProjectDefaults defaults() const;
    void setDefaults(const DsProjectDefaults &defaults);

    const QString &filePath() const;

    // ── Items ─────────────────────────────────────────────────────────

    const std::vector<Item> &items() const;
    void setItems(std::vector<Item> items);

    // ── Path utilities ────────────────────────────────────────────────

    static QString toPosixPath(const QString &nativePath);
    static QString fromPosixPath(const QString &posixPath);

private:
    QString m_filePath;
    QString m_workingDirectory;
    DsProjectDefaults m_defaults;
    std::vector<Item> m_items;
    nlohmann::json m_extraFields;  // preserve unknown fields for round-trip
};

} // namespace dstools
