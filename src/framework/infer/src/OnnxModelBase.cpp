#include <dstools/OnnxModelBase.h>
#include <dstools/PathEncoding.h>

#include <fstream>

namespace dstools::infer {

    Result<void> OnnxModelBase::validateModelFile(const std::filesystem::path &modelPath) {
        std::error_code ec;
        if (!std::filesystem::exists(modelPath, ec) || ec) {
            return Err("Model file does not exist: " + dstools::pathToUtf8(modelPath));
        }

        const auto fileSize = std::filesystem::file_size(modelPath, ec);
        if (ec) {
            return Err("Cannot read model file size: " + dstools::pathToUtf8(modelPath));
        }
        if (fileSize == 0) {
            return Err("Model file is empty: " + dstools::pathToUtf8(modelPath));
        }
        if (fileSize < 16) {
            return Err("Model file is too small to be a valid ONNX model (" +
                       std::to_string(fileSize) + " bytes): " + dstools::pathToUtf8(modelPath));
        }

        std::ifstream file(modelPath, std::ios::binary);
        if (!file.is_open()) {
            return Err("Cannot open model file for reading: " + dstools::pathToUtf8(modelPath));
        }

        char header[4] = {};
        if (!file.read(header, 4)) {
            return Err("Cannot read model file header: " + dstools::pathToUtf8(modelPath));
        }

        static constexpr char onnxMagic[] = {0x08, 0x07};
        if (header[0] == onnxMagic[0] && header[1] == onnxMagic[1]) {
            return Ok();
        }

        if (header[0] == '{' || header[0] == '[') {
            return Err("Model file appears to be JSON/text format, not a binary ONNX model: " +
                       dstools::pathToUtf8(modelPath));
        }

        return Ok();
    }

    Result<void> OnnxModelBase::loadSession(const std::filesystem::path &modelPath) {
        return loadSession(modelPath, m_provider, m_deviceId);
    }

    Result<void> OnnxModelBase::loadSession(const std::filesystem::path &modelPath, const ExecutionProvider provider,
                                             const int deviceId) {
        return loadSessionTo(m_session, modelPath, provider, deviceId);
    }

    Result<void> OnnxModelBase::loadSessionTo(std::unique_ptr<Ort::Session> &target, const std::filesystem::path &modelPath,
                                               const ExecutionProvider provider, const int deviceId) {
        auto validation = validateModelFile(modelPath);
        if (!validation) {
            return validation;
        }

        try {
            auto options = OnnxEnv::createSessionOptions(provider, deviceId);
#ifdef _WIN32
            target = std::make_unique<Ort::Session>(OnnxEnv::env(), modelPath.wstring().c_str(), options);
#else
            target = std::make_unique<Ort::Session>(OnnxEnv::env(), modelPath.c_str(), options);
#endif
            return Ok();
        } catch (const Ort::Exception &e) {
            return Err(std::string(e.what()));
        } catch (const std::exception &e) {
            return Err(std::string(e.what()));
        }
    }

    Result<nlohmann::json> OnnxModelBase::loadConfig(const std::filesystem::path &modelDir) {
        const auto configPath = modelDir / "config.json";
        std::ifstream file(configPath);
        if (!file.is_open()) {
            return Err<nlohmann::json>("Cannot open config.json in " + dstools::pathToUtf8(modelDir));
        }
        try {
            nlohmann::json config = nlohmann::json::parse(file);
            onConfigLoaded(config);
            return Ok(std::move(config));
        } catch (const nlohmann::json::parse_error &e) {
            return Err<nlohmann::json>(std::string("Failed to parse config.json: ") + e.what());
        } catch (const nlohmann::json::type_error &e) {
            return Err<nlohmann::json>(std::string("Type error in config.json: ") + e.what());
        }
    }

    void CancellableOnnxModel::terminate() {
        std::lock_guard lock(m_runMutex);
        if (m_activeRunOptions)
            m_activeRunOptions->SetTerminate();
    }

} // namespace dstools::infer
