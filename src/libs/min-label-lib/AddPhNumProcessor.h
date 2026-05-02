#pragma once

/// @file AddPhNumProcessor.h
/// @brief ITaskProcessor wrapper for PhNumCalculator.

#include <dsfw/ITaskProcessor.h>

namespace dstools {

/// @brief Wraps PhNumCalculator as an ITaskProcessor for the task framework.
class AddPhNumProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("add-ph-num"); }
    QString displayName() const override { return QStringLiteral("Add Phoneme Count"); }
    TaskSpec taskSpec() const override;
    Result<void> initialize(IModelManager &mm, const ProcessorConfig &config) override;
    void release() override;
    Result<TaskOutput> process(const TaskInput &input) override;
};

} // namespace dstools
