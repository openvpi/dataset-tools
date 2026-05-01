#pragma once

/// @file FunAsrProcessor.h
/// @brief FunASR-based speech recognition task processor.

#include <dsfw/ITaskProcessor.h>

#include <memory>
#include <mutex>

namespace LyricFA {
class Asr;
}

namespace dstools {

/// @brief ITaskProcessor implementation for FunASR speech recognition.
class FunAsrProcessor : public ITaskProcessor {
public:
    FunAsrProcessor();
    ~FunAsrProcessor() override;

    QString processorId() const override;
    QString displayName() const override;
    TaskSpec taskSpec() const override;

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &modelConfig) override;
    void release() override;

    Result<TaskOutput> process(const TaskInput &input) override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    mutable std::mutex m_mutex;
};

} // namespace dstools
