#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>
#include <dstools/TimePos.h>

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <map>
#include <vector>

namespace dstools {

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

    // Layer data
    std::map<QString, nlohmann::json> layers;
    ProcessorConfig globalConfig;

    // Status
    enum class Status { Active, Discarded, Error };
    Status status = Status::Active;
    QString discardReason;
    QString discardedAtStep;

    // Progress
    QStringList completedSteps;
    std::vector<StepRecord> stepHistory;

    // Core methods
    Result<TaskInput> buildTaskInput(const TaskSpec &spec) const;
    void applyTaskOutput(const TaskSpec &spec, const TaskOutput &output);

    // Serialization
    nlohmann::json toJson() const;
    static Result<PipelineContext> fromJson(const nlohmann::json &j);
};

} // namespace dstools
