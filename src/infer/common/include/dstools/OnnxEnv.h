#pragma once

#include <QString>
#include <dstools/ExecutionProvider.h>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <string>

namespace dstools::infer {

    /// Global ONNX Runtime environment (singleton).
    /// Replaces duplicate Ort::Env creation in each inference library.
    class OnnxEnv {
    public:
        /// Get global Ort::Env (process-unique)
        static Ort::Env &env();

        /// Create configured SessionOptions
        static Ort::SessionOptions createSessionOptions(ExecutionProvider provider = ExecutionProvider::CPU,
                                                        int deviceId = 0, int interOpThreads = 4);

        /// Create Session from model file
        /// @param errorMsg Filled with error message on failure
        /// @return Session on success, nullptr on failure
        static std::unique_ptr<Ort::Session> createSession(const std::wstring &modelPath,
                                                           ExecutionProvider provider = ExecutionProvider::CPU,
                                                           int deviceId = 0, QString *errorMsg = nullptr);

    private:
        OnnxEnv() = delete;
    };

} // namespace dstools::infer
