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
    class RmvpePitchProcessor : public dsfw::ITaskProcessor {
    public:
        RmvpePitchProcessor();
        ~RmvpePitchProcessor() override;

        QString processorId() const override;
        QString displayName() const override;
        dsfw::TaskSpec taskSpec() const override;

        dsfw::Result<void> initialize(ModelManager &mm, const dsfw::ProcessorConfig &modelConfig) override;
        void release() override;

        dsfw::Result<dsfw::TaskOutput> process(const dsfw::TaskInput &input) override;

    private:
        std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe; ///< RMVPE engine instance.
        mutable std::mutex m_mutex;            ///< Serializes access to the engine.
    };

} // namespace dstools
