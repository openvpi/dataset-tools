#pragma once
/// @file OnnxModelBase.h
/// @brief Base classes for ONNX Runtime model wrappers.

#include <dstools/ExecutionProvider.h>
#include <dstools/OnnxEnv.h>
#include <dstools/Result.h>

#include <nlohmann/json.hpp>
#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

namespace dstools::infer {

    /// @brief Base class for ONNX Runtime model wrappers.
    class OnnxModelBase {
    protected:
        std::unique_ptr<Ort::Session> m_session;
        Ort::MemoryInfo m_memoryInfo{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
        Ort::AllocatorWithDefaultOptions m_allocator;
        ExecutionProvider m_provider = ExecutionProvider::CPU;
        int m_deviceId = 0;

        OnnxModelBase() = default;
        OnnxModelBase(ExecutionProvider provider, int deviceId)
            : m_provider(provider), m_deviceId(deviceId) {}

        Result<void> loadSession(const std::filesystem::path &modelPath);
        Result<void> loadSession(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId);

        static Result<void> loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                          ExecutionProvider provider, int deviceId);

        Result<nlohmann::json> loadConfig(const std::filesystem::path &modelDir);

        virtual void onConfigLoaded(const nlohmann::json &config) { (void)config; }

    public:
        static Result<void> validateModelFile(const std::filesystem::path &modelPath);
        virtual ~OnnxModelBase() = default;

        /// @brief Check whether a model session is loaded.
        /// @return True if a session is active.
        bool isOpen() const {
            return m_session != nullptr;
        }

        /// @brief Close the current model session and free resources.
        void closeSession() {
            m_session.reset();
        }
    };

    /// @brief ONNX model base with support for cancelling running inference.
    class CancellableOnnxModel : public OnnxModelBase {
    protected:
        mutable std::mutex m_runMutex;
        mutable Ort::RunOptions *m_activeRunOptions = nullptr;

        CancellableOnnxModel() = default;
        CancellableOnnxModel(ExecutionProvider provider, int deviceId)
            : OnnxModelBase(provider, deviceId) {}

    public:
        /// @brief Request termination of a running inference operation.
        void terminate();
    };

} // namespace dstools::infer
