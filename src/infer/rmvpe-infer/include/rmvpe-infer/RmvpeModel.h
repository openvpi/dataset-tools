#ifndef RMVPEMODEL_H
#define RMVPEMODEL_H

#include <filesystem>
#include <memory>
#include <mutex>
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
        bool forward(const std::vector<float> &waveform_data, float threshold, std::vector<float> &f0,
                     std::vector<bool> &uv, std::string &msg);

        void terminate();

    private:
        std::mutex m_runMutex;
        Ort::RunOptions *m_activeRunOptions = nullptr;
        std::unique_ptr<Ort::Session> m_session;
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_waveform_input_name;
        const char *m_threshold_input_name;
        const char *m_f0_output_name;
        const char *m_uv_output_name;

#ifdef _WIN_X86
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // namespace Rmvpe
#endif // RMVPEMODEL_H
