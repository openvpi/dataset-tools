#include <dstools/OnnxModelBase.h>

#include <fstream>

namespace dstools::infer {

    bool OnnxModelBase::loadSession(const std::filesystem::path &modelPath, std::string *errorMsg) {
        return loadSession(modelPath, m_provider, m_deviceId, *errorMsg);
    }

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

    bool OnnxModelBase::loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                      std::string *errorMsg) {
        return loadSessionTo(target, modelPath, m_provider, m_deviceId, *errorMsg);
    }

    Result<nlohmann::json> OnnxModelBase::loadConfig(const std::filesystem::path &modelDir) {
        const auto configPath = modelDir / "config.json";
        std::ifstream file(configPath);
        if (!file.is_open()) {
            return Err("Cannot open config.json in " + modelDir.string());
        }
        try {
            nlohmann::json config = nlohmann::json::parse(file);
            onConfigLoaded(config);
            return Ok(std::move(config));
        } catch (const nlohmann::json::parse_error &e) {
            return Err(std::string("Failed to parse config.json: ") + e.what());
        } catch (const nlohmann::json::type_error &e) {
            return Err(std::string("Type error in config.json: ") + e.what());
        }
    }

    void CancellableOnnxModel::terminate() {
        std::lock_guard lock(m_runMutex);
        if (m_activeRunOptions)
            m_activeRunOptions->SetTerminate();
    }

} // namespace dstools::infer
