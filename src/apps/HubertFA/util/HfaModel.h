#ifndef FBLMODEL_H
#define FBLMODEL_H

#include <filesystem>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

#include "Provider.h"

namespace HFA {
    struct HfaLogits {
        std::vector<std::vector<std::vector<float>>> ph_frame_logits;
        std::vector<std::vector<float>> ph_edge_logits;
        std::vector<std::vector<std::vector<float>>> cvnt_logits;
    };

    class HfaModel {
    public:
        explicit HfaModel(const std::filesystem::path &encoder_Path, const std::filesystem::path &predictor_Path, ExecutionProvider provider, int device_id);
        ~HfaModel();
        bool forward(const std::vector<std::vector<float>> &input_data, HfaLogits &result, std::string &msg) const;

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_session_options;
        Ort::Session *m_encoder_session;
        Ort::Session *m_predictor_session;
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_input_name;
        const char *m_encoder_output_name = "input_feature";
        const char* m_predictor_output_name[3] = {"ph_frame_logits", "ph_edge_logits", "cvnt_logits"};

#ifdef _WIN_X86
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // Hfa

#endif // FBLMODEL_H
