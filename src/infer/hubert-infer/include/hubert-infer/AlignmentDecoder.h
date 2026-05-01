/// @file AlignmentDecoder.h
/// @brief Viterbi-based phoneme alignment decoder for HuBERT-FA.

#pragma once

#include <hubert-infer/HubertInferGlobal.h>
#include <hubert-infer/AlignWord.h>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace HFA {

    /// @brief Decodes phoneme frame logits and edge logits into time-aligned word/phone
    ///        boundaries using dynamic programming.
    class HUBERT_INFER_EXPORT AlignmentDecoder {
        /// @brief Mel spectrogram configuration parameters.
        struct MelSpecConfig {
            int hop_length;   ///< Hop length in samples.
            int sample_rate;  ///< Audio sample rate in Hz.
        };

        std::map<std::string, int> vocab_;             ///< Phoneme-to-index vocabulary.
        MelSpecConfig mel_spec_config_;                 ///< Mel spectrogram configuration.
        float frame_length_;                            ///< Duration of a single frame in seconds.

        std::vector<int> ph_seq_id_;                    ///< Phoneme sequence as vocabulary indices.
        std::vector<int> ph_idx_seq_;                   ///< Decoded phoneme index sequence per frame.
        std::vector<std::vector<float>> ph_frame_pred_; ///< Per-frame phoneme probability predictions.
        std::vector<int> ph_time_int_pred_;             ///< Predicted phoneme boundary frame indices.
        std::vector<float> edge_prob_;                  ///< Edge (transition) probabilities per frame.
        std::vector<float> frame_confidence_;           ///< Confidence score per frame.
        float total_confidence_ = 0.0f;                 ///< Overall alignment confidence.

        /// @brief Applies the sigmoid activation function.
        static float sigmoid(float x);

        /// @brief Computes 2D softmax along the given axis.
        static std::vector<std::vector<float>> softmax_2d(const std::vector<std::vector<float>> &x, int axis);
        /// @brief Computes 2D log-softmax along the given axis.
        static std::vector<std::vector<float>> log_softmax_2d(const std::vector<std::vector<float>> &x, int axis);

        /// @brief Runs the Viterbi forward pass for dynamic programming alignment.
        /// @param T Number of time frames.
        /// @param S Number of phonemes in sequence.
        /// @param prob_log Log-probability matrix.
        /// @param edge_prob Edge transition probabilities.
        /// @param curr_ph_max_prob_log Running max log-probability per phoneme.
        /// @param dp Dynamic programming table.
        /// @param ph_seq_id Phoneme sequence indices.
        /// @param prob3_pad_len Padding length for probability matrix.
        /// @param backtrack_s Backtracking table for path recovery.
        static void forward_pass(int T, int S, const std::vector<std::vector<float>> &prob_log,
                                 const std::vector<float> &edge_prob, std::vector<float> &curr_ph_max_prob_log,
                                 std::vector<std::vector<float>> &dp, const std::vector<int> &ph_seq_id,
                                 int prob3_pad_len, std::vector<std::vector<int>> &backtrack_s);

        /// @brief Decodes phoneme boundaries from log-probabilities via Viterbi backtracking.
        /// @param ph_seq_id Phoneme sequence indices.
        /// @param ph_prob_log Log-probability matrix [vocab_size][T].
        /// @param edge_prob Edge transition probabilities.
        /// @param[out] ph_idx_seq Decoded phoneme index per frame.
        /// @param[out] ph_time_int Phoneme boundary frame indices.
        /// @param[out] frame_confidence Per-frame confidence scores.
        /// @param T Number of time frames.
        static void _decode(const std::vector<int> &ph_seq_id,
                            const std::vector<std::vector<float>> &ph_prob_log, // [vocab_size][T]
                            const std::vector<float> &edge_prob, std::vector<int> &ph_idx_seq,
                            std::vector<int> &ph_time_int, std::vector<float> &frame_confidence, int T);

    public:
        /// @brief Constructs the decoder with a phoneme vocabulary and mel spectrogram config.
        /// @param vocab Phoneme-to-index mapping.
        /// @param mel_spec_config Mel spectrogram parameters (hop_length, sample_rate).
        AlignmentDecoder(const std::map<std::string, int> &vocab, const std::map<std::string, float> &mel_spec_config);

        /// @brief Decodes frame/edge logits into time-aligned words and phones.
        /// @param ph_frame_logits Per-frame phoneme logits.
        /// @param ph_edge_logits Per-frame edge logits.
        /// @param wav_length Duration of the audio in seconds.
        /// @param ph_seq Phoneme sequence as strings.
        /// @param[out] words Resulting word list with aligned phone boundaries.
        /// @param[out] msg Diagnostic message on failure.
        /// @param word_seq Optional word sequence for word-level grouping.
        /// @param ph_idx_to_word_idx Optional phoneme-to-word index mapping.
        /// @param ignore_sp Whether to ignore SP phonemes during decoding.
        /// @return True on success.
        bool decode(const std::vector<std::vector<std::vector<float>>> &ph_frame_logits,
                    const std::vector<std::vector<float>> &ph_edge_logits, float wav_length,
                    const std::vector<std::string> &ph_seq, WordList &words, std::string &msg,
                    const std::vector<std::string> &word_seq = {}, const std::vector<int> &ph_idx_to_word_idx = {},
                    bool ignore_sp = true);

        /// @brief Returns per-frame phoneme probability predictions.
        const std::vector<std::vector<float>> &get_ph_frame_pred() const {
            return ph_frame_pred_;
        }
        /// @brief Returns edge probabilities per frame.
        const std::vector<float> &get_edge_prob() const {
            return edge_prob_;
        }
        /// @brief Returns per-frame confidence scores.
        const std::vector<float> &get_frame_confidence() const {
            return frame_confidence_;
        }
        /// @brief Returns phoneme sequence as vocabulary indices.
        const std::vector<int> &get_ph_seq_id() const {
            return ph_seq_id_;
        }
        /// @brief Returns decoded phoneme index sequence per frame.
        const std::vector<int> &get_ph_idx_seq() const {
            return ph_idx_seq_;
        }
        /// @brief Returns predicted phoneme boundary frame indices.
        const std::vector<int> &get_ph_time_int_pred() const {
            return ph_time_int_pred_;
        }
        /// @brief Returns overall alignment confidence.
        float get_total_confidence() const {
            return total_confidence_;
        }
    };

} // namespace HFA
