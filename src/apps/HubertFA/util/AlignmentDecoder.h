#pragma once

#include "AlignWord.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace HFA {

    class AlignmentDecoder {
    private:
        struct MelSpecConfig {
            int hop_length;
            int sample_rate;
        };

        std::map<std::string, int> vocab_;
        std::vector<std::string> non_speech_phs_;
        MelSpecConfig melspec_config_;
        float frame_length_;

        std::vector<int> ph_seq_id_;
        std::vector<int> ph_idx_seq_;
        std::vector<std::vector<float>> ph_frame_pred_;
        std::vector<int> ph_time_int_pred_;

        std::vector<Phoneme> ph_seq_pred_;
        std::vector<std::pair<float, float>> ph_intervals_pred_;

        std::vector<std::vector<float>> cvnt_probs_;
        std::vector<float> edge_prob_;
        std::vector<float> frame_confidence_;

        static float sigmoid(float x);
        static std::vector<float> softmax(const std::vector<float> &x);
        static std::vector<float> log_softmax(const std::vector<float> &x);

        static void forward_pass(int T, int S, const std::vector<std::vector<float>> &prob_log,
                                 const std::vector<float> &edge_prob, std::vector<float> &curr_ph_max_prob_log,
                                 std::vector<std::vector<float>> &dp, const std::vector<int> &ph_seq_id,
                                 int prob3_pad_len, std::vector<std::vector<int>> &backtrack_s);

        static void _decode(const std::vector<int> &ph_seq_id, const std::vector<std::vector<float>> &ph_prob_log,
                            const std::vector<float> &edge_prob, std::vector<int> &ph_idx_seq,
                            std::vector<int> &ph_time_int, std::vector<float> &frame_confidence);

        WordList non_speech_words(const std::vector<float> &prob, float threshold = 0.5f, int max_gap = 5,
                                  int ap_threshold = 10, const std::string &tag = "") const;

    public:
        AlignmentDecoder(const std::map<std::string, int> &vocab, const std::vector<std::string> &class_names,
                         const std::map<std::string, float> &mel_spec_config);

        bool decode(const std::vector<std::vector<std::vector<float>>> &ph_frame_logits,
                    const std::vector<std::vector<float>> &ph_edge_logits,
                    const std::vector<std::vector<std::vector<float>>> &cvnt_logits, float wav_length,
                    const std::vector<std::string> &ph_seq, WordList &words, std::vector<float> confidence,
                    std::string &msg, const std::vector<std::string> &word_seq = {},
                    const std::vector<int> &ph_idx_to_word_idx = {}, bool ignore_sp = true,
                    const std::vector<std::string> &non_speech_phonemes = {});
    };

} // namespace HFA