/// @file MoeModel.h
/// @brief ONNX Runtime model for MOE mouth curve inference.

#pragma once
#include <cstdint>
#include <dsfw/infer/OnnxModelBase.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Moe {
    class MoeModel : public dsfw::infer::CancellableOnnxModel {
    public:
        explicit MoeModel(const std::filesystem::path &modelPath, dsfw::infer::ExecutionProvider provider,
                          int deviceId);
        ~MoeModel();

        bool is_open() const {
            return isOpen() && !m_initFailed;
        }
        bool initFailed() const {
            return m_initFailed;
        }

        dsfw::Result<std::vector<float>> predict(const float *waveform, int64_t numSamples);

        void terminate();

    private:
        bool m_initFailed = false;
    };
} // namespace Moe