#ifndef FBLMODEL_H
#define FBLMODEL_H

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
namespace FBL {

    class FblModel {
    public:
        explicit FblModel(const std::string &model_path);
        bool forward(const std::vector<std::vector<float>> &input_data, std::vector<float> &result,
                     std::string &msg) const;

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_session_options;
        Ort::Session *m_session;
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_input_name;
        const char *m_output_name;

#ifdef _WIN_X86
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // FBL

#endif // FBLMODEL_H
