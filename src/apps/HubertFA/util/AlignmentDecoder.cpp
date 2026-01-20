#include "AlignmentDecoder.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>

namespace HFA {

    float AlignmentDecoder::sigmoid(const float x) {
        return 1.0f / (1.0f + std::exp(-x));
    }

    std::vector<std::vector<float>> AlignmentDecoder::softmax_2d(const std::vector<std::vector<float>> &x,
                                                                 const int axis) {
        if (x.empty())
            return {};

        const size_t dim1 = x.size();
        const size_t dim2 = x[0].size();

        if (axis == 0) {
            std::vector<std::vector<float>> result(dim1, std::vector<float>(dim2));

            for (size_t j = 0; j < dim2; ++j) {
                float max_val = -std::numeric_limits<float>::infinity();
                for (size_t i = 0; i < dim1; ++i) {
                    if (j < x[i].size()) {
                        max_val = std::max(max_val, x[i][j]);
                    }
                }

                float sum = 0.0f;
                for (size_t i = 0; i < dim1; ++i) {
                    if (j < x[i].size()) {
                        result[i][j] = std::exp(x[i][j] - max_val);
                        sum += result[i][j];
                    } else {
                        result[i][j] = 0.0f;
                    }
                }

                if (sum > 0.0f) {
                    for (size_t i = 0; i < dim1; ++i) {
                        result[i][j] /= sum;
                    }
                }
            }
            return result;
        }
        if (axis == 1) {
            std::vector<std::vector<float>> result(dim1, std::vector<float>(dim2));

            for (size_t i = 0; i < dim1; ++i) {
                if (x[i].empty())
                    continue;

                const float max_val = *std::max_element(x[i].begin(), x[i].end());
                float sum = 0.0f;

                for (size_t j = 0; j < dim2; ++j) {
                    result[i][j] = std::exp(x[i][j] - max_val);
                    sum += result[i][j];
                }

                if (sum > 0.0f) {
                    for (size_t j = 0; j < dim2; ++j) {
                        result[i][j] /= sum;
                    }
                }
            }
            return result;
        }

        throw std::runtime_error("Unsupported axis for 2D softmax");
    }

    std::vector<std::vector<float>> AlignmentDecoder::log_softmax_2d(const std::vector<std::vector<float>> &x,
                                                                     const int axis) {
        if (x.empty())
            return {};

        const size_t dim1 = x.size();
        const size_t dim2 = x[0].size();

        if (axis == 0) {
            std::vector<std::vector<float>> result(dim1, std::vector<float>(dim2));

            for (size_t j = 0; j < dim2; ++j) {
                float max_val = -std::numeric_limits<float>::infinity();
                for (size_t i = 0; i < dim1; ++i) {
                    if (j < x[i].size()) {
                        max_val = std::max(max_val, x[i][j]);
                    }
                }

                float log_sum_exp = 0.0f;
                for (size_t i = 0; i < dim1; ++i) {
                    if (j < x[i].size()) {
                        log_sum_exp += std::exp(x[i][j] - max_val);
                    }
                }
                log_sum_exp = max_val + std::log(log_sum_exp + 1e-12f);

                for (size_t i = 0; i < dim1; ++i) {
                    if (j < x[i].size()) {
                        result[i][j] = x[i][j] - log_sum_exp;
                    } else {
                        result[i][j] = -std::numeric_limits<float>::infinity();
                    }
                }
            }
            return result;
        }

        if (axis == 1) {
            std::vector<std::vector<float>> result(dim1, std::vector<float>(dim2));

            for (size_t i = 0; i < dim1; ++i) {
                if (x[i].empty())
                    continue;

                const float max_val = *std::max_element(x[i].begin(), x[i].end());
                float log_sum_exp = 0.0f;

                for (size_t j = 0; j < dim2; ++j) {
                    log_sum_exp += std::exp(x[i][j] - max_val);
                }
                log_sum_exp = max_val + std::log(log_sum_exp + 1e-12f);

                for (size_t j = 0; j < dim2; ++j) {
                    result[i][j] = x[i][j] - log_sum_exp;
                }
            }
            return result;
        }
        throw std::runtime_error("Unsupported axis for 2D softmax");
    }

