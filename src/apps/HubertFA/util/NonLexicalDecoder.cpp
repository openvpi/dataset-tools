#include "NonLexicalDecoder.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace HFA {

    std::vector<float> NonLexicalDecoder::softmax(const std::vector<float> &x) {
        const float max_val = *std::max_element(x.begin(), x.end());
        std::vector<float> exp_vals(x.size());
        float sum = 0.0f;

        for (size_t i = 0; i < x.size(); ++i) {
            exp_vals[i] = std::exp(x[i] - max_val);
            sum += exp_vals[i];
        }

        for (auto &val : exp_vals) {
            val /= sum;
        }
        return exp_vals;
    }

    NonLexicalDecoder::NonLexicalDecoder(const std::vector<std::string> &class_names,
                                         const std::map<std::string, float> &mel_spec_config)
        : non_lexical_phs_(class_names) {
        melspec_config_.hop_length = static_cast<int>(mel_spec_config.at("hop_size"));
        melspec_config_.sample_rate = static_cast<int>(mel_spec_config.at("sample_rate"));
        frame_length_ = static_cast<float>(melspec_config_.hop_length) / melspec_config_.sample_rate;
    }

    std::vector<WordList> NonLexicalDecoder::decode(const std::vector<std::vector<std::vector<float>>> &cvnt_logits,
                                                    const float wav_length,
                                                    const std::vector<std::string> &non_lexical_phonemes) {
        std::vector<WordList> result;

        if (non_lexical_phonemes.empty()) {
            return result;
        }

        // 计算帧数（如果wav_length有效）
        int num_frames = cvnt_logits[0][0].size(); // 默认使用logits的长度
        if (wav_length > 0) {
            num_frames =
                static_cast<int>((wav_length * melspec_config_.sample_rate + 0.5f) / melspec_config_.hop_length);
            // 调整logits大小
            std::vector<std::vector<std::vector<float>>> adjusted_logits = cvnt_logits;
            if (!adjusted_logits.empty() && !adjusted_logits[0].empty()) {
                for (auto &batch : adjusted_logits) {
                    for (auto &cls : batch) {
                        if (cls.size() > num_frames)
                            cls.resize(num_frames);
                    }
                }
            }
        }

        // 计算softmax概率（只处理第一个batch）
        cvnt_probs_.clear();
        if (!cvnt_logits.empty() && !cvnt_logits[0].empty()) {
            const std::vector<std::vector<float>> cvnt_logits_batch = cvnt_logits[0];
            const int num_classes = static_cast<int>(cvnt_logits_batch.size());

            cvnt_probs_.resize(num_classes, std::vector<float>(num_frames, 0.0f));

            // 逐帧计算softmax
            for (int t = 0; t < num_frames; ++t) {
                std::vector<float> frame_logits(num_classes);
                for (int c = 0; c < num_classes; ++c) {
                    frame_logits[c] = cvnt_logits_batch[c][t];
                }

                std::vector<float> probs = softmax(frame_logits);
                for (int c = 0; c < num_classes; ++c) {
                    cvnt_probs_[c][t] = probs[c];
                }
            }
        }

        // 为每个目标音素生成WordList
        for (const auto &ph : non_lexical_phonemes) {
            auto it = std::find(non_lexical_phs_.begin(), non_lexical_phs_.end(), ph);
            if (it != non_lexical_phs_.end()) {
                const int index = static_cast<int>(std::distance(non_lexical_phs_.begin(), it));

                // 获取该音素的概率曲线
                if (index < cvnt_probs_.size()) {
                    WordList words;
                    auto tag_words = non_lexical_words(cvnt_probs_[index], 0.5f, 5, 10, ph);
                    result.push_back(tag_words);
                }
            }
        }

        return result;
    }

    WordList NonLexicalDecoder::non_lexical_words(const std::vector<float> &prob, const float threshold,
                                                  const int max_gap, const int mix_frames,
                                                  const std::string &tag) const {
        WordList words;
        int start = -1;
        int gap_count = 0;

        for (int i = 0; i < static_cast<int>(prob.size()); ++i) {
            if (prob[i] >= threshold) {
                if (start == -1) {
                    start = i;
                }
                gap_count = 0;
            } else if (start != -1) {
                if (gap_count < max_gap) {
                    gap_count++;
                } else {
                    const int end = i - gap_count - 1;
                    if (end > start && end - start >= mix_frames) {
                        const float start_time = start * frame_length_;
                        const float end_time = end * frame_length_;
                        auto word = new Word(start_time, end_time, tag);
                        word->add_phoneme(Phoneme(start_time, end_time, tag));
                        words.push_back(word);
                    }
                    start = -1;
                    gap_count = 0;
                }
            }
        }

        // 处理音频末尾的段
        if (start != -1 && static_cast<int>(prob.size()) - start >= mix_frames) {
            const float start_time = start * frame_length_;
            const float end_time = (prob.size() - 1) * frame_length_;
            const auto word = new Word(start_time, end_time, tag);
            word->add_phoneme(Phoneme(start_time, end_time, tag));
            words.push_back(word);
        }

        return words;
    }

} // namespace HFA