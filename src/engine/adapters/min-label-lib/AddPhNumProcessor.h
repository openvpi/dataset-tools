#pragma once

/// @file AddPhNumProcessor.h
/// @brief ITaskProcessor wrapper for PhNumCalculator.

#include <dsfw/ITaskProcessor.h>

namespace dstools {

    /// @brief Wraps PhNumCalculator as an ITaskProcessor for the task framework.
    class AddPhNumProcessor : public dsfw::ITaskProcessor {
    public:
        QString processorId() const override {
            return QStringLiteral("add-ph-num");
        }
        QString displayName() const override {
            return QStringLiteral("Add Phoneme Count");
        }
        dsfw::TaskSpec taskSpec() const override;
        dsfw::Result<void> initialize(ModelManager &mm, const dsfw::ProcessorConfig &config) override;
        void release() override;
        dsfw::Result<dsfw::TaskOutput> process(const dsfw::TaskInput &input) override;
    };

} // namespace dstools
