#pragma once

/// @file HubertAlignmentProcessor.h
/// @brief HuBERT-based forced alignment task processor.

#include <dsfw/ITaskProcessor.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace HFA {
    class HFA;
}

namespace dstools {

    /// @brief ITaskProcessor implementation for HuBERT-FA phoneme alignment.
    class HubertAlignmentProcessor : public dsfw::ITaskProcessor {
    public:
        HubertAlignmentProcessor();
        ~HubertAlignmentProcessor() override;

        QString processorId() const override;
        QString displayName() const override;
        dsfw::TaskSpec taskSpec() const override;
        dsfw::ProcessorConfig capabilities() const noexcept override;

        dsfw::Result<void> initialize(ModelManager &mm, const dsfw::ProcessorConfig &modelConfig) override;
        void release() override;

        dsfw::Result<dsfw::TaskOutput> process(const dsfw::TaskInput &input) override;

    private:
        std::unique_ptr<HFA::HFA> m_hfa;
        std::string m_language = "zh";
        std::vector<std::string> m_nonSpeechPh = {"AP", "SP"};
        mutable std::mutex m_mutex;
    };

} // namespace dstools
