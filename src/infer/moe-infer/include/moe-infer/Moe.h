/// @file Moe.h
/// @brief MOE mouth opening curve extraction engine.

#pragma once
#include "MoeGlobal.h"

#include <cstdint>
#include <dstools/ExecutionProvider.h>
#include <dstools/IInferenceEngine.h>
#include <dstools/Result.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace Moe {
    class MoeModel;

    struct MoeResult {
        std::vector<float> curve;
    };

    class MOE_INFER_EXPORT Moe : public dstools::infer::IInferenceEngine {
    public:
        Moe();
        explicit Moe(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId);
        ~Moe() override;

        bool isOpen() const override;
        void terminate() override;
        const char *engineName() const override {
            return "MOE";
        }

        dstools::Result<void> load(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider,
                                   int deviceId) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

        dstools::Result<MoeResult> predict(const std::vector<float> &waveform) const;

        int melSampleRate() const {
            return 16000;
        }
        int hopSize() const {
            return 320;
        }
        float frameTimestep() const {
            return 0.02f;
        }
        float vmin() const {
            return -0.15f;
        }
        float vmax() const {
            return 1.0f;
        }

    private:
        std::unique_ptr<MoeModel> m_model;
    };
} // namespace Moe