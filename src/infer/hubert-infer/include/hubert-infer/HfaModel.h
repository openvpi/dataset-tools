/// @file HfaModel.h
/// @brief ONNX Runtime model for HuBERT-FA inference.

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
    /// @brief Output logits from the HuBERT-FA model.
    struct HfaLogits {
        std::vector<std::vector<std::vector<float>>> ph_frame_logits; ///< Per-frame phoneme logits.
        std::vector<std::vector<float>> ph_edge_logits;               ///< Per-frame edge logits.
        std::vector<std::vector<std::vector<float>>> cvnt_logits;     ///< Consonant/non-lexical logits.
    };

    /// @brief OnnxModelBase subclass that loads and runs the HuBERT-FA ONNX model.
    class HUBERT_INFER_EXPORT HfaModel : public dstools::infer::OnnxModelBase {
    public:
        /// @brief Constructs and loads the HFA ONNX model.
        /// @param model_Path Path to the ONNX model file.
        /// @param provider Execution provider (CPU/DirectML/CUDA).
        /// @param device_id Device index for GPU execution.
        explicit HfaModel(const std::filesystem::path &model_Path, ExecutionProvider provider, int device_id);
        ~HfaModel();

        /// @brief Runs inference on waveform data.
        /// @param input_data Input waveform channels.
        /// @param[out] result Output logits.
        /// @param[out] msg Error message on failure.
        /// @return True on success.
        bool forward(const std::vector<std::vector<float>> &input_data, HfaLogits &result, std::string &msg) const;

    private:
        Ort::AllocatorWithDefaultOptions m_allocator;

        const char *m_input_name = "waveform";                                                    ///< Input tensor name.
        const char *m_predictor_output_name[3] = {"ph_frame_logits", "ph_edge_logits", "cvnt_logits"}; ///< Output tensor names.
    };

} // Hfa

#endif // HFAMODEL_H
