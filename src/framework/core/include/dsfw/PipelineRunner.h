#pragma once

#include <QObject>
#include <dsfw/IFormatAdapter.h>
#include <dsfw/ITaskProcessor.h>
#include <dsfw/PipelineContext.h>
#include <functional>
#include <memory>
#include <vector>

namespace dsfw {

    using ValidationCallback =
        std::function<PipelineContext::Status(const PipelineContext &ctx, const TaskSpec &spec, QString &reason)>;

    struct StepConfig {
        QString taskName;
        QString processorId;
        ProcessorConfig config;
        bool optional = false;
        bool manual = false;
        QString importFormat;
        QString importPath;
        QString exportFormat;
        QString exportPath;
        ValidationCallback validator;
    };

    struct PipelineOptions {
        std::vector<StepConfig> steps;
        ProcessorConfig globalConfig;
        QString workingDir;
        QString snapshotDir;
    };

    class PipelineRunner : public QObject {
        Q_OBJECT
    public:
        explicit PipelineRunner(QObject *parent = nullptr);

        static constexpr int kMaxSnapshots = 3;

        /// Run the pipeline on a list of contexts.
        /// Each context represents one item (e.g. one audio slice).
        [[nodiscard]] Result<void> run(const PipelineOptions &opts, std::vector<PipelineContext> &contexts);

        /// Run the pipeline with a cancel token for cooperative cancellation.
        [[nodiscard]] Result<void> run(const PipelineOptions &opts, std::vector<PipelineContext> &contexts,
                         std::shared_ptr<std::atomic<bool>> cancelToken);

        /// Load the latest snapshot from the snapshot directory.
        /// Returns the loaded contexts and the step index they were saved at.
        [[nodiscard]] static Result<std::pair<std::vector<PipelineContext>, int>> loadLatestSnapshot(
            const QString &snapshotDir);

        /// Clean up old snapshots, keeping only the kMaxSnapshots most recent.
        static void cleanupOldSnapshots(const QString &snapshotDir);

        /// Check if any snapshot files exist in the given directory.
        /// Use this to detect residual snapshots from a previous interrupted run.
        [[nodiscard]] static bool hasLatestSnapshot(const QString &snapshotDir);

    signals:
        void progress(int step, int item, int totalItems, const QString &desc);
        void stepCompleted(int step, const QString &stepName);
        void itemDiscarded(const QString &itemId, const QString &reason);
        void cancelled();

    private:
        void saveSnapshot(const PipelineOptions &opts,
                          const std::vector<PipelineContext> &contexts,
                          int currentStep,
                          int totalSteps);
    };

} // namespace dsfw

// Backward compatibility
namespace dstools {
    using dsfw::PipelineRunner;
    using dsfw::PipelineOptions;
    using dsfw::StepConfig;
    using dsfw::ValidationCallback;
}
