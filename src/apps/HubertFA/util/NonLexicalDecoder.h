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

    class NonLexicalDecoder {
    private:
        struct MelSpecConfig {
            int hop_length;
            int sample_rate;
        };

        std::vector<std::string> non_lexical_phs_;
        MelSpecConfig melspec_config_;
        float frame_length_;
        std::vector<std::vector<float>> cvnt_probs_;

        static std::vector<float> softmax(const std::vector<float> &x);

        WordList non_lexical_words(const std::vector<float> &prob, float threshold = 0.5f, int max_gap = 5,
                                   int mix_frames = 10, const std::string &tag = "") const;

    public:
        NonLexicalDecoder(const std::vector<std::string> &class_names,
                          const std::map<std::string, float> &mel_spec_config);

        std::vector<WordList> decode(const std::vector<std::vector<std::vector<float>>> &cvnt_logits, float wav_length,
                                     const std::vector<std::string> &non_lexical_phonemes = {});

        const std::vector<std::vector<float>> &get_cvnt_probs() const {
            return cvnt_probs_;
        }
        const std::vector<std::string> &get_non_lexical_phs() const {
            return non_lexical_phs_;
        }
        float get_frame_length() const {
            return frame_length_;
        }
    };

} // namespace HFA