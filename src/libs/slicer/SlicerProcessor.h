#pragma once

/// @file SlicerProcessor.h
/// @brief ITaskProcessor wrapper for SlicerService.

#include <dsfw/ITaskProcessor.h>

namespace dstools {

/// @brief Wraps SlicerService as an ITaskProcessor for the task framework.
class SlicerProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("slicer"); }
    QString displayName() const override { return QStringLiteral("Audio Slicer"); }
    TaskSpec taskSpec() const override;
    Result<void> initialize(IModelManager &mm, const ProcessorConfig &config) override;
    void release() override;
    Result<TaskOutput> process(const TaskInput &input) override;
};

} // namespace dstools
