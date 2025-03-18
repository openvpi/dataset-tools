#include <array>
#include <iostream>

#ifdef ONNXRUNTIME_ENABLE_DML
#include <dml_provider_factory.h>
#endif

#include <rmvpe-infer/RmvpeModel.h>

namespace Rmvpe
{
    static inline bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);
    static inline bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage = nullptr);

    RmvpeModel::RmvpeModel(const std::filesystem::path &modelPath, const ExecutionProvider provider,
                           const int device_id) :
        m_env(Ort::Env(ORT_LOGGING_LEVEL_WARNING, "RmvpeModel")), m_session_options(Ort::SessionOptions()),
        m_session(nullptr), m_waveform_input_name("waveform"), m_threshold_input_name("threshold"),
        m_f0_output_name("f0"), m_uv_output_name("uv") {

        m_session_options.SetInterOpNumThreads(4);

        // Choose execution provider based on the provided option
        switch (provider) {
#ifdef ONNXRUNTIME_ENABLE_DML
        case ExecutionProvider::DML:
        {
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
        case ExecutionProvider::CUDA:
        {
            std::string errorMessage;
            if (!initCUDA(m_session_options, device_id, &errorMessage)) {
                std::cout << "Failed to enable CUDA: " << errorMessage << ". Falling back to CPU." << std::endl;
            } else {
                std::cout << "Use CUDA execution provider" << std::endl;
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
            m_session = Ort::Session(m_env, modelPath.wstring().c_str(), m_session_options);
#else
            m_session = Ort::Session(m_env, modelPath.c_str(), m_session_options);
#endif
        } catch (const Ort::Exception &e) {
            std::cout << "Failed to create session: " << e.what() << std::endl;
        }
    }

    // Destructor: Release ONNX session
    RmvpeModel::~RmvpeModel() = default;

    bool RmvpeModel::is_open() const {
        return m_session != nullptr;
    }

    void RmvpeModel::terminate() { run_options.SetTerminate(); }

    // Forward pass through the model: takes waveform and threshold as inputs, returns f0 and uv as outputs
    bool RmvpeModel::forward(const std::vector<float> &waveform_data, float threshold, std::vector<float> &f0,
                             std::vector<bool> &uv, std::string &msg) {
        if (!m_session) {
            msg = "Session is not initialized.";
            return false;
        }
        try {
            const size_t n_samples = waveform_data.size();

            const std::array<int64_t, 2> input_waveform_shape = {1, static_cast<int64_t>(n_samples)};
            Ort::Value waveform_tensor = Ort::Value::CreateTensor<float>(
                m_memory_info, const_cast<float *>(waveform_data.data()), waveform_data.size(),
                input_waveform_shape.data(), input_waveform_shape.size());

            constexpr std::array<int64_t, 1> input_threshold_shape = {1}; // Scalar, shape is {1}
            Ort::Value threshold_tensor = Ort::Value::CreateTensor<float>(
                m_memory_info, &threshold, 1, input_threshold_shape.data(), input_threshold_shape.size());

            std::vector<float> f0_data;
            std::vector<int> uv_data;

            const char *input_names[] = {"waveform", "threshold"};
            const char *output_names[] = {"f0", "uv"};

            const Ort::Value input_tensors[] = {(std::move(waveform_tensor)), (std::move(threshold_tensor))};

            run_options.UnsetTerminate();
            auto output_tensors =
                m_session.Run(run_options, input_names, input_tensors, 2, // 输入：waveform 和 threshold
                               output_names, 2 // 输出：f0 和 uv
                );

            const float *f0_array = output_tensors.front().GetTensorMutableData<float>();
            f0.assign(f0_array, f0_array + output_tensors.front().GetTensorTypeAndShapeInfo().GetElementCount());

            const bool *uv_array = output_tensors.back().GetTensorMutableData<bool>();
            uv.assign(uv_array, uv_array + output_tensors.back().GetTensorTypeAndShapeInfo().GetElementCount());

            return true;
        }
        catch (const Ort::Exception &e) {
            msg = "Error during model inference: " + std::string(e.what());
            return false;
        }
    }

    static inline bool initDirectML(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage) {
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
        Ort::Status getApiStatus((ortApi.GetExecutionProviderApi(
            "DML", ORT_API_VERSION, reinterpret_cast<const void **>(&ortDmlApi))));
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

        Ort::Status appendStatus(
            ortDmlApi->SessionOptionsAppendExecutionProvider_DML(options, deviceIndex));
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

    static inline bool initCUDA(Ort::SessionOptions &options, int deviceIndex, std::string *errorMessage) {
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
        std::unique_ptr<OrtCUDAProviderOptionsV2, decltype(ortApi.ReleaseCUDAProviderOptions)>
            cudaOptions(cudaOptionsPtr, ortApi.ReleaseCUDAProviderOptions);

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
            const char *cudaOptionsKeys[CUDA_OPTIONS_SIZE] = {"device_id",
                                                              "cudnn_conv_algo_search"};
            const char *cudaOptionsValues[CUDA_OPTIONS_SIZE] = {cudaDeviceIdCStr, "DEFAULT"};
            Ort::Status updateStatus(ortApi.UpdateCUDAProviderOptions(
                cudaOptions.get(), cudaOptionsKeys, cudaOptionsValues, CUDA_OPTIONS_SIZE));
            if (!updateStatus.IsOK()) {
                if (errorMessage) {
                    *errorMessage = updateStatus.GetErrorMessage();
                }
                return false;
            }
        }
        Ort::Status appendStatus(
            ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions.get()));
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

} // namespace Rmvpe
