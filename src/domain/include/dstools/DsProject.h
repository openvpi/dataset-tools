#pragma once

/// @file DsProject.h
/// @brief .dsproj project file data model for DiffSinger Labeler.
///
/// Stores project-level settings: working directory, default model paths,
/// inference parameters. Preserves unknown JSON fields for forward compatibility.

#include <nlohmann/json.hpp>

#include <QString>

namespace dstools {

/// Default model paths and inference parameters stored in a .dsproj file.
struct DsProjectDefaults {
    QString asrModelPath;
    QString hubertModelPath;
    QString gameModelPath;
    QString rmvpeModelPath;
    int gpuIndex = -1;  // -1 = CPU
    int hopSize = 512;
    int sampleRate = 44100;
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

private:
    QString m_filePath;
    QString m_workingDirectory;
    DsProjectDefaults m_defaults;
    nlohmann::json m_extraFields;  // preserve unknown fields for round-trip
};

} // namespace dstools
