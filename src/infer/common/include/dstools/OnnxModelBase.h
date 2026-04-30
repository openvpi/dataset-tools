#pragma once

/// @file OnnxModelBase.h
/// @brief Shared base class for ONNX Runtime model wrappers.

#include <dstools/ExecutionProvider.h>
#include <dstools/OnnxEnv.h>

#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

namespace dstools::infer {

    /// @brief Base class for single-session ONNX models.
    class OnnxModelBase {
    protected:
        std::unique_ptr<Ort::Session> m_session;
        Ort::MemoryInfo m_memoryInfo{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
        Ort::AllocatorWithDefaultOptions m_allocator;

        /// @brief Create and store a session from model file.
        bool loadSession(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId,
                         std::string &errorMsg);

        /// @brief Create a session into a custom target (for multi-session models).
        static bool loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                  ExecutionProvider provider, int deviceId, std::string &errorMsg);

    public:
        virtual ~OnnxModelBase() = default;

        bool isOpen() const {
            return m_session != nullptr;
        }

        void closeSession() {
            m_session.reset();
        }
    };

    /// @brief Mixin adding cancellable inference via Ort::RunOptions.
    class CancellableOnnxModel : public OnnxModelBase {
    protected:
        std::mutex m_runMutex;
        Ort::RunOptions *m_activeRunOptions = nullptr;

    public:
        void terminate();
    };

} // namespace dstools::infer
