#pragma once

/// @file PipelineContext.h
/// @brief Per-item pipeline state — layers, progress, dirty tracking, serialization.
///
/// nlohmann::json is isolated: layers are stored as LayerData (string carrier),
/// and toJsonString()/fromJsonString() operate on std::string.

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <dsfw/TaskTypes.h>
#include <dsfw/Result.h>
#include <dsfw/TimePos.h>
#include <map>
#include <vector>

namespace dsfw {

    struct StepRecord {
        QString stepName;
        QString processorId;
        QDateTime startTime;
        QDateTime endTime;
        bool success = false;
        QString errorMessage;
        ProcessorConfig usedConfig;
    };

    struct PipelineContext {
        // Identity
        QString audioPath;
        QString itemId;

        // Layer data (isolated from nlohmann::json — use LayerData::toJson() when needed)
        std::map<QString, LayerData> layers;
        ProcessorConfig globalConfig;

        // Status
        enum class Status { Active, Discarded, Error };
        Status status = Status::Active;
        QString discardReason;
        QString discardedAtStep;

        // Progress
        std::vector<StepRecord> stepHistory;
        QStringList editedSteps;
        QStringList dirty;
        QStringList manuallyEdited; // layers manually edited by user; auto-recompute won't overwrite

        // Core methods
    [[nodiscard]] Result<TaskInput> buildTaskInput(const TaskSpec &spec) const;
    void applyTaskOutput(const TaskSpec &spec, const TaskOutput &output);

    [[nodiscard]] Result<void> checkPreconditions(const TaskSpec &spec) const;

    // Convenience: derive completed steps from stepHistory
    QStringList completedSteps() const;

        // Dirty layer propagation (see pipeline.md §3.1 for I/O contract)
        static const std::map<QString, QStringList> layerDag;
        void propagateDirty(const QString &modifiedLayer);
        void addDirtyLayer(const QString &layer);
        void removeDirtyLayer(const QString &layer);

        // Step-level dirty derivation
        QStringList deriveDirtySteps(const std::vector<TaskSpec> &taskSpecs) const;

        // Manual edit protection: auto-recompute won't overwrite these layers
        void setLayerManuallyEdited(const QString &layer, bool edited = true);
        bool isLayerManuallyEdited(const QString &layer) const;

        // Track which pipeline steps were manually edited
        void addEditedStep(const QString &step);

        // Serialization — string-based (nlohmann::json not in public API)
    std::string toJsonString() const;
    [[nodiscard]] static Result<PipelineContext> fromJsonString(const std::string &jsonStr);

    // Schema validation for loaded PipelineContext JSON (string-based, isolates nlohmann::json)
    [[nodiscard]] static Result<void> validateFromString(const std::string &jsonStr);

    friend class PipelineRunner;

private:
        nlohmann::json toJson() const;
        [[nodiscard]] static Result<PipelineContext> fromJson(const nlohmann::json &j);
        [[nodiscard]] static Result<void> validate(const nlohmann::json &j);
    };

} // namespace dsfw
