#include "SequenceAligner.h"

#include <QHash>
#include <algorithm>
#include <limits>

namespace LyricFA {

    SequenceAligner::SequenceAligner(const int deletion_cost, const int insertion_cost, const int substitution_cost)
        : m_deletion_cost(deletion_cost), m_insertion_cost(insertion_cost), m_substitution_cost(substitution_cost) {
    }

    std::tuple<int, QVector<QString>, QVector<QString>>
        SequenceAligner::compute_alignment(const QVector<QString> &seq1, const QVector<QString> &seq2) const {
        const int len1 = seq1.size();
        const int len2 = seq2.size();

        QVector<QVector<int>> dp(len1 + 1, QVector<int>(len2 + 1, 0));
        QVector<QVector<int>> bt(len1 + 1, QVector<int>(len2 + 1, 0));

        for (int i = 1; i <= len1; ++i) {
            dp[i][0] = i * m_deletion_cost;
            bt[i][0] = 2; // DELETE
        }
        for (int j = 1; j <= len2; ++j) {
            dp[0][j] = j * m_insertion_cost;
            bt[0][j] = 3; // INSERT
        }

        for (int i = 1; i <= len1; ++i) {
            const QString &s1 = seq1[i - 1];
            for (int j = 1; j <= len2; ++j) {
                const QString &s2 = seq2[j - 1];
                if (s1 == s2) {
                    dp[i][j] = dp[i - 1][j - 1];
                    bt[i][j] = 0; // MATCH
                } else {
                    const int sub = dp[i - 1][j - 1] + m_substitution_cost;
                    const int del = dp[i - 1][j] + m_deletion_cost;
                    const int ins = dp[i][j - 1] + m_insertion_cost;

                    int min_cost = sub;
                    int op = 1; // SUB
                    if (del < min_cost) {
                        min_cost = del;
                        op = 2; // DEL
                    }
                    if (ins < min_cost) {
                        min_cost = ins;
                        op = 3; // INS
                    }
                    dp[i][j] = min_cost;
                    bt[i][j] = op;
                }
            }
        }

        QVector<QString> aligned1, aligned2;
        int i = len1, j = len2;
        while (i > 0 || j > 0) {
            if (i > 0 && j > 0) {
                const int op = bt[i][j];
                if (op == 0 || op == 1) { // MATCH 或 SUB
                    aligned1.prepend(seq1[i - 1]);
                    aligned2.prepend(seq2[j - 1]);
                    --i;
                    --j;
                } else if (op == 2) { // DELETE
                    aligned1.prepend(seq1[i - 1]);
                    aligned2.prepend("-");
                    --i;
                } else { // INSERT
                    aligned1.prepend("-");
                    aligned2.prepend(seq2[j - 1]);
                    --j;
                }
            } else if (i > 0) {
                aligned1.prepend(seq1[i - 1]);
                aligned2.prepend("-");
                --i;
            } else { // j > 0
                aligned1.prepend("-");
                aligned2.prepend(seq2[j - 1]);
                --j;
            }
        }

        return {dp[len1][len2], aligned1, aligned2};
    }

    //----------------------------------------------------------
    // compute_edit_distance：仅计算编辑距离（空间优化版）
    //----------------------------------------------------------
    int SequenceAligner::compute_edit_distance(const QVector<QString> &seq1, const QVector<QString> &seq2) const {
        // 确保 seq1 是较长序列（便于空间优化）
        if (seq1.size() < seq2.size()) {
            return compute_edit_distance(seq2, seq1);
        }

        const int len1 = seq1.size();
        const int len2 = seq2.size();

        QVector<int> prev(len2 + 1);
        QVector<int> curr(len2 + 1);

        // 初始化第一行（seq2 为空时的插入代价）
        for (int j = 0; j <= len2; ++j) {
            prev[j] = j * m_insertion_cost;
        }

        for (int i = 1; i <= len1; ++i) {
            curr[0] = i * m_deletion_cost; // seq1 前缀删除代价
            const QString &c1 = seq1[i - 1];
            for (int j = 1; j <= len2; ++j) {
                const QString &c2 = seq2[j - 1];
                if (c1 == c2) {
                    curr[j] = prev[j - 1];
                } else {
                    int sub = prev[j - 1] + m_substitution_cost;
                    int del = prev[j] + m_deletion_cost;
                    int ins = curr[j - 1] + m_insertion_cost;
                    curr[j] = std::min({sub, del, ins});
                }
            }
            std::swap(prev, curr);
        }

        return prev[len2];
    }