    AlignmentDecoder::AlignmentDecoder(const std::map<std::string, int> &vocab,
                                       const std::map<std::string, float> &mel_spec_config)
        : vocab_(vocab) {
        mel_spec_config_.hop_length = static_cast<int>(mel_spec_config.at("hop_size"));
        mel_spec_config_.sample_rate = static_cast<int>(mel_spec_config.at("sample_rate"));
        frame_length_ = static_cast<float>(mel_spec_config_.hop_length) / mel_spec_config_.sample_rate;
    }

    bool AlignmentDecoder::decode(const std::vector<std::vector<std::vector<float>>> &ph_frame_logits,
                                  const std::vector<std::vector<float>> &ph_edge_logits, float wav_length,
                                  const std::vector<std::string> &ph_seq, WordList &words, std::string &msg,
                                  const std::vector<std::string> &word_seq, const std::vector<int> &ph_idx_to_word_idx,
                                  bool ignore_sp) {
        try {
            std::vector<int> ph_seq_id;
            for (const auto &ph : ph_seq) {
                if (vocab_.find(ph) == vocab_.end()) {
                    msg = "Phone '" + ph + "' not found in vocabulary.";
                    return false;
                }
                ph_seq_id.push_back(vocab_.at(ph));
            }
            this->ph_seq_id_ = ph_seq_id;

            size_t vocab_size = vocab_.size();
            std::vector<float> ph_mask(vocab_size, 1e9f);
            for (int id : ph_seq_id) {
                if (id >= 0 && static_cast<size_t>(id) < vocab_size) {
                    ph_mask[id] = 0.0f;
                }
            }
            ph_mask[0] = 0.0f; // SP音素

            // 计算帧数
            int num_frames =
                static_cast<int>((wav_length * mel_spec_config_.sample_rate + 0.5f) / mel_spec_config_.hop_length);

            // 获取 logits [vocab_size][T]
            if (ph_frame_logits.empty() || ph_frame_logits[0].empty()) {
                msg = "ph_frame_logits is empty";
                return false;
            }

            std::vector<std::vector<float>> ph_frame_logits_vocab = ph_frame_logits[0];
            // 裁剪时间维度到 num_frames
            for (size_t v = 0; v < ph_frame_logits_vocab.size(); ++v) {
                if (ph_frame_logits_vocab[v].size() > static_cast<size_t>(num_frames)) {
                    ph_frame_logits_vocab[v].resize(num_frames);
                }
            }

            // 应用音素掩码 [vocab_size][T]
            for (size_t v = 0; v < ph_frame_logits_vocab.size(); ++v) {
                for (int t = 0; t < num_frames; ++t) {
                    if (t < static_cast<int>(ph_frame_logits_vocab[v].size())) {
                        ph_frame_logits_vocab[v][t] -= ph_mask[v];
                    }
                }
            }

            this->ph_frame_pred_ = softmax_2d(ph_frame_logits_vocab, 0); // axis=0
            auto ph_prob_log = log_softmax_2d(ph_frame_logits_vocab, 0); // axis=0

            if (ph_edge_logits.empty()) {
                msg = "ph_edge_logits is empty";
                return false;
            }

            std::vector<float> adjusted_edge_logits = ph_edge_logits[0];
            if (adjusted_edge_logits.size() > static_cast<size_t>(num_frames)) {
                adjusted_edge_logits.resize(num_frames);
            }

            std::vector<float> ph_edge_pred;
            for (size_t t = 0; t < adjusted_edge_logits.size(); ++t) {
                float val = sigmoid(adjusted_edge_logits[t]);
                ph_edge_pred.push_back(std::max(0.0f, std::min(1.0f, val)));
            }

            std::vector<float> edge_diff(ph_edge_pred.size(), 0.0f);
            if (ph_edge_pred.size() > 1) {
                for (size_t i = 0; i < ph_edge_pred.size() - 1; ++i) {
                    edge_diff[i] = ph_edge_pred[i + 1] - ph_edge_pred[i];
                }
            }

            std::vector<float> edge_prob(ph_edge_pred.size());
            edge_prob[0] = ph_edge_pred[0];
            for (size_t i = 1; i < ph_edge_pred.size(); ++i) {
                edge_prob[i] = ph_edge_pred[i] + ph_edge_pred[i - 1];
                if (edge_prob[i] < 0.0f)
                    edge_prob[i] = 0.0f;
                if (edge_prob[i] > 1.0f)
                    edge_prob[i] = 1.0f;
            }
            this->edge_prob_ = edge_prob;

            // Viterbi解码
            std::vector<int> ph_idx_seq;
            std::vector<int> ph_time_int_pred;
            std::vector<float> frame_confidence;
            _decode(ph_seq_id, ph_prob_log, edge_prob, ph_idx_seq, ph_time_int_pred, frame_confidence, num_frames);

            this->ph_idx_seq_ = ph_idx_seq;
            this->ph_time_int_pred_ = ph_time_int_pred;
            this->frame_confidence_ = frame_confidence;

            std::vector<float> ph_time_fractional;
            for (size_t i = 0; i < ph_time_int_pred.size(); ++i) {
                if (ph_time_int_pred[i] >= 0 && ph_time_int_pred[i] < static_cast<int>(edge_diff.size())) {
                    float diff_val = edge_diff[ph_time_int_pred[i]] / 2.0f;
                    // clip(-0.5, 0.5)
                    if (diff_val < -0.5f)
                        diff_val = -0.5f;
                    if (diff_val > 0.5f)
                        diff_val = 0.5f;
                    ph_time_fractional.push_back(diff_val);
                } else {
                    ph_time_fractional.push_back(0.0f);
                }
            }

            std::vector<float> ph_time_pred;
            for (size_t i = 0; i < ph_time_int_pred.size(); ++i) {
                float time = frame_length_ * (static_cast<float>(ph_time_int_pred[i]) + ph_time_fractional[i]);
                ph_time_pred.push_back(std::max(0.0f, time));
            }
            ph_time_pred.push_back(frame_length_ * num_frames);

            std::vector<std::pair<float, float>> ph_intervals;
            for (size_t i = 0; i < ph_time_pred.size() - 1; ++i) {
                ph_intervals.emplace_back(ph_time_pred[i], ph_time_pred[i + 1]);
            }

            std::vector<std::string> ph_seq_decoded;
            int word_idx_last = -1;
            Word current_word(0, 1, "temp");
            bool has_current_word = false;

            for (size_t i = 0; i < ph_idx_seq.size(); ++i) {
                int ph_idx = ph_idx_seq[i];
                if (ph_idx < 0 || ph_idx >= static_cast<int>(ph_seq.size())) {
                    continue;
                }

                std::string ph_text = ph_seq[ph_idx];
                ph_seq_decoded.push_back(ph_text);

                if (ph_text == "SP" && ignore_sp) {
                    continue;
                }

                float start = ph_intervals[i].first;
                float end = ph_intervals[i].second;

                Phone phoneme(start, end, ph_text);

                int word_idx;
                if (ph_idx_to_word_idx.empty()) {
                    word_idx = ph_idx;
                } else if (ph_idx < static_cast<int>(ph_idx_to_word_idx.size())) {
                    word_idx = ph_idx_to_word_idx[ph_idx];
                } else {
                    word_idx = ph_idx;
                }

                std::string word_text;
                if (word_seq.empty()) {
                    word_text = ph_text;
                } else if (word_idx < static_cast<int>(word_seq.size())) {
                    word_text = word_seq[word_idx];
                } else {
                    word_text = ph_text;
                }

                if (word_idx == word_idx_last && has_current_word) {
                    current_word.append_phone(phoneme);
                    current_word.end = phoneme.end;
                } else {
                    if (has_current_word)
                        words.append(current_word);
                    current_word = Word(start, end, word_text);
                    current_word.add_phone(phoneme);
                    has_current_word = true;
                    word_idx_last = word_idx;
                }
            }

            if (has_current_word) {
                words.append(current_word);
            }

            if (!frame_confidence.empty()) {
                float log_sum = 0.0f;
                int count = 0;
                for (float conf : frame_confidence) {
                    log_sum += std::log(conf + 1e-6f);
                    count++;
                }
                total_confidence_ = std::exp(log_sum / count / 3.0f);
            } else {
                total_confidence_ = 0.0f;
            }

            return true;
        } catch (const std::exception &e) {
            msg = std::string("Error in decode: ") + e.what();
            return false;
        }
    }

