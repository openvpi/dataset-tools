/// @file Rmvpe.h
/// @brief RMVPE pitch (F0) extraction engine.

#pragma once
#include <filesystem>
#include <functional>

#include <audio-util/SndfileVio.h>
#include <dstools/IInferenceEngine.h>
#include <dstools/Result.h>
#include <rmvpe-infer/Provider.h>
#include <rmvpe-infer/RmvpeGlobal.h>

namespace Rmvpe
{
    class RmvpeModel;

    /// @brief F0 extraction result with offset, frequency values, and voicing flags.
    struct RmvpeRes {
        float offset;              ///< Time offset of the first frame in seconds.
        std::vector<float> f0;     ///< F0 frequency values per frame (Hz, 0 = unvoiced).
        std::vector<bool> uv;      ///< Voicing flags (true = unvoiced).
    };

    /// @brief IInferenceEngine implementation for RMVPE-based pitch extraction
    ///        with cancellation support.
    class RMVPE_INFER_EXPORT Rmvpe : public dstools::infer::IInferenceEngine {
    public:
        Rmvpe();
        /// @brief Constructs and loads the RMVPE model.
        /// @param modelPath Path to the ONNX model file.
        /// @param provider Execution provider (CPU/DirectML/CUDA).
        /// @param device_id Device index for GPU execution.
        explicit Rmvpe(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~Rmvpe() override;

        /// @brief Returns true if the model is loaded.
        bool is_open() const;
        bool isOpen() const override { return is_open(); }

        /// @brief Extracts F0 from an audio file.
        /// @param filepath Path to the audio file.
        /// @param threshold Voicing threshold.
        /// @param[out] res Extraction results per channel.
        /// @param progressChanged Callback for progress updates (percentage).
        /// @return Success or error.
        dstools::Result<void> get_f0(const std::filesystem::path &filepath, float threshold, std::vector<RmvpeRes> &res,
                                     const std::function<void(int)> &progressChanged) const;

        /// @brief Cancels a running inference.
        void terminate() override;

        const char *engineName() const override { return "RMVPE"; }

        /// @brief Loads the model from the given path.
        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        /// @brief Unloads the model and frees resources.
        void unload() override;
        /// @brief Returns estimated GPU/CPU memory usage in bytes.
        int64_t estimatedMemoryBytes() const override;

    private:
        std::unique_ptr<RmvpeModel> m_rmvpe; ///< ONNX model instance.
    };
} // namespace Rmvpe
