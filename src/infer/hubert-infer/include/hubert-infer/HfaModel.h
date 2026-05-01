#ifndef HFAMODEL_H
#define HFAMODEL_H

#include <hubert-infer/HubertInferGlobal.h>

#include <filesystem>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

#include <dstools/OnnxModelBase.h>
#include <hubert-infer/Provider.h>

namespace HFA {
    struct HfaLogits {
        std::vector<std::vector<std::vector<float>>> ph_frame_logits;
        std::vector<std::vector<float>> ph_edge_logits;
        std::vector<std::vector<std::vector<float>>> cvnt_logits;
    };

    class HUBERT_INFER_EXPORT HfaModel : public dstools::infer::OnnxModelBase {
    public:
        explicit HfaModel(const std::filesystem::path &model_Path, ExecutionProvider provider, int device_id);
        ~HfaModel();
        bool forward(const std::vector<std::vector<float>> &input_data, HfaLogits &result, std::string &msg) const;

    private:
        Ort::AllocatorWithDefaultOptions m_allocator;

        const char *m_input_name = "waveform";
        const char *m_predictor_output_name[3] = {"ph_frame_logits", "ph_edge_logits", "cvnt_logits"};
    };

} // Hfa

#endif // HFAMODEL_H
