#pragma once

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

        bool loadSession(const std::filesystem::path &modelPath, std::string *errorMsg = nullptr);
        bool loadSession(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId,
                         std::string &errorMsg);

        static bool loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                  ExecutionProvider provider, int deviceId, std::string &errorMsg);
        bool loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                           std::string *errorMsg = nullptr);

        Result<nlohmann::json> loadConfig(const std::filesystem::path &modelDir);

        virtual void onConfigLoaded(const nlohmann::json &config) { (void)config; }

    public:
        virtual ~OnnxModelBase() = default;

        bool isOpen() const {
            return m_session != nullptr;
        }

        void closeSession() {
            m_session.reset();
        }
    };

    class CancellableOnnxModel : public OnnxModelBase {
    protected:
        mutable std::mutex m_runMutex;
        mutable Ort::RunOptions *m_activeRunOptions = nullptr;

        CancellableOnnxModel() = default;
        CancellableOnnxModel(ExecutionProvider provider, int deviceId)
            : OnnxModelBase(provider, deviceId) {}

    public:
        void terminate();
    };

} // namespace dstools::infer
