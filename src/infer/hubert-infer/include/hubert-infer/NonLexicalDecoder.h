/// @file NonLexicalDecoder.h
/// @brief Non-lexical sound detector for breath and silence segments.

#pragma once

#include <hubert-infer/HubertInferGlobal.h>
#include <hubert-infer/AlignWord.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace HFA {

    /// @brief Decodes consonant logits to detect non-lexical sounds (AP/SP/breath)
    ///        using probability thresholding and gap merging.
    class HUBERT_INFER_EXPORT NonLexicalDecoder {
    private:
        /// @brief Mel spectrogram configuration parameters.
        struct MelSpecConfig {
            int hop_length;   ///< Hop length in samples.
            int sample_rate;  ///< Audio sample rate in Hz.
        };

        std::vector<std::string> non_lexical_phs_;      ///< Non-lexical phoneme class names.
        MelSpecConfig melspec_config_;                    ///< Mel spectrogram configuration.
        float frame_length_;                             ///< Duration of a single frame in seconds.
        std::vector<std::vector<float>> cvnt_probs_;     ///< Per-frame class probabilities after softmax.

        /// @brief Computes softmax over a vector.
        static std::vector<float> softmax(const std::vector<float> &x);

        /// @brief Detects non-lexical word segments from per-frame probabilities.
        /// @param prob Per-frame probability for a single class.
        /// @param threshold Minimum probability to trigger detection.
        /// @param max_gap Maximum gap in frames to merge adjacent detections.
        /// @param mix_frames Minimum segment length in frames.
        /// @param tag Label for detected segments.
        /// @return Detected non-lexical word segments.
        WordList non_lexical_words(const std::vector<float> &prob, float threshold = 0.5f, int max_gap = 5,
                                   int mix_frames = 10, const std::string &tag = "") const;

    public:
        /// @brief Constructs the decoder with class names and mel spectrogram config.
        /// @param class_names Non-lexical phoneme class names.
        /// @param mel_spec_config Mel spectrogram parameters (hop_length, sample_rate).
        NonLexicalDecoder(const std::vector<std::string> &class_names,
                          const std::map<std::string, float> &mel_spec_config);

        /// @brief Decodes consonant logits into per-class non-lexical word lists.
        /// @param cvnt_logits Raw consonant logits from the model.
        /// @param wav_length Duration of the audio in seconds.
        /// @param non_lexical_phonemes Phoneme labels to detect (empty = all classes).
        /// @return One WordList per detected non-lexical class.
        std::vector<WordList> decode(const std::vector<std::vector<std::vector<float>>> &cvnt_logits, float wav_length,
                                     const std::vector<std::string> &non_lexical_phonemes = {});

        /// @brief Returns per-frame class probabilities.
        const std::vector<std::vector<float>> &get_cvnt_probs() const {
            return cvnt_probs_;
        }
        /// @brief Returns non-lexical phoneme class names.
        const std::vector<std::string> &get_non_lexical_phs() const {
            return non_lexical_phs_;
        }
        /// @brief Returns the frame duration in seconds.
        float get_frame_length() const {
            return frame_length_;
        }
    };

} // namespace HFA
