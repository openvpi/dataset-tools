#pragma once

#include <dsfw/PipelineContext.h>
#include <dsfw/ITaskProcessor.h>
#include <dsfw/IAudioPreprocessor.h>
#include <dsfw/IFormatAdapter.h>

#include <QObject>

#include <functional>
#include <memory>
#include <vector>

namespace dstools {

using ValidationCallback = std::function<
    PipelineContext::Status(const PipelineContext &ctx, const TaskSpec &spec, QString &reason)>;

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
};

class PipelineRunner : public QObject {
    Q_OBJECT
public:
    explicit PipelineRunner(QObject *parent = nullptr);

    /// Run the pipeline on a list of contexts.
    /// Each context represents one item (e.g. one audio slice).
    Result<void> run(const PipelineOptions &opts,
                     std::vector<PipelineContext> &contexts);

signals:
    void progress(int step, int item, int totalItems, const QString &desc);
    void stepCompleted(int step, const QString &stepName);
    void itemDiscarded(const QString &itemId, const QString &reason);
};

} // namespace dstools
