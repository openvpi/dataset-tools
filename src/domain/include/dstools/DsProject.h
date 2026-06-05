#pragma once

/// @file DsProject.h
/// @brief .dsproj project file data model for DiffSinger Labeler.
///
/// Stores project-level settings: working directory, default model paths,
/// inference parameters, items and slices. Preserves unknown JSON fields
/// for forward compatibility.

#include <dstools/Constants.h>
#include <QString>
#include <QStringList>
#include <dsfw/ConfigTypes.h>
#include <dstools/Result.h>
#include <map>
#include <memory>
#include <vector>

namespace dstools {

/// @brief Configuration for a task's model backend.
struct TaskModelConfig {
    QString processorId;      ///< Processor implementation (e.g. "hubert-fa", "rmvpe").
    QString modelPath;        ///< Path to model file or directory.
    QString provider = "cpu"; ///< Execution provider: "cpu", "dml", or "cuda".
    int deviceId = 0;         ///< GPU device index.
    bool forceCpu = false;    ///< Override global provider to force CPU for this model.
    ConfigMap extra;          ///< Engine-specific parameters.
};

/// @brief Export configuration.
struct ExportConfig {
    QStringList formats;
    int hopSize = constants::kDefaultHopSize;
    int sampleRate = constants::kDefaultSampleRate;
    int resampleRate = 44100;
    bool includeDiscarded = false;
};

/// @brief Slicer parameter configuration stored in .dsproj slicer.params.
struct SlicerConfig {
    double threshold = -40.0; ///< dB threshold for silence detection.
    int minLength = 5000;     ///< Minimum slice length in ms.
    int minInterval = 300;    ///< Minimum interval between slices in ms.
    int hopSize = 10;         ///< Hop size in ms.
    int maxSilence = 500;     ///< Maximum silence kept in ms.
};

/// @brief Slicer runtime state stored in .dsproj slicer section.
struct SlicerState {
    SlicerConfig params;                                ///< Slicer parameters.
    QStringList audioFiles;                             ///< Audio file paths (native format).
    std::map<QString, std::vector<double>> slicePoints; ///< filePath → boundary times (seconds).
};

/// @brief A single slice within an item.
struct Slice {
    QString id;
    QString name;
    int64_t inPos = 0;                         ///< Start position in microseconds.
    int64_t outPos = 0;                        ///< End position in microseconds.
    QString status = QStringLiteral("active"); ///< "active", "discarded", "error"
    QString discardReason;
    QString discardedAt; ///< Step at which the slice was discarded.
    ConfigMap extra;     ///< Preserve unknown fields.
};

/// @brief An audio item containing one or more slices.
struct Item {
    QString id;
    QString name;
    QString speaker;
    QString language;
    QString audioSource; ///< Path to audio file (native format, relative or absolute).
    std::vector<Slice> slices;
    ConfigMap extra; ///< Preserve unknown fields.
};

/// In-memory representation of a .dsproj project file.
class DsProject {
public:
    DsProject();
    ~DsProject();
    DsProject(DsProject&&) noexcept;
    DsProject& operator=(DsProject&&) noexcept;
    DsProject(const DsProject&) = delete;
    DsProject& operator=(const DsProject&) = delete;

    // ── File I/O ──────────────────────────────────────────────────────

    [[nodiscard]] static Result<DsProject> loadFile(const QString& path);

    [[nodiscard]] Result<void> saveFile(const QString& path = {}) const;

    // ── Validation ────────────────────────────────────────────────────

    /// Validate that items (slices) are consistent with slicer state.
    /// Checks: segment count vs slice points, timestamp alignment, audio file existence.
    /// Returns Ok() if consistent, or Error() with a newline-separated list of issues.
    [[nodiscard]] Result<void> validateSliceConsistency() const;

    /// Validate that all external paths referenced in the project exist.
    /// Checks: item audioSource paths, slicer audioFiles, model paths.
    /// Relative paths are resolved against the working directory.
    /// Returns a list of missing paths (empty if all exist).
    [[nodiscard]] std::vector<QString> validateExternalPaths() const;

    // ── Properties ────────────────────────────────────────────────────

    QString workingDirectory() const;
    void setWorkingDirectory(const QString& dir);

    const QString& filePath() const;

    // ── Items ─────────────────────────────────────────────────────────

    const std::vector<Item>& items() const;
    void setItems(std::vector<Item> items);

    // ── Slicer state ──────────────────────────────────────────────────

    const SlicerState& slicerState() const;
    void setSlicerState(SlicerState state);

    // ── Export config ──────────────────────────────────────────────────

    const ExportConfig& exportConfig() const;
    void setExportConfig(ExportConfig config);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dstools
