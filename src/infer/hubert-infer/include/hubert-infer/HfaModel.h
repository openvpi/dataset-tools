#ifndef HFAMODEL_H
#define HFAMODEL_H

#include <hubert-infer/HubertInferGlobal.h>

#include <filesystem>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

#include <hubert-infer/Provider.h>

namespace HFA {
    struct HfaLogits {
        std::vector<std::vector<std::vector<float>>> ph_frame_logits;
        std::vector<std::vector<float>> ph_edge_logits;
        std::vector<std::vector<std::vector<float>>> cvnt_logits;
    };

    class HUBERT_INFER_EXPORT HfaModel {
    public:
        explicit HfaModel(const std::filesystem::path &model_Path, ExecutionProvider provider, int device_id);
        ~HfaModel();
        bool forward(const std::vector<std::vector<float>> &input_data, HfaLogits &result, std::string &msg) const;

    private:
        std::unique_ptr<Ort::Session> m_model_session;
        Ort::AllocatorWithDefaultOptions m_allocator;

        const char *m_input_name = "waveform";
        const char *m_predictor_output_name[3] = {"ph_frame_logits", "ph_edge_logits", "cvnt_logits"};

#ifdef _WIN_X86
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // Hfa

#endif // HFAMODEL_H
