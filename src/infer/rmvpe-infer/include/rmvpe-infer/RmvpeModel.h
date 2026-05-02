/// @file RmvpeModel.h
/// @brief ONNX Runtime model for RMVPE pitch inference.

#pragma once
#include <filesystem>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <rmvpe-infer/RmvpeGlobal.h>
#include <rmvpe-infer/Provider.h>
#include <string>
#include <vector>

#include <dstools/OnnxModelBase.h>

namespace Rmvpe
{
    /// @brief CancellableOnnxModel that runs the RMVPE ONNX model to extract F0
    ///        and voicing from waveform data.
    class RMVPE_INFER_EXPORT RmvpeModel : public dstools::infer::CancellableOnnxModel {
    public:
        /// @brief Constructs and loads the RMVPE ONNX model.
        /// @param modelPath Path to the ONNX model file.
        /// @param provider Execution provider (CPU/DirectML/CUDA).
        /// @param device_id Device index for GPU execution.
        explicit RmvpeModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~RmvpeModel();

        /// @brief Returns true if the model is loaded.
        bool is_open() const { return isOpen(); }

        /// @brief Runs F0 extraction on waveform data.
        /// @param waveform_data Input audio samples.
        /// @param threshold Voicing detection threshold.
        /// @param[out] f0 Extracted F0 values per frame.
        /// @param[out] uv Voicing flags per frame (true = unvoiced).
        /// @param[out] msg Error message on failure.
        /// @return True on success.
        bool forward(const std::vector<float> &waveform_data, float threshold, std::vector<float> &f0,
                     std::vector<bool> &uv, std::string &msg);

    private:
        Ort::AllocatorWithDefaultOptions m_allocator;  ///< ONNX Runtime memory allocator.
    };

} // namespace Rmvpe
