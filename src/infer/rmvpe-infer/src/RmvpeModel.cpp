#include <array>
#include <iostream>

#include <dstools/OnnxEnv.h>
#include <rmvpe-infer/RmvpeModel.h>

namespace Rmvpe
{
    RmvpeModel::RmvpeModel(const std::filesystem::path &modelPath, const ExecutionProvider provider,
                           const int device_id) :
        m_session(nullptr), m_waveform_input_name("waveform"), m_threshold_input_name("threshold"),
        m_f0_output_name("f0"), m_uv_output_name("uv") {

        auto sessionOptions = dstools::infer::OnnxEnv::createSessionOptions(provider, device_id);

        try {
#ifdef _WIN32
            m_session = std::make_unique<Ort::Session>(dstools::infer::OnnxEnv::env(), modelPath.wstring().c_str(), sessionOptions);
#else
            m_session = std::make_unique<Ort::Session>(dstools::infer::OnnxEnv::env(), modelPath.c_str(), sessionOptions);
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

    void RmvpeModel::terminate() {
        std::lock_guard lock(m_runMutex);
        if (m_activeRunOptions) m_activeRunOptions->SetTerminate();
    }

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
            std::vector<float> waveform_mutable(waveform_data.begin(), waveform_data.end());
            Ort::Value waveform_tensor = Ort::Value::CreateTensor<float>(
                m_memory_info, waveform_mutable.data(), waveform_mutable.size(),
                input_waveform_shape.data(), input_waveform_shape.size());

            constexpr std::array<int64_t, 1> input_threshold_shape = {1}; // Scalar, shape is {1}
            Ort::Value threshold_tensor = Ort::Value::CreateTensor<float>(
                m_memory_info, &threshold, 1, input_threshold_shape.data(), input_threshold_shape.size());

            std::vector<float> f0_data;
            std::vector<int> uv_data;

            const char *input_names[] = {"waveform", "threshold"};
            const char *output_names[] = {"f0", "uv"};

            const Ort::Value input_tensors[] = {(std::move(waveform_tensor)), (std::move(threshold_tensor))};

            Ort::RunOptions runOptions;
            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = &runOptions;
            }
            auto output_tensors =
                m_session->Run(runOptions, input_names, input_tensors, 2,
                               output_names, 2
                );
            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = nullptr;
            }

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

} // namespace Rmvpe
