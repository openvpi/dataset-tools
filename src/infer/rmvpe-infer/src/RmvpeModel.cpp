#include <array>
#include <iostream>

#include <dstools/OnnxEnv.h>
#include <rmvpe-infer/RmvpeModel.h>

namespace Rmvpe
{
    RmvpeModel::RmvpeModel(const std::filesystem::path &modelPath, const ExecutionProvider provider,
                           const int device_id)
        : CancellableOnnxModel(provider, device_id) {
#if defined(_M_IX86) || defined(__i386__)
        m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#endif
        auto loadResult = loadSession(modelPath);
        if (!loadResult) {
            std::cout << "Failed to create session: " << loadResult.error() << std::endl;
        }
    }

    RmvpeModel::~RmvpeModel() = default;

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
                m_memoryInfo, waveform_mutable.data(), waveform_mutable.size(),
                input_waveform_shape.data(), input_waveform_shape.size());

            constexpr std::array<int64_t, 1> input_threshold_shape = {1};
            Ort::Value threshold_tensor = Ort::Value::CreateTensor<float>(
                m_memoryInfo, &threshold, 1, input_threshold_shape.data(), input_threshold_shape.size());

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
