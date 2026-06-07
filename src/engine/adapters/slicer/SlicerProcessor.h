#pragma once

/// @file SlicerProcessor.h
/// @brief ITaskProcessor wrapper for SlicerService.

#include <dsfw/ITaskProcessor.h>

namespace dstools {

    /// @brief Wraps SlicerService as an ITaskProcessor for the task framework.
    class SlicerProcessor : public dsfw::ITaskProcessor {
    public:
        QString processorId() const override {
            return QStringLiteral("slicer");
        }
        QString displayName() const override {
            return QStringLiteral("Audio Slicer");
        }
        dsfw::TaskSpec taskSpec() const override;
        dsfw::Result<void> initialize(ModelManager &mm, const dsfw::ProcessorConfig &config) override;
        void release() override;
        dsfw::Result<dsfw::TaskOutput> process(const dsfw::TaskInput &input) override;
    };

} // namespace dstools
