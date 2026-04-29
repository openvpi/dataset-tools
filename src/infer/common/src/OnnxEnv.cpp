#include <dstools/OnnxEnv.h>

#include <onnxruntime_cxx_api.h>

#include <iostream>

#ifdef ONNXRUNTIME_ENABLE_DML
#    include <dml_provider_factory.h>
#endif

namespace dstools::infer {

    Ort::Env &OnnxEnv::env() {
        static Ort::Env s_env(ORT_LOGGING_LEVEL_WARNING, "dstools");
        return s_env;
    }

    Ort::SessionOptions OnnxEnv::createSessionOptions(ExecutionProvider provider, int deviceId, int interOpThreads) {

        Ort::SessionOptions options;
        options.SetInterOpNumThreads(interOpThreads);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

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
                        std::cerr << "Failed to enable DML: " << appendStatus.GetErrorMessage() << std::endl;
                    }
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
                    ortApi.UpdateCUDAProviderOptions(cudaOptions.get(), keys, values, 2);
                    ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions.get());
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
                                                         int deviceId, QString *errorMsg) {

        try {
            auto options = createSessionOptions(provider, deviceId);
            auto session = std::make_unique<Ort::Session>(env(), modelPath.c_str(), options);
            return session;
        } catch (const Ort::Exception &e) {
            if (errorMsg) {
                *errorMsg = QString::fromStdString(e.what());
            }
            return nullptr;
        }
    }

} // namespace dstools::infer
