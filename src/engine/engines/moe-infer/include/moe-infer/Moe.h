/// @file Moe.h
/// @brief MOE mouth opening curve extraction engine.

#pragma once
#include "MoeGlobal.h"

#include <cstdint>
#include <dsfw/infer/IInferenceEngine.h>
#include <dsfw/Result.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace Moe {
class MoeModel;

struct MoeResult {
    std::vector<float> curve;
};

class MOE_INFER_EXPORT Moe : public dsfw::infer::IInferenceEngine {
public:
    Moe();
    explicit Moe(const std::filesystem::path& modelPath, dsfw::infer::ExecutionProvider provider, int deviceId);
    ~Moe() override;

    bool isOpen() const override;
    void terminate() override;
    const char* engineName() const override {
        return "MOE";
    }

    dsfw::Result<void>
    load(const std::filesystem::path& modelPath, dsfw::infer::ExecutionProvider provider, int deviceId) override;
    void unload() override;
    int64_t estimatedMemoryBytes() const override;

    dsfw::Result<MoeResult> predict(const std::vector<float>& waveform) const;

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
}  // namespace Moe