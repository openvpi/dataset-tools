#include "AlignmentDecoder.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace HFA {

    float AlignmentDecoder::sigmoid(const float x) {
        return 1.0f / (1.0f + std::exp(-x));
    }

    std::vector<float> AlignmentDecoder::softmax(const std::vector<float> &x) {
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

    std::vector<float> AlignmentDecoder::log_softmax(const std::vector<float> &x) {
        const float max_val = *std::max_element(x.begin(), x.end());
        float log_sum = 0.0f;
        for (const float val : x)
            log_sum += std::exp(val - max_val);

        log_sum = max_val + std::log(log_sum);

        std::vector<float> result(x.size());
        for (size_t i = 0; i < x.size(); ++i)
            result[i] = x[i] - log_sum;
        return result;
    }

    AlignmentDecoder::AlignmentDecoder(const std::map<std::string, int> &vocab,
                                       const std::vector<std::string> &class_names,
                                       const std::map<std::string, float> &mel_spec_config)
        : vocab_(vocab), non_speech_phs_(class_names) {
        melspec_config_.hop_length = mel_spec_config.at("hop_length");
        melspec_config_.sample_rate = mel_spec_config.at("sample_rate");
        frame_length_ = static_cast<float>(melspec_config_.hop_length) / melspec_config_.sample_rate;
    }

    bool AlignmentDecoder::decode(const std::vector<std::vector<std::vector<float>>> &ph_frame_logits,
                                  const std::vector<std::vector<float>> &ph_edge_logits,
                                  const std::vector<std::vector<std::vector<float>>> &cvnt_logits, float wav_length,
                                  const std::vector<std::string> &ph_seq, WordList &words,
                                  std::vector<float> confidence, std::string &msg,
                                  const std::vector<std::string> &word_seq, const std::vector<int> &ph_idx_to_word_idx,
                                  bool ignore_sp, const std::vector<std::string> &non_speech_phonemes) {

        std::vector<int> ph_seq_id;
        for (const auto &ph : ph_seq) {
            if (vocab_.find(ph) == vocab_.end()) {
                msg = "Phoneme '" + ph + "' not found in vocabulary.";
                return false;
            }
            ph_seq_id.push_back(vocab_.at(ph));
        }
        this->ph_seq_id_ = ph_seq_id;

        std::vector<float> ph_mask(vocab_.size(), 1e9f);
        for (int id : ph_seq_id) {
            ph_mask[id] = 0.0f;
        }
        ph_mask[0] = 0.0f;

        int num_frames = ph_frame_logits[0].size();
        if (wav_length > 0) {
            num_frames =
                static_cast<int>((wav_length * melspec_config_.sample_rate + 0.5f) / melspec_config_.hop_length);
        }

        std::vector<std::vector<float>> adjusted_frame_logits = ph_frame_logits[0];
        if (adjusted_frame_logits.size() > num_frames)
            adjusted_frame_logits.resize(num_frames);

        for (auto &vec : adjusted_frame_logits) {
            if (vec.size() < vocab_.size())
                vec.resize(vocab_.size(), 0.0f);
        }

        std::vector<float> adjusted_edge_logits = ph_edge_logits[0];
        if (adjusted_edge_logits.size() > num_frames)
            adjusted_edge_logits.resize(num_frames);


        std::vector<std::vector<std::vector<float>>> adjusted_cvnt_logits = cvnt_logits;
        if (!adjusted_cvnt_logits.empty() && !adjusted_cvnt_logits[0].empty() &&
            adjusted_cvnt_logits[0][0].size() > num_frames) {
            for (auto &batch : adjusted_cvnt_logits) {
                for (auto &cls : batch) {
                    cls.resize(num_frames);
                }
            }
        }

        for (int t = 0; t < num_frames; ++t) {
            for (size_t i = 0; i < vocab_.size(); ++i)
                adjusted_frame_logits[t][i] -= ph_mask[i];
        }

        std::vector<std::vector<float>> ph_frame_pred;
        for (int t = 0; t < num_frames; ++t)
            ph_frame_pred.push_back(softmax(adjusted_frame_logits[t]));
        this->ph_frame_pred_ = ph_frame_pred;

        std::vector<std::vector<float>> ph_prob_log;
        for (int t = 0; t < num_frames; ++t)
            ph_prob_log.push_back(log_softmax(adjusted_frame_logits[t]));

        std::vector<float> ph_edge_pred;
        for (int t = 0; t < num_frames; ++t) {
            float val = sigmoid(adjusted_edge_logits[t]);
            ph_edge_pred.push_back(std::max(0.0f, std::min(1.0f, val)));
        }

        std::vector<float> edge_diff(ph_edge_pred.size(), 0.0f);
        if (ph_edge_pred.size() > 1) {
            for (size_t i = 0; i < ph_edge_pred.size() - 1; ++i)
                edge_diff[i] = ph_edge_pred[i + 1] - ph_edge_pred[i];
        }

        std::vector<float> edge_prob(ph_edge_pred.size());
        edge_prob[0] = ph_edge_pred[0];
        for (size_t i = 1; i < ph_edge_pred.size(); ++i) {
            float sum = ph_edge_pred[i] + ph_edge_pred[i - 1];
            edge_prob[i] = std::max(0.0f, std::min(1.0f, sum));
        }
        this->edge_prob_ = edge_prob;

        std::vector<int> ph_idx_seq;
        std::vector<int> ph_time_int_pred;
        std::vector<float> frame_confidence;
        _decode(ph_seq_id, ph_prob_log, edge_prob, ph_idx_seq, ph_time_int_pred, frame_confidence);

        float log_sum = 0.0f;
        for (float conf : frame_confidence)
            log_sum += std::log(conf + 1e-6f);
        // TODO: confidence

        this->ph_idx_seq_ = ph_idx_seq;
        this->ph_time_int_pred_ = ph_time_int_pred;
        this->frame_confidence_ = frame_confidence;

        // 后处理
        std::vector<float> ph_time_fractional;
        for (size_t i = 0; i < ph_time_int_pred.size(); ++i) {
            float diff_val = edge_diff[ph_time_int_pred[i]];
            ph_time_fractional.push_back(std::max(-0.5f, std::min(0.5f, diff_val / 2.0f)));
        }

        std::vector<float> ph_time_pred;
        for (size_t i = 0; i < ph_time_int_pred.size(); ++i)
            ph_time_pred.push_back(frame_length_ * (ph_time_int_pred[i] + ph_time_fractional[i]));
        ph_time_pred.push_back(frame_length_ * num_frames);

        std::vector<std::pair<float, float>> ph_intervals;
        for (size_t i = 0; i < ph_time_pred.size() - 1; ++i)
            ph_intervals.emplace_back(ph_time_pred[i], ph_time_pred[i + 1]);

        // 构建WordList
        std::vector<std::string> ph_seq_decoded;
        int word_idx_last = -1;
        Word *current_word = nullptr;

        for (size_t i = 0; i < ph_idx_seq.size(); ++i) {
            int ph_idx = ph_idx_seq[i];
            std::string ph_text = ph_seq[ph_idx];
            ph_seq_decoded.push_back(ph_text);

            if (ph_text == "SP" && ignore_sp)
                continue;

            Phoneme phoneme(ph_intervals[i].first, ph_intervals[i].second, ph_text);
            int word_idx = ph_idx_to_word_idx.empty() ? ph_idx : ph_idx_to_word_idx[ph_idx];

            if (word_idx == word_idx_last && current_word != nullptr) {
                current_word->append_phoneme(phoneme);
            } else {
                current_word = new Word(ph_intervals[i].first, ph_intervals[i].second,
                                        word_seq.empty() ? ph_text : word_seq[word_idx]);
                current_word->add_phoneme(phoneme);
                words.append(current_word);
                word_idx_last = word_idx;
            }
        }

        std::vector<std::pair<int, int>> ph_intervals_int;
        for (size_t i = 0; i < ph_time_int_pred.size() - 1; ++i)
            ph_intervals_int.emplace_back(ph_time_int_pred[i], ph_time_int_pred[i + 1]);

        std::vector<bool> speech_phonemes_mask(num_frames, false);
        for (size_t i = 0; i < ph_intervals_int.size(); ++i) {
            if (ph_seq_decoded[i] != "SP") {
                for (int j = ph_intervals_int[i].first; j < ph_intervals_int[i].second; ++j) {
                    if (j < num_frames)
                        speech_phonemes_mask[j] = true;
                }
            }
        }

        if (!adjusted_cvnt_logits.empty()) {
            std::vector<std::vector<float>> cvnt_logits_batch = adjusted_cvnt_logits[0];
            int num_classes = cvnt_logits_batch.size();

            for (int c = 1; c < num_classes; ++c) {
                for (int t = 0; t < num_frames; ++t) {
                    if (speech_phonemes_mask[t])
                        cvnt_logits_batch[c][t] = 0.0f;
                }
            }

            cvnt_probs_.clear();
            cvnt_probs_.resize(num_classes, std::vector<float>(num_frames, 0.0f));

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
        } else {
            cvnt_probs_.clear();
        }

        for (const auto &ph : non_speech_phonemes) {
            auto it = std::find(non_speech_phs_.begin(), non_speech_phs_.end(), ph);
            if (it != non_speech_phs_.end()) {
                int index = std::distance(non_speech_phs_.begin(), it);
                auto tag_words = non_speech_words(cvnt_probs_[index], 0.5f, 5, 10, ph);
                for (const auto &tag_word : tag_words)
                    words.add_AP(tag_word);
            }
        }
        return true;
    }

    void AlignmentDecoder::_decode(const std::vector<int> &ph_seq_id,
                                   const std::vector<std::vector<float>> &ph_prob_log,
                                   const std::vector<float> &edge_prob, std::vector<int> &ph_idx_seq,
                                   std::vector<int> &ph_time_int, std::vector<float> &frame_confidence) {
        const int T = ph_prob_log.size();
        const int S = ph_seq_id.size();

        std::vector<std::vector<float>> prob_log(T, std::vector<float>(S));
        for (int t = 0; t < T; ++t) {
            for (int s = 0; s < S; ++s)
                prob_log[t][s] = ph_prob_log[t][ph_seq_id[s]];
        }

        std::vector<std::vector<float>> dp(T, std::vector<float>(S, -std::numeric_limits<float>::infinity()));
        std::vector<float> curr_ph_max_prob_log(S, -std::numeric_limits<float>::infinity());
        std::vector<std::vector<int>> backtrack_s(T, std::vector<int>(S, -1));

        dp[0][0] = prob_log[0][0];
        curr_ph_max_prob_log[0] = prob_log[0][0];
        if (ph_seq_id[0] == 0 && S > 1) {
            dp[0][1] = prob_log[0][1];
            curr_ph_max_prob_log[1] = prob_log[0][1];
        }

        const int prob3_pad_len = S >= 2 ? 2 : 1;
        forward_pass(T, S, prob_log, edge_prob, curr_ph_max_prob_log, dp, ph_seq_id, prob3_pad_len, backtrack_s);

        int final_s = S >= 2 && dp[T - 1][S - 2] > dp[T - 1][S - 1] && ph_seq_id[S - 1] == 0 ? S - 2 : S - 1;

        std::vector<int> tmp_ph_idx_seq;
        std::vector<int> tmp_ph_time_int;
        std::vector<float> tmp_frame_confidence;

        for (int t = T - 1; t >= 0; --t) {
            tmp_frame_confidence.push_back(dp[t][final_s]);
            if (backtrack_s[t][final_s] != 0) {
                tmp_ph_idx_seq.push_back(final_s);
                tmp_ph_time_int.push_back(t);
                final_s -= backtrack_s[t][final_s];
            }
        }

        ph_idx_seq = std::vector<int>(tmp_ph_idx_seq.rbegin(), tmp_ph_idx_seq.rend());
        ph_time_int = std::vector<int>(tmp_ph_time_int.rbegin(), tmp_ph_time_int.rend());
        frame_confidence = std::vector<float>(tmp_frame_confidence.rbegin(), tmp_frame_confidence.rend());
    }

    void AlignmentDecoder::forward_pass(const int T, const int S, const std::vector<std::vector<float>> &prob_log,
                                        const std::vector<float> &edge_prob, std::vector<float> &curr_ph_max_prob_log,
                                        std::vector<std::vector<float>> &dp, const std::vector<int> &ph_seq_id,
                                        const int prob3_pad_len, std::vector<std::vector<int>> &backtrack_s) {
        std::vector<float> edge_prob_log(edge_prob.size());
        std::vector<float> not_edge_prob_log(edge_prob.size());
        for (size_t i = 0; i < edge_prob.size(); ++i) {
            edge_prob_log[i] = std::log(edge_prob[i] + 1e-6f);
            not_edge_prob_log[i] = std::log(1.0f - edge_prob[i] + 1e-6f);
        }

        std::vector<bool> mask_reset(S);
        for (int i = 0; i < S; ++i)
            mask_reset[i] = ph_seq_id[i] == 0;

        std::vector<float> prob1(S);
        std::vector<float> prob2(S, -std::numeric_limits<float>::infinity());
        std::vector<float> prob3(S, -std::numeric_limits<float>::infinity());

        for (int t = 1; t < T; ++t) {
            const auto &prob_log_t = prob_log[t];
            const float edge_log_t = edge_prob_log[t];
            const float not_edge_log_t = not_edge_prob_log[t];
            const auto &dp_prev = dp[t - 1];

            std::fill(prob2.begin(), prob2.end(), -std::numeric_limits<float>::infinity());
            for (int s = prob3_pad_len; s < S; ++s)
                prob3[s] = -std::numeric_limits<float>::infinity();

            // 类型1转移: 停留在当前音素
            for (int s = 0; s < S; ++s) {
                prob1[s] = dp_prev[s] + prob_log_t[s] + not_edge_log_t;
            }

            // 类型2转移: 移动到下一个音素
            for (int s = 1; s < S; ++s) {
                prob2[s] = dp_prev[s - 1] + prob_log_t[s - 1] + edge_log_t +
                           curr_ph_max_prob_log[s - 1] * (static_cast<float>(T) / S);
            }

            // 类型3转移: 跳转到后续音素
            for (int s = prob3_pad_len; s < S; ++s) {
                const int src_idx = s - prob3_pad_len;
                const int cond_idx = src_idx + 1;

                if (cond_idx >= S - 1 || ph_seq_id[cond_idx] == 0) {
                    prob3[s] = dp_prev[src_idx] + prob_log_t[src_idx] + edge_log_t +
                               curr_ph_max_prob_log[src_idx] * (static_cast<float>(T) / S);
                }
            }

            for (int s = 0; s < S; ++s) {
                float max_val = prob1[s];
                int best_type = 0;

                if (prob2[s] > max_val) {
                    max_val = prob2[s];
                    best_type = 1;
                }
                if (prob3[s] > max_val) {
                    max_val = prob3[s];
                    best_type = 2;
                }

                dp[t][s] = max_val;
                backtrack_s[t][s] = best_type;

                if (best_type == 0) {
                    if (prob_log_t[s] > curr_ph_max_prob_log[s]) {
                        curr_ph_max_prob_log[s] = prob_log_t[s];
                    }
                } else {
                    curr_ph_max_prob_log[s] = prob_log_t[s];
                }

                if (mask_reset[s]) {
                    curr_ph_max_prob_log[s] = 0.0f;
                }
            }
        }
    }

    WordList AlignmentDecoder::non_speech_words(const std::vector<float> &prob, const float threshold,
                                                const int max_gap, const int ap_threshold,
                                                const std::string &tag) const {
        WordList words;
        int start = -1;
        int gap_count = 0;

        for (int i = 0; i < prob.size(); ++i) {
            if (prob[i] >= threshold) {
                if (start == -1) {
                    start = i;
                }
                gap_count = 0;
            } else {
                if (start != -1) {
                    if (gap_count < max_gap) {
                        gap_count++;
                    } else {
                        const int end = i - gap_count - 1;
                        if (end > start && end - start >= ap_threshold) {
                            const float start_time = start * frame_length_;
                            const float end_time = end * frame_length_;
                            auto word = new Word(start_time, end_time, tag, true);
                            words.push_back(word);
                        }
                        start = -1;
                        gap_count = 0;
                    }
                }
            }
        }

        if (start != -1 && prob.size() - start >= ap_threshold) {
            const float start_time = start * frame_length_;
            const float end_time = (prob.size() - 1) * frame_length_;
            const auto word = new Word(start_time, end_time, tag, true);
            words.push_back(word);
        }

        return words;
    }

} // namespace HFA