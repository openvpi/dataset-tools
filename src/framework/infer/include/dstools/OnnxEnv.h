#pragma once
/// @file OnnxEnv.h
/// @brief Global ONNX Runtime environment singleton and session factory.

#include <dstools/ExecutionProvider.h>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <string>

namespace dstools::infer {

    /// @brief Global ONNX Runtime environment (singleton).
    ///
    /// Replaces duplicate Ort::Env creation in each inference library.
    class OnnxEnv {
    public:
        /// @brief Get the process-unique Ort::Env instance.
        /// @return Reference to the global environment.
        static Ort::Env &env();

        /// @brief Create configured session options for a given provider.
        /// @param provider Execution provider (CPU, DML, CUDA).
        /// @param deviceId Device index for GPU providers.
        /// @param interOpThreads Number of inter-op parallelism threads.
        /// @return Configured SessionOptions.
        static Ort::SessionOptions createSessionOptions(ExecutionProvider provider = ExecutionProvider::CPU,
                                                        int deviceId = 0, int interOpThreads = 4);

        /// @brief Create an ONNX Runtime session from a model file.
        /// @param modelPath Path to the ONNX model.
        /// @param provider Execution provider.
        /// @param deviceId Device index for GPU providers.
        /// @param errorMsg Filled with error message on failure.
        /// @return Session on success, nullptr on failure.
        static std::unique_ptr<Ort::Session> createSession(const std::wstring &modelPath,
                                                           ExecutionProvider provider = ExecutionProvider::CPU,
                                                           int deviceId = 0, std::string *errorMsg = nullptr);

    private:
        OnnxEnv() = delete;
    };

} // namespace dstools::infer
