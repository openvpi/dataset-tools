#include "HfaModel.h"

#include <iostream>

#ifdef ONNXRUNTIME_ENABLE_DML
#    include <dml_provider_factory.h>
#endif

namespace HFA {
    static bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);
    static bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

    HfaModel::HfaModel(const std::filesystem::path &model_Path, const ExecutionProvider provider, const int device_id)
        : m_env(Ort::Env(ORT_LOGGING_LEVEL_WARNING, "HfaModel")), m_session_options(Ort::SessionOptions()),
          m_model_session(nullptr) {
        m_session_options.SetInterOpNumThreads(4);

        // Choose execution provider based on the provided option
        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
            case ExecutionProvider::DML: {
                std::string errorMessage;
                if (!initDirectML(m_session_options, device_id, &errorMessage)) {
                    std::cout << "Failed to enable Dml: " << errorMessage << ". Falling back to CPU." << std::endl;
                } else {
                    std::cout << "Use Dml execution provider" << std::endl;
                }
                break;
            }
#endif

#if ONNXRUNTIME_ENABLE_CUDA
            case ExecutionProvider::CUDA: {
                std::string errorMessage;
                if (!initCUDA(m_session_options, device_id, &errorMessage)) {
                    std::cout << "Failed to enable CUDA: " << errorMessage << std::endl;
                } else {
                    std::cout << "Using CUDA execution provider" << std::endl;
                }
                break;
            }
#endif

            default:
                break;
        }

        // Create ONNX Runtime Session
        try {
#ifdef _WIN32
            m_model_session = new Ort::Session(m_env, model_Path.wstring().c_str(), m_session_options);
#else
            m_model_session = new Ort::Session(m_env, model_Path.c_str(), m_session_options);
#endif
        } catch (const Ort::Exception &e) {
            std::cout << "Failed to create session: " << e.what() << std::endl;
        }
    }

    HfaModel::~HfaModel() {
        delete m_model_session;
        m_input_name = {};
    }

    bool HfaModel::forward(const std::vector<std::vector<float>> &input_data, HfaLogits &result,
                           std::string &msg) const {
        if (input_data.empty()) {
            msg = "输入数据不能为空";
            return false;
        }

        const size_t batch_size = input_data.size();
        size_t max_len = 0;
        for (const auto &vec : input_data) {
            max_len = max(max_len, vec.size());
        }

        std::vector<float> flattened_input;
        flattened_input.reserve(batch_size * max_len);
        for (const auto &vec : input_data) {
            flattened_input.insert(flattened_input.end(), vec.begin(), vec.end());
            flattened_input.insert(flattened_input.end(), max_len - vec.size(), 0.0f);
        }

        const std::array<int64_t, 2> input_shape = {static_cast<int64_t>(batch_size), static_cast<int64_t>(max_len)};

        const Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            m_memoryInfo, flattened_input.data(), flattened_input.size(), input_shape.data(), input_shape.size());

        try {
            const std::vector<const char *> output_names = {m_predictor_output_name[0], m_predictor_output_name[1],
                                                            m_predictor_output_name[2]};

            auto predictor_outputs = m_model_session->Run(Ort::RunOptions{nullptr}, &m_input_name, &input_tensor, 1,
                                                          output_names.data(), output_names.size());

            auto parse_3d_output = [](Ort::Value &tensor) {
                const auto shape = tensor.GetTensorTypeAndShapeInfo().GetShape();

                if (shape.size() != 3) {
                    throw std::runtime_error("Expected 3D tensor (BCT) but got " + std::to_string(shape.size()) + "D");
                }

                const size_t batch = shape[0];
                const size_t classes = shape[1];
                const size_t time = shape[2];

                const float *data = tensor.GetTensorMutableData<float>();

                std::vector<std::vector<std::vector<float>>> output(batch);

                for (size_t b = 0; b < batch; ++b) {
                    output[b].resize(classes);

                    for (size_t c = 0; c < classes; ++c) {
                        output[b][c].resize(time);

                        for (size_t t = 0; t < time; ++t) {
                            const size_t index = b * (classes * time) + c * time + t;
                            output[b][c][t] = data[index];
                        }
                    }
                }

                return output;
            };

            auto parse_2d_output = [](Ort::Value &tensor) {
                const auto shape = tensor.GetTensorTypeAndShapeInfo().GetShape();
                const float *data = tensor.GetTensorMutableData<float>();
                const size_t batch = shape[0];
                const size_t time = shape[1];

                std::vector<std::vector<float>> output;
                output.resize(batch);

                for (size_t b = 0; b < batch; b++) {
                    output[b].assign(data + b * time, data + (b + 1) * time);
                }
                return output;
            };

            result.ph_frame_logits = parse_3d_output(predictor_outputs[0]);
            result.ph_edge_logits = parse_2d_output(predictor_outputs[1]);
            result.cvnt_logits = parse_3d_output(predictor_outputs[2]);

            return true;
        } catch (const Ort::Exception &e) {
            msg = "预测器推理错误: " + std::string(e.what());
            return false;
        }
    }

    static bool initDirectML(Ort::SessionOptions &options, const int deviceIndex, std::string *errorMessage) {
#ifdef ONNXRUNTIME_ENABLE_DML
        if (!options) {
            if (errorMessage) {
                *errorMessage = "SessionOptions must not be nullptr!";
            }
            return false;
        }

        if (deviceIndex < 0) {
            if (errorMessage) {
                *errorMessage = "GPU device index must be a non-negative integer!";
            }
            return false;
        }

        const OrtApi &ortApi = Ort::GetApi();
        const OrtDmlApi *ortDmlApi;
        const Ort::Status getApiStatus(
            (ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void **>(&ortDmlApi))));
        if (!getApiStatus.IsOK()) {
            // Failed to get DirectML API.
            if (errorMessage) {
                *errorMessage = getApiStatus.GetErrorMessage();
            }
            return false;
        }

        // Successfully get DirectML API
        options.DisableMemPattern();
        options.SetExecutionMode(ORT_SEQUENTIAL);

        const Ort::Status appendStatus(ortDmlApi->SessionOptionsAppendExecutionProvider_DML(options, deviceIndex));
        if (!appendStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = appendStatus.GetErrorMessage();
            }
            return false;
        }
        return true;
