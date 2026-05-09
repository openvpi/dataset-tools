#include <dstools/OnnxEnv.h>
#include <dstools/OnnxModelBase.h>
#include <dstools/PathEncoding.h>

#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <iostream>

#ifdef ONNXRUNTIME_ENABLE_DML
#    include <dml_provider_factory.h>
#endif

namespace dstools::infer {

    static Ort::Env *s_customEnv = nullptr;

    Ort::Env &OnnxEnv::env() {
        if (s_customEnv)
            return *s_customEnv;
        static Ort::Env s_env(ORT_LOGGING_LEVEL_WARNING, "dstools");
        return s_env;
    }

    void OnnxEnv::setEnv(Ort::Env *env) {
        s_customEnv = env;
    }

    void OnnxEnv::resetEnv() {
        s_customEnv = nullptr;
    }

    Ort::SessionOptions OnnxEnv::createSessionOptions(ExecutionProvider provider, [[maybe_unused]] int deviceId, int interOpThreads) {

        Ort::SessionOptions options;
        options.SetInterOpNumThreads(interOpThreads);
 
        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
            case ExecutionProvider::DML: {
                const OrtApi &ortApi = Ort::GetApi();
                const OrtDmlApi *ortDmlApi = nullptr;
                Ort::Status getApiStatus(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION,
                                                                        reinterpret_cast<const void **>(&ortDmlApi)));
                if (getApiStatus.IsOK() && ortDmlApi) {
                    options.DisableMemPattern();
                    options.SetExecutionMode(ORT_SEQUENTIAL);
                    Ort::Status appendStatus(ortDmlApi->SessionOptionsAppendExecutionProvider_DML(options, deviceId));
                    if (!appendStatus.IsOK()) {
                        std::cerr << "Warning: Failed to enable DML execution provider: "
                                  << appendStatus.GetErrorMessage() << std::endl;
                        std::cerr << "Falling back to CPU. Performance may be significantly reduced." << std::endl;
                    }
                } else {
                    std::cerr << "Warning: DML execution provider API not available." << std::endl;
                }
                break;
            }
#endif

#ifdef ONNXRUNTIME_ENABLE_CUDA
            case ExecutionProvider::CUDA: {
                const OrtApi &ortApi = Ort::GetApi();
                OrtCUDAProviderOptionsV2 *cudaOptionsPtr = nullptr;
                Ort::Status createStatus(ortApi.CreateCUDAProviderOptions(&cudaOptionsPtr));
                if (createStatus.IsOK()) {
                    std::unique_ptr<OrtCUDAProviderOptionsV2, decltype(ortApi.ReleaseCUDAProviderOptions)> cudaOptions(
                        cudaOptionsPtr, ortApi.ReleaseCUDAProviderOptions);
                    auto deviceIdStr = std::to_string(deviceId);
                    const char *keys[] = {"device_id", "cudnn_conv_algo_search"};
                    const char *values[] = {deviceIdStr.c_str(), "DEFAULT"};
                    OrtStatusPtr updateStatus = ortApi.UpdateCUDAProviderOptions(cudaOptions.get(), keys, values, 2);
                    if (updateStatus) {
                        DSFW_LOG_WARN("infer", ("CUDA UpdateCUDAProviderOptions failed: " + std::string(ortApi.GetErrorMessage(updateStatus))).c_str());
                        ortApi.ReleaseStatus(updateStatus);
                    }
                    OrtStatusPtr appendStatus = ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions.get());
                    if (appendStatus) {
                        DSFW_LOG_WARN("infer", ("Failed to enable CUDA execution provider: " + std::string(ortApi.GetErrorMessage(appendStatus))).c_str());
                        DSFW_LOG_WARN("infer", "Falling back to CPU. Performance may be significantly reduced.");
                        ortApi.ReleaseStatus(appendStatus);
                    }
                } else {
                    DSFW_LOG_WARN("infer", ("CreateCUDAProviderOptions failed: " + std::string(createStatus.GetErrorMessage())).c_str());
                }
                break;
            }
#endif

            default:
                break;
        }

        return options;
    }

    std::unique_ptr<Ort::Session> OnnxEnv::createSession(const std::wstring &modelPath, ExecutionProvider provider,
                                                         int deviceId, std::string *errorMsg) {

        auto validation = OnnxModelBase::validateModelFile(std::filesystem::path(modelPath));
        if (!validation) {
            if (errorMsg) {
                *errorMsg = validation.error();
            }
            return nullptr;
        }

        try {
            auto options = createSessionOptions(provider, deviceId);
#ifdef _WIN32
            auto session = std::make_unique<Ort::Session>(env(), modelPath.c_str(), options);
#else
            auto modelPathStr = dstools::pathToUtf8(std::filesystem::path(modelPath));
            auto session = std::make_unique<Ort::Session>(env(), modelPathStr.c_str(), options);
#endif
            return session;
        } catch (const Ort::Exception &e) {
            if (errorMsg) {
                *errorMsg = e.what();
            }
            return nullptr;
        }
    }

} // namespace dstools::infer
