#pragma once

/// @file DsItemManager.h
/// @brief .dsitem flow tracking for the DiffSinger dataset pipeline.
///
/// DsItemManager tracks per-file, per-step processing state using .dsitem
/// JSON files stored under `dstemp/<step>/`. This enables incremental
/// processing: only files whose source has changed or whose step has not
/// yet completed are reprocessed.

#include <QString>
#include <QDateTime>
#include <QStringList>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dstools {

enum class PipelineStep {
    Slicer,           // "slicer"
    Asr,              // "asr"
    AsrReview,        // "asr_review"
    Alignment,        // "alignment"
    AlignmentReview,  // "alignment_review"
    BuildCsv,         // "build_csv"
    Midi,             // "midi"
    BuildDs,          // "build_ds"
    PitchReview       // "pitch_review"
};

enum class ItemStatus {
    Pending,      // "pending"
    Completed,    // "completed"
    Failed,       // "failed"
    Reviewed,     // "reviewed"
    Unreviewed    // "unreviewed"
};

struct ModelInfo {
    QString name;
    QString version;
    QString path;
    bool isNull() const { return name.isEmpty(); }
};

struct DsItemRecord {
    QString version = "1.0.0";
    QString sourceFile;
    PipelineStep step = PipelineStep::Slicer;
    std::optional<ModelInfo> model;
    nlohmann::json params;
    QStringList inputs;
    QStringList outputs;
    QDateTime timestamp;
    ItemStatus status = ItemStatus::Pending;
};

class DsItemManager {
public:
    explicit DsItemManager(const std::filesystem::path &projectRoot);

    // ── Read / Write ──────────────────────────────────────────────────

    bool load(const std::filesystem::path &dsitemPath, DsItemRecord &record, std::string &error) const;
    bool save(const std::filesystem::path &dsitemPath, const DsItemRecord &record,
              std::string &error) const;

    // ── Query ─────────────────────────────────────────────────────────

    std::filesystem::path itemPath(const QString &sourceFile, PipelineStep step) const;
    ItemStatus queryStatus(const QString &sourceFile, PipelineStep step) const;

    // ── Incremental processing ────────────────────────────────────────

    bool needsProcessing(const QString &sourceFile, PipelineStep step) const;

    // ── Batch query ───────────────────────────────────────────────────

    struct StepSummary {
        int total = 0;
        int completed = 0;
        int failed = 0;
        int pending = 0;
    };
    StepSummary summarizeStep(PipelineStep step) const;

    // ── Update ────────────────────────────────────────────────────────

    bool markCompleted(const QString &sourceFile, PipelineStep step,
                       const QStringList &outputs, std::string &error);
    bool markFailed(const QString &sourceFile, PipelineStep step,
                    const QString &errorMsg, std::string &error);

    // ── Accessors ─────────────────────────────────────────────────────

    const std::filesystem::path &projectRoot() const { return m_projectRoot; }

private:
    std::filesystem::path m_projectRoot;

    static QString stepToString(PipelineStep step);
    static PipelineStep stringToStep(const QString &s);
    static QString statusToString(ItemStatus status);
    static ItemStatus stringToStatus(const QString &s);

    nlohmann::json recordToJson(const DsItemRecord &record) const;
    DsItemRecord jsonToRecord(const nlohmann::json &j) const;
};

} // namespace dstools
