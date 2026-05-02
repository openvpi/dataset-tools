#pragma once

/// @file RmvpePitchProcessor.h
/// @brief RMVPE-based pitch extraction task processor.

#include <dsfw/ITaskProcessor.h>

#include <memory>
#include <mutex>

namespace Rmvpe {
class Rmvpe;
}

namespace dstools {

/// @brief ITaskProcessor implementation for RMVPE F0 pitch extraction.
class RmvpePitchProcessor : public ITaskProcessor {
public:
    RmvpePitchProcessor();
    ~RmvpePitchProcessor() override;

    QString processorId() const override;
    QString displayName() const override;
    TaskSpec taskSpec() const override;

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &modelConfig) override;
    void release() override;

    Result<TaskOutput> process(const TaskInput &input) override;

private:
    std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe; ///< RMVPE engine instance.
    mutable std::mutex m_mutex;             ///< Serializes access to the engine.
};

} // namespace dstools
