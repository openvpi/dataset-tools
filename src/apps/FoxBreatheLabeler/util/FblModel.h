#ifndef FBLMODEL_H
#define FBLMODEL_H

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>
namespace FBL {

    class FblModel {
    public:
        explicit FblModel(const std::string &model_path);
        std::vector<float> forward(const std::vector<std::vector<float>> &input_data) const;

    private:
        void validate_input(const std::vector<float> &input_data) const;
        void validate_output(const std::vector<float> &output_data) const;

        std::unique_ptr<Ort::Env> m_env;
        std::unique_ptr<Ort::SessionOptions> m_session_options;
        std::unique_ptr<Ort::Session> m_session;
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_input_name;
        const char *m_output_name;
        std::vector<int64_t> m_input_dims;
        std::vector<int64_t> m_output_dims;

#ifdef _WIN_X86
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // FBL

#endif // FBLMODEL_H
