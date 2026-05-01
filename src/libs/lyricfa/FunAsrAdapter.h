#pragma once

/// @file FunAsrAdapter.h
/// @brief Adapter bridging FunASR Model to the IInferenceEngine interface.

#include <dstools/IInferenceEngine.h>

#include <memory>
#include <mutex>

namespace FunAsr {
    class Model;
}

namespace LyricFA {

    /// @brief IInferenceEngine implementation wrapping FunAsr::Model for unified model lifecycle management.
    class FunAsrAdapter : public dstools::infer::IInferenceEngine {
    public:
        FunAsrAdapter() = default;
        ~FunAsrAdapter() override;

        FunAsrAdapter(const FunAsrAdapter &) = delete;
        FunAsrAdapter &operator=(const FunAsrAdapter &) = delete;

        /// @brief Check if the model is loaded.
        /// @return True if the model is open.
        bool isOpen() const override;

        /// @brief Return the engine identifier string.
        /// @return Engine name.
        const char *engineName() const override;

        /// @brief Load the ASR model.
        /// @param modelPath Path to the model directory.
        /// @param provider Execution provider (CPU/GPU).
        /// @param deviceId GPU device index.
        /// @return Result indicating success or failure.
        dstools::Result<void> load(const std::filesystem::path &modelPath,
                                   dstools::infer::ExecutionProvider provider, int deviceId) override;

        /// @brief Load the ASR model with error message output.
        /// @param modelPath Path to the model directory.
        /// @param provider Execution provider (CPU/GPU).
        /// @param deviceId GPU device index.
        /// @param errorMsg Error message on failure.
        /// @return True on success.
        bool load(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId,
                  std::string &errorMsg);

        /// @brief Release the loaded model.
        void unload() override;

        /// @brief Access the underlying FunAsr::Model.
        /// @return Pointer to the model, or nullptr if not loaded.
        FunAsr::Model *model() const;

    private:
        std::unique_ptr<FunAsr::Model> m_model; ///< Underlying FunASR model.
        mutable std::mutex m_mutex;              ///< Mutex for thread-safe access.
    };

} // namespace LyricFA
