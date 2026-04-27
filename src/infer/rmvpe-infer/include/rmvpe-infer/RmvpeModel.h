#ifndef RMVPEMODEL_H
#define RMVPEMODEL_H

#include <filesystem>
#include <onnxruntime_cxx_api.h>
#include <rmvpe-infer/Provider.h>
#include <string>
#include <vector>

namespace Rmvpe
{
    class RmvpeModel {
    public:
        explicit RmvpeModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~RmvpeModel();

        bool is_open() const;
        // Forward pass through the model
        bool forward(const std::vector<float> &waveform_data, float threshold, std::vector<float> &f0,
                     std::vector<bool> &uv, std::string &msg);

        void terminate();

    private:
        Ort::Env m_env;
        Ort::RunOptions run_options;
        Ort::SessionOptions m_session_options;
        Ort::Session m_session;
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_waveform_input_name; // Name of the waveform input
        const char *m_threshold_input_name; // Name of the threshold input
        const char *m_f0_output_name; // Name of the f0 output
        const char *m_uv_output_name; // Name of the uv output

#ifdef _WIN_X86
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // namespace Rmvpe

#endif // RMVPEMODEL_H