#else
        if (errorMessage) {
            *errorMessage = "The library is not built with DirectML support.";
        }
        return false;
#endif
    }

    static bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage) {
#ifdef ONNXRUNTIME_ENABLE_CUDA
        if (!options) {
            if (errorMessage) {
                *errorMessage = "SessionOptions must not be nullptr!";
            }
            return false;
        }

        if (deviceIndex < 0) {
            if (errorMessage) {
                *errorMessage = "GPU device index must be a non-negative integer!";
            }
            return false;
        }

        const OrtApi &ortApi = Ort::GetApi();

        OrtCUDAProviderOptionsV2 *cudaOptionsPtr = nullptr;
        Ort::Status createStatus(ortApi.CreateCUDAProviderOptions(&cudaOptionsPtr));

        // Currently, ORT C++ API does not have a wrapper for CUDAProviderOptionsV2.
        // Let the smart pointer take ownership of cudaOptionsPtr so it will be released when it
        // goes out of scope.
        std::unique_ptr<OrtCUDAProviderOptionsV2, decltype(ortApi.ReleaseCUDAProviderOptions)> cudaOptions(
            cudaOptionsPtr, ortApi.ReleaseCUDAProviderOptions);

        if (!createStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = createStatus.GetErrorMessage();
            }
            return false;
        }

        // The following block of code sets device_id
        {
            // Device ID from int to string
            auto cudaDeviceIdStr = std::to_string(deviceIndex);
            auto cudaDeviceIdCStr = cudaDeviceIdStr.c_str();

            constexpr int CUDA_OPTIONS_SIZE = 2;
            const char *cudaOptionsKeys[CUDA_OPTIONS_SIZE] = {"device_id", "cudnn_conv_algo_search"};
            const char *cudaOptionsValues[CUDA_OPTIONS_SIZE] = {cudaDeviceIdCStr, "DEFAULT"};
            Ort::Status updateStatus(ortApi.UpdateCUDAProviderOptions(cudaOptions.get(), cudaOptionsKeys,
                                                                      cudaOptionsValues, CUDA_OPTIONS_SIZE));
            if (!updateStatus.IsOK()) {
                if (errorMessage) {
                    *errorMessage = updateStatus.GetErrorMessage();
                }
                return false;
            }
        }
        Ort::Status appendStatus(ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions.get()));
        if (!appendStatus.IsOK()) {
            if (errorMessage) {
                *errorMessage = appendStatus.GetErrorMessage();
            }
            return false;
        }
        return true;
#else
        if (errorMessage) {
            *errorMessage = "The library is not built with CUDA support.";
        }
        return false;
#endif
    }
} // namespace HFA