    int SequenceAligner::find_exact_match(const QVector<QString> &input_seq, const QVector<QString> &reference_seq) {
        const int input_len = input_seq.size();
        const int ref_len = reference_seq.size();
        for (int start = 0; start <= ref_len - input_len; ++start) {
            bool match = true;
            for (int k = 0; k < input_len; ++k) {
                if (reference_seq[start + k] != input_seq[k]) {
                    match = false;
                    break;
                }
            }
            if (match)
                return start;
        }
        return -1;
    }

    std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
        SequenceAligner::build_exact_match_result(int start, const int length, const QVector<QString> &reference_seq,
                                                  const QVector<QString> *reference_text) {
        int end = start + length;
        QVector<QString> matched_phonetic = reference_seq.mid(start, length);
        QVector<QString> matched_text;
        if (reference_text) {
            matched_text = reference_text->mid(start, length);
        }

        QString matched_text_str;
        for (const QString &s : matched_text) {
            if (!matched_text_str.isEmpty())
                matched_text_str += ' ';
            matched_text_str += s;
        }
        return {matched_text_str, start, end, matched_phonetic, matched_text, QString()};
    }

    int SequenceAligner::compute_lcs_length(const QVector<QString> &seq1, const QVector<QString> &seq2) {
        // 保证 seq1 长度 >= seq2
        if (seq1.size() < seq2.size()) {
            return compute_lcs_length(seq2, seq1);
        }

        const int m = seq1.size();
        const int n = seq2.size();

        QVector<int> prev(n + 1, 0);
        QVector<int> curr(n + 1, 0);

        for (int i = 1; i <= m; ++i) {
            const QString &c1 = seq1[i - 1];
            for (int j = 1; j <= n; ++j) {
                if (c1 == seq2[j - 1]) {
                    curr[j] = prev[j - 1] + 1;
                } else {
                    curr[j] = std::max(prev[j], curr[j - 1]);
                }
            }
            std::swap(prev, curr);
        }
        return prev[n];
    }


    int SequenceAligner::determine_window_size(const int input_len, const int ref_len, const double max_scale,
                                               const int extra) {
        const int window = std::min(input_len + extra, static_cast<int>(input_len * max_scale));
        return std::min(window, ref_len);
    }

    std::pair<int, int> SequenceAligner::scan_windows(const QVector<QString> &input_seq,
                                                      const QVector<QString> &reference_seq,
                                                      const int window_size) const {

        const int ref_len = reference_seq.size();
        const int input_len = input_seq.size();

        QHash<QString, int> input_freq;
        for (const QString &s : input_seq) {
            input_freq[s] = input_freq.value(s, 0) + 1;
        }

        QVector<std::pair<int, int>> candidates;

        for (int start = 0; start <= ref_len - window_size; ++start) {
            QVector<QString> window = reference_seq.mid(start, window_size);

            bool has_overlap = false;
            for (const QString &w : window) {
                if (input_freq.contains(w)) {
                    has_overlap = true;
                    break;
                }
            }
            if (!has_overlap)
                continue;

            QHash<QString, int> window_freq;
            for (const QString &w : window) {
                window_freq[w] = window_freq.value(w, 0) + 1;
            }

            int overlap = 0;
            for (auto it = input_freq.begin(); it != input_freq.end(); ++it) {
                overlap += std::min(it.value(), window_freq.value(it.key(), 0));
            }
            const double coverage = static_cast<double>(overlap) / input_len;
            if (coverage < OVERLAP_THRESHOLD)
                continue;

            const int lcs = compute_lcs_length(input_seq, window);
            int approx_dist = input_len + window_size - 2 * lcs; // 与 Python 一致

            candidates.append({approx_dist, start});
        }

        if (candidates.isEmpty()) {
            return {-1, std::numeric_limits<int>::max()};
        }

        std::sort(candidates.begin(), candidates.end());

        const int total = candidates.size();
        int num_to_keep = std::max(10, static_cast<int>(0.3 * total));
        if (num_to_keep > total)
            num_to_keep = total;

        int best_start = -1;
        int min_edit_dist = std::numeric_limits<int>::max();

        for (int idx = 0; idx < num_to_keep; ++idx) {
            const int start = candidates[idx].second;
            QVector<QString> window = reference_seq.mid(start, window_size);
            const int edit_dist = compute_edit_distance(input_seq, window);
            if (edit_dist < min_edit_dist) {
                min_edit_dist = edit_dist;
                best_start = start;
                if (edit_dist == 0)
                    break;
            }
        }

        return {best_start, min_edit_dist};
    }