    void AlignmentDecoder::_decode(const std::vector<int> &ph_seq_id,
                                   const std::vector<std::vector<float>> &ph_prob_log, // [vocab_size][T]
                                   const std::vector<float> &edge_prob, std::vector<int> &ph_idx_seq,
                                   std::vector<int> &ph_time_int, std::vector<float> &frame_confidence, const int T) {
        const int S = static_cast<int>(ph_seq_id.size());
        if (S == 0 || T == 0)
            return;

        if (ph_prob_log.empty() || ph_prob_log[0].size() < static_cast<size_t>(T)) {
            return;
        }

        std::vector<std::vector<float>> prob_log(S, std::vector<float>(T, -std::numeric_limits<float>::infinity()));
        for (int s = 0; s < S; ++s) {
            const int vocab_idx = ph_seq_id[s];
            if (vocab_idx >= 0 && static_cast<size_t>(vocab_idx) < ph_prob_log.size()) {
                for (int t = 0; t < T; ++t) {
                    if (t < static_cast<int>(ph_prob_log[vocab_idx].size())) {
                        prob_log[s][t] = ph_prob_log[vocab_idx][t];
                    }
                }
            }
        }

        // 初始化DP表 - [S][T]
        std::vector<std::vector<float>> dp(S, std::vector<float>(T, -std::numeric_limits<float>::infinity()));
        std::vector<float> curr_ph_max_prob_log(S, -std::numeric_limits<float>::infinity());
        std::vector<std::vector<int>> backtrack_s(S, std::vector<int>(T, -1));

        // 初始状态
        if (prob_log[0].size() > 0) {
            dp[0][0] = prob_log[0][0];
            curr_ph_max_prob_log[0] = prob_log[0][0];
        }

        if (S > 1 && ph_seq_id[0] == 0 && prob_log[1].size() > 0) {
            dp[1][0] = prob_log[1][0];
            curr_ph_max_prob_log[1] = prob_log[1][0];
        }

        const int prob3_pad_len = S >= 2 ? 2 : 1;
        forward_pass(T, S, prob_log, edge_prob, curr_ph_max_prob_log, dp, ph_seq_id, prob3_pad_len, backtrack_s);

        // 确定结束状态 (与Python一致)
        int final_s = S - 1;
        if (S >= 2 && dp[S - 2][T - 1] > dp[S - 1][T - 1] && ph_seq_id[S - 1] == 0) {
            final_s = S - 2;
        }

        // 回溯路径
        std::vector<int> tmp_ph_idx_seq;
        std::vector<int> tmp_ph_time_int;
        std::vector<float> tmp_frame_confidence(T, 0.0f);

        int s = final_s;
        for (int t = T - 1; t >= 0; --t) {
            if (s < 0 || s >= S)
                break;

            tmp_frame_confidence[t] = dp[s][t];

            if (backtrack_s[s][t] != 0) {
                tmp_ph_idx_seq.push_back(s);
                tmp_ph_time_int.push_back(t);

                // 根据转移类型更新s
                if (backtrack_s[s][t] == 1) {
                    s -= 1; // 类型2转移
                } else if (backtrack_s[s][t] == 2) {
                    s -= 2; // 类型3转移
                }
            }
        }

        // 反转结果
        std::reverse(tmp_ph_idx_seq.begin(), tmp_ph_idx_seq.end());
        std::reverse(tmp_ph_time_int.begin(), tmp_ph_time_int.end());

        ph_idx_seq = tmp_ph_idx_seq;
        ph_time_int = tmp_ph_time_int;

        // 计算 frame_confidence (与Python一致: exp(diff(pad(confidence, (1, 0)))) )
        frame_confidence.resize(T);
        if (T > 0) {
            // 添加 padding
            std::vector<float> padded_confidence(T + 1, 0.0f);
            for (int t = 0; t < T; ++t) {
                padded_confidence[t + 1] = tmp_frame_confidence[t];
            }

            // 计算差分
            for (int t = 0; t < T; ++t) {
                frame_confidence[t] = std::exp(padded_confidence[t + 1] - padded_confidence[t]);
            }
        }
    }

