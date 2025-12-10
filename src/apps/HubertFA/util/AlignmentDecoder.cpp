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
        // 验证音素序列
        std::vector<int> ph_seq_id;
        for (const auto &ph : ph_seq) {
            if (vocab_.find(ph) == vocab_.end()) {
                msg = "Phoneme '" + ph + "' not found in vocabulary.";
                return false;
            }
            ph_seq_id.push_back(vocab_.at(ph));
        }
        this->ph_seq_id_ = ph_seq_id;

        // 创建音素掩码
        std::vector<float> ph_mask(vocab_.size(), 1e9f);
        for (int id : ph_seq_id) {
            ph_mask[id] = 0.0f;
        }
        ph_mask[0] = 0.0f; // SP音素

        // 计算帧数
        int num_frames =
            static_cast<int>((wav_length * mel_spec_config_.sample_rate + 0.5f) / mel_spec_config_.hop_length);

        // 调整logits大小 - 现在ph_frame_logits[0]的形状是[vocab_size][T]
        std::vector<std::vector<float>> ph_frame_logits_vocab = ph_frame_logits[0];

        // 确保维度正确：vocab_size × T
        size_t vocab_size = vocab_.size();
        if (ph_frame_logits_vocab.size() < vocab_size) {
            ph_frame_logits_vocab.resize(vocab_size);
        }

        // 裁剪时间维度到num_frames
        for (size_t v = 0; v < vocab_size; ++v) {
            if (ph_frame_logits_vocab[v].size() > static_cast<size_t>(num_frames)) {
                ph_frame_logits_vocab[v].resize(num_frames);
            }
        }

        // 应用音素掩码 - 正确的维度顺序[vocab_size][T]
        for (size_t v = 0; v < vocab_size; ++v) {
            for (int t = 0; t < num_frames; ++t) {
                // 确保有足够的空间
                if (t >= static_cast<int>(ph_frame_logits_vocab[v].size())) {
                    ph_frame_logits_vocab[v].resize(t + 1, 0.0f);
                }
                ph_frame_logits_vocab[v][t] -= ph_mask[v];
            }
        }

        // 计算概率分布 - 形状为[vocab_size][T]
        std::vector<std::vector<float>> ph_frame_pred(vocab_size);
        std::vector<std::vector<float>> ph_prob_log(vocab_size);

        for (size_t v = 0; v < vocab_size; ++v) {
            // 对每个vocab维度的logits应用softmax
            ph_frame_pred[v] = softmax(ph_frame_logits_vocab[v]);
            ph_prob_log[v] = log_softmax(ph_frame_logits_vocab[v]);
        }
        this->ph_frame_pred_ = ph_frame_pred;

        // 计算边界概率
        std::vector<float> adjusted_edge_logits = ph_edge_logits[0];
        if (adjusted_edge_logits.size() > static_cast<size_t>(num_frames)) {
            adjusted_edge_logits.resize(num_frames);
        }

        std::vector<float> ph_edge_pred;
        for (int t = 0; t < num_frames; ++t) {
            float val = sigmoid(adjusted_edge_logits[t]);
            ph_edge_pred.push_back(std::max(0.0f, std::min(1.0f, val)));
        }

        // 计算边界差分
        std::vector<float> edge_diff(ph_edge_pred.size(), 0.0f);
        if (ph_edge_pred.size() > 1) {
            for (size_t i = 0; i < ph_edge_pred.size() - 1; ++i)
                edge_diff[i] = ph_edge_pred[i + 1] - ph_edge_pred[i];
        }

        // 计算联合边界概率
        std::vector<float> edge_prob(ph_edge_pred.size());
        edge_prob[0] = ph_edge_pred[0];
        for (size_t i = 1; i < ph_edge_pred.size(); ++i) {
            float sum = ph_edge_pred[i] + ph_edge_pred[i - 1];
            edge_prob[i] = std::max(0.0f, std::min(1.0f, sum));
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

        // 后处理：计算音素时间边界
        std::vector<float> ph_time_fractional;
        for (size_t i = 0; i < ph_time_int_pred.size(); ++i) {
            if (ph_time_int_pred[i] < static_cast<int>(edge_diff.size())) {
                float diff_val = edge_diff[ph_time_int_pred[i]];
                ph_time_fractional.push_back(std::max(-0.5f, std::min(0.5f, diff_val / 2.0f)));
            } else {
                ph_time_fractional.push_back(0.0f);
            }
        }

        std::vector<float> ph_time_pred;
        for (size_t i = 0; i < ph_time_int_pred.size(); ++i) {
            ph_time_pred.push_back(frame_length_ * (ph_time_int_pred[i] + ph_time_fractional[i]));
        }
        ph_time_pred.push_back(frame_length_ * num_frames);

        std::vector<std::pair<float, float>> ph_intervals;
        for (size_t i = 0; i < ph_time_pred.size() - 1; ++i) {
            ph_intervals.emplace_back(ph_time_pred[i], ph_time_pred[i + 1]);
        }

        // 构建WordList
        std::vector<std::string> ph_seq_decoded;
        int word_idx_last = -1;
        Word *current_word = nullptr;

        for (size_t i = 0; i < ph_idx_seq.size(); ++i) {
            int ph_idx = ph_idx_seq[i];
            if (ph_idx < 0 || ph_idx >= static_cast<int>(ph_seq.size())) {
                continue;
            }

            std::string ph_text = ph_seq[ph_idx];
            ph_seq_decoded.push_back(ph_text);

            if (ph_text == "SP" && ignore_sp)
                continue;

            Phoneme phoneme(ph_intervals[i].first, ph_intervals[i].second, ph_text);
            int word_idx =
                ph_idx_to_word_idx.empty()
                    ? ph_idx
                    : (ph_idx < static_cast<int>(ph_idx_to_word_idx.size()) ? ph_idx_to_word_idx[ph_idx] : ph_idx);
            std::string word_text = word_seq.empty()
                                        ? ph_text
                                        : (word_idx < static_cast<int>(word_seq.size()) ? word_seq[word_idx] : ph_text);

            if (word_idx == word_idx_last && current_word != nullptr) {
                current_word->append_phoneme(phoneme);
            } else {
                current_word = new Word(ph_intervals[i].first, ph_intervals[i].second, word_text);
                current_word->add_phoneme(phoneme);
                words.append(current_word);
                word_idx_last = word_idx;
            }
        }
        return true;
    }

    void AlignmentDecoder::_decode(const std::vector<int> &ph_seq_id,
                                   const std::vector<std::vector<float>> &ph_prob_log, // [vocab_size][T]
                                   const std::vector<float> &edge_prob, std::vector<int> &ph_idx_seq,
                                   std::vector<int> &ph_time_int, std::vector<float> &frame_confidence, int T) {
        const int S = static_cast<int>(ph_seq_id.size());

        // 检查维度
        if (ph_prob_log.empty() || ph_prob_log[0].size() < static_cast<size_t>(T)) {
            return;
        }

        // 提取目标音素的log概率 - ph_prob_log是[vocab_size][T]
        std::vector<std::vector<float>> prob_log(S, std::vector<float>(T));
        for (int s = 0; s < S; ++s) {
            const int vocab_idx = ph_seq_id[s];
            if (vocab_idx >= 0 && vocab_idx < static_cast<int>(ph_prob_log.size())) {
                for (int t = 0; t < T; ++t) {
                    prob_log[s][t] = ph_prob_log[vocab_idx][t];
                }
            }
        }

        // 初始化DP表 - [S][T]
        std::vector<std::vector<float>> dp(S, std::vector<float>(T, -std::numeric_limits<float>::infinity()));
        std::vector<float> curr_ph_max_prob_log(S, -std::numeric_limits<float>::infinity());
        std::vector<std::vector<int>> backtrack_s(S, std::vector<int>(T, -1));

        // 初始状态
        dp[0][0] = prob_log[0][0];
        curr_ph_max_prob_log[0] = prob_log[0][0];
        if (ph_seq_id[0] == 0 && S > 1) {
            dp[1][0] = prob_log[1][0];
            curr_ph_max_prob_log[1] = prob_log[1][0];
        }

        const int prob3_pad_len = S >= 2 ? 2 : 1;
        forward_pass(T, S, prob_log, edge_prob, curr_ph_max_prob_log, dp, ph_seq_id, prob3_pad_len, backtrack_s);

        // 确定结束状态
        int final_s = S - 1;
        if (S >= 2 && dp[S - 2][T - 1] > dp[S - 1][T - 1] && ph_seq_id[S - 1] == 0) {
            final_s = S - 2;
        }

        // 回溯路径
        std::vector<int> tmp_ph_idx_seq;
        std::vector<int> tmp_ph_time_int;
        std::vector<float> tmp_frame_confidence(T);

        int s = final_s;
        for (int t = T - 1; t >= 0; --t) {
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

        // 计算frame_confidence
        frame_confidence.resize(T);
        frame_confidence[0] = std::exp(tmp_frame_confidence[0]);
        for (int t = 1; t < T; ++t) {
            if (tmp_frame_confidence[t - 1] != 0.0f) {
                frame_confidence[t] = std::exp(tmp_frame_confidence[t] - tmp_frame_confidence[t - 1]);
            } else {
                frame_confidence[t] = std::exp(tmp_frame_confidence[t]);
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

        // 创建重置掩码
        std::vector<bool> mask_reset(S);
        for (int s = 0; s < S; ++s) {
            mask_reset[s] = ph_seq_id[s] == 0;
        }

        // 转移概率数组
        std::vector<float> prob1(S);
        std::vector<float> prob2(S, -std::numeric_limits<float>::infinity());
        std::vector<float> prob3(S, -std::numeric_limits<float>::infinity());

        for (int t = 1; t < T; ++t) {
            // 预计算prob3候选值
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
                    const int src_idx = s - prob3_pad_len;
                    if (src_idx >= S - 1 || ph_seq_id[src_idx + 1] == 0) {
                        prob3[s] = candidate_vals[src_idx];
                    }
                }

                // 选择最佳转移
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

                // 更新当前音素最大概率
                if (best_type == 0) {
                    if (prob_log[s][t] > curr_ph_max_prob_log[s]) {
                        curr_ph_max_prob_log[s] = prob_log[s][t];
                    }
                } else {
                    curr_ph_max_prob_log[s] = prob_log[s][t];
                }

                // 重置SP音素的统计
                if (mask_reset[s]) {
                    curr_ph_max_prob_log[s] = 0.0f;
                }
            }

            // 重置临时数组
            std::fill(prob2.begin(), prob2.end(), -std::numeric_limits<float>::infinity());
            std::fill(prob3.begin(), prob3.end(), -std::numeric_limits<float>::infinity());
        }
    }

} // namespace HFA