    std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
        SequenceAligner::build_match_from_alignment(const QVector<QString> &input_seq,
                                                    const QVector<QString> &reference_seq,
                                                    const QVector<QString> *reference_text, int best_start,
                                                    const int window_size) const {

        int window_end = best_start + window_size;
        const QVector<QString> window_seq = reference_seq.mid(best_start, window_size);

        auto [dist, aligned_input, aligned_window] = compute_alignment(input_seq, window_seq);

        QVector<QString> matched_phonetic;
        QVector<QString> matched_text;
        int win_idx = 0;

        for (int k = 0; k < aligned_window.size(); ++k) {
            const QString &win_char = aligned_window[k];
            const QString &inp_char = aligned_input[k];
            if (win_char != "-") {
                if (inp_char != "-") {
                    matched_phonetic.append(win_char);
                    if (reference_text && best_start + win_idx < reference_text->size()) {
                        matched_text.append((*reference_text)[best_start + win_idx]);
                    }
                }
                ++win_idx;
            }
        }

        if (matched_phonetic.isEmpty()) {
            return {QString(),
                    -1,
                    -1,
                    QVector<QString>(),
                    QVector<QString>(),
                    QStringLiteral("Alignment produced empty result")};
        }

        QString matched_text_str;
        for (const QString &s : matched_text) {
            if (!matched_text_str.isEmpty())
                matched_text_str += ' ';
            matched_text_str += s;
        }

        return {matched_text_str, best_start, window_end, matched_phonetic, matched_text, QString()};
    }

    std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
        SequenceAligner::find_best_match(const QVector<QString> &input_seq, const QVector<QString> &reference_seq,
                                         const QVector<QString> *reference_text, const double max_window_scale,
                                         const int extra_window) const {

        if (input_seq.isEmpty()) {
            return {
                QString(), -1, -1, QVector<QString>(), QVector<QString>(), QStringLiteral("Input sequence is empty")};
        }
        if (reference_seq.isEmpty()) {
            return {QString(),
                    -1,
                    -1,
                    QVector<QString>(),
                    QVector<QString>(),
                    QStringLiteral("Reference sequence is empty")};
        }

        const int input_len = input_seq.size();
        const int ref_len = reference_seq.size();

        if (input_len > ref_len) {
            return {QString(),
                    -1,
                    -1,
                    QVector<QString>(),
                    QVector<QString>(),
                    QStringLiteral("Input longer than reference")};
        }

        const int exact_start = find_exact_match(input_seq, reference_seq);
        if (exact_start != -1) {
            return build_exact_match_result(exact_start, input_len, reference_seq, reference_text);
        }

        const int window_size = determine_window_size(input_len, ref_len, max_window_scale, extra_window);

        auto [best_start, min_edit_dist] = scan_windows(input_seq, reference_seq, window_size);
        if (best_start == -1) {
            return {
                QString(), -1, -1, QVector<QString>(), QVector<QString>(), QStringLiteral("No matching window found")};
        }

        return build_match_from_alignment(input_seq, reference_seq, reference_text, best_start, window_size);
    }

    std::tuple<QString, QString, int, int, QString>
        SequenceAligner::find_best_match_and_return_lyrics(const QVector<QString> &input_pronunciation,
                                                           const QVector<QString> &reference_text,
                                                           const QVector<QString> &reference_pronunciation) const {

        auto [matched_text, start, end, matched_phonetic_list, matched_text_list, reason] =
            find_best_match(input_pronunciation, reference_pronunciation, &reference_text);

        QString matched_phonetic;
        for (const QString &s : matched_phonetic_list) {
            if (!matched_phonetic.isEmpty())
                matched_phonetic += ' ';
            matched_phonetic += s;
        }

        return {matched_text, matched_phonetic, start, end, reason};
    }

    int calculate_difference_count(const QVector<QString> &seq1, const QVector<QString> &seq2) {
        const int min_len = std::min(seq1.size(), seq2.size());
        int diff = 0;
        for (int i = 0; i < min_len; ++i) {
            if (seq1[i] != seq2[i])
                ++diff;
        }
        diff += std::abs(seq1.size() - seq2.size());
        return diff;
    }

} // namespace LyricFA