    void AlignmentDecoder::forward_pass(const int T, const int S, const std::vector<std::vector<float>> &prob_log,
                                        const std::vector<float> &edge_prob, std::vector<float> &curr_ph_max_prob_log,
                                        std::vector<std::vector<float>> &dp, const std::vector<int> &ph_seq_id,
                                        const int prob3_pad_len, std::vector<std::vector<int>> &backtrack_s) {
        // 计算log概率
        std::vector<float> edge_prob_log(T);
        std::vector<float> not_edge_prob_log(T);
        for (int t = 0; t < T; ++t) {
            edge_prob_log[t] = std::log(edge_prob[t] + 1e-6f);
            not_edge_prob_log[t] = std::log(1.0f - edge_prob[t] + 1e-6f);
        }

        std::vector<bool> mask_reset(S);
        for (int s = 0; s < S; ++s) {
            mask_reset[s] = ph_seq_id[s] == 0;
        }

        std::vector<float> prob1(S);
        std::vector<float> prob2(S, -std::numeric_limits<float>::infinity());
        std::vector<float> prob3(S, -std::numeric_limits<float>::infinity());

        for (int t = 1; t < T; ++t) {
            std::vector<float> candidate_vals(S - prob3_pad_len, -std::numeric_limits<float>::infinity());
            for (int src_idx = 0; src_idx < S - prob3_pad_len; ++src_idx) {
                candidate_vals[src_idx] = dp[src_idx][t - 1] + prob_log[src_idx][t] + edge_prob_log[t] +
                                          curr_ph_max_prob_log[src_idx] * (static_cast<float>(T) / S);
            }

            for (int s = 0; s < S; ++s) {
                // 类型1转移: 停留在当前音素
                prob1[s] = dp[s][t - 1] + prob_log[s][t] + not_edge_prob_log[t];

                // 类型2转移: 移动到下一个音素
                if (s > 0) {
                    prob2[s] = dp[s - 1][t - 1] + prob_log[s - 1][t] + edge_prob_log[t] +
                               curr_ph_max_prob_log[s - 1] * (static_cast<float>(T) / S);
                }

                // 类型3转移: 跳转到后续音素
                if (s >= prob3_pad_len) {
                    // 计算 idx_arr = s - prob3_pad_len + 1，并clip到[0, S-1]
                    int idx_arr = s - prob3_pad_len + 1;
                    if (idx_arr < 0)
                        idx_arr = 0;
                    if (idx_arr > S - 1)
                        idx_arr = S - 1;

                    if (idx_arr >= S - 1 || ph_seq_id[idx_arr] == 0) {
                        prob3[s] = candidate_vals[s - prob3_pad_len];
                    }
                }

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

                dp[s][t] = max_val;
                backtrack_s[s][t] = best_type;

                if (best_type == 0) {
                    if (prob_log[s][t] > curr_ph_max_prob_log[s]) {
                        curr_ph_max_prob_log[s] = prob_log[s][t];
                    }
                } else {
                    curr_ph_max_prob_log[s] = prob_log[s][t];
                }

                // curr_ph_max_prob_log[mask_reset] = 0.0
                if (mask_reset[s]) {
                    curr_ph_max_prob_log[s] = 0.0f;
                }
            }

            std::fill(prob2.begin(), prob2.end(), -std::numeric_limits<float>::infinity());
            std::fill(prob3.begin(), prob3.end(), -std::numeric_limits<float>::infinity());
        }
    }

} // namespace HFA