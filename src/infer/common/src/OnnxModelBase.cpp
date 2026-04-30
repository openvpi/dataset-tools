#include <dstools/OnnxModelBase.h>

namespace dstools::infer {

    bool OnnxModelBase::loadSession(const std::filesystem::path &modelPath, const ExecutionProvider provider,
                                    const int deviceId, std::string &errorMsg) {
        return loadSessionTo(m_session, modelPath, provider, deviceId, errorMsg);
    }

    bool OnnxModelBase::loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                      const ExecutionProvider provider, const int deviceId, std::string &errorMsg) {
        try {
            auto options = OnnxEnv::createSessionOptions(provider, deviceId);
#ifdef _WIN32
            target = std::make_unique<Ort::Session>(OnnxEnv::env(), modelPath.wstring().c_str(), options);
#else
            target = std::make_unique<Ort::Session>(OnnxEnv::env(), modelPath.c_str(), options);
#endif
            return true;
        } catch (const Ort::Exception &e) {
            errorMsg = e.what();
            return false;
        }
    }

    void CancellableOnnxModel::terminate() {
        std::lock_guard lock(m_runMutex);
        if (m_activeRunOptions)
            m_activeRunOptions->SetTerminate();
    }

} // namespace dstools::infer
