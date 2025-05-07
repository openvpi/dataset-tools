#include "LevenshteinDistance.h"

namespace LyricFA {
    FaRes LevenshteinDistance::find_similar_substrings(const QStringList &target, const QStringList &pinyin_list,
                                                       QStringList text_list, const bool &del_tip, const bool &ins_tip,
                                                       const bool &sub_tip) {
        if (text_list.isEmpty())
            text_list = pinyin_list;
        Q_ASSERT(target.size() <= pinyin_list.size());
        Q_ASSERT(text_list.size() == pinyin_list.size());

        const auto [start, end, textDiff, pinyinDiff] = find_best_matches(text_list, pinyin_list, target);
        const auto slider_res = pinyin_list.mid(start, end - start);
        if (!slider_res.empty() && (slider_res.first() == target.first() || slider_res.last() == target.last()) &&
            textDiff.size() <= 1)
            return {text_list.mid(start, end - start), pinyin_list.mid(start, end - start), textDiff, pinyinDiff};

        QList<CalcuRes> similar_substrings;

        // 扩展匹配范围，最多扩展10个字符
        for (int sub_length = static_cast<int>(target.size());
             sub_length < std::min(target.size() + 10, pinyin_list.size() + 1); sub_length++) {

            for (int i = std::max(0, start - 10);
                 i < std::min(end + 10, static_cast<int>(pinyin_list.size()) - sub_length + 1); i++) {

                similar_substrings.append(
                    calculate_edit_distance(text_list.mid(i, sub_length), pinyin_list.mid(i, sub_length), target));
            }
        }

        const auto calcuRes = findMinEditDistance(similar_substrings);

        return {calcuRes.text_res, calcuRes.pinyin_res,
                fill_step_out(calcuRes.corresponding_texts, del_tip, ins_tip, sub_tip),
                fill_step_out(calcuRes.corresponding_characters, del_tip, ins_tip, sub_tip)};
    }


    MatchRes LevenshteinDistance::find_best_matches(const QStringList &text_list, const QStringList &source_list,
                                                    const QStringList &sub_list) {
        int max_match_length = 0;
        int max_match_index = -1;

        for (int i = 0; i < source_list.size(); i++) {
            int match_length = 0;
            int j = 0;
            while (i + j < source_list.size() && j < sub_list.size()) {
                if (source_list[i + j] == sub_list[j]) {
                    match_length++;
                }
                j++;

                if (match_length > max_match_length) {
                    max_match_length = match_length;
                    max_match_index = i;
                }
            }
        }

        max_match_length = std::min(source_list.size() - max_match_index, sub_list.size());
        if (max_match_length < sub_list.size()) {
            max_match_index =
                std::max(0, static_cast<const int &>(max_match_index - (sub_list.size() - max_match_length)));
            max_match_length = std::min(source_list.size() - max_match_index, sub_list.size());
        }

        if (max_match_index == -1) {
            max_match_index = 0;
            max_match_length = static_cast<int>(sub_list.size());
        }

        QStringList textDiff;
        QStringList pinyinDiff;
        for (int k = 0; k < sub_list.size(); k++) {
            if (source_list[max_match_index + k] != sub_list[k]) {
                textDiff.append("(" + text_list[max_match_index + k] + "->" + sub_list[k] + ", " + QString::number(k) +
                                ")");
                pinyinDiff.append("(" + source_list[max_match_index + k] + "->" + sub_list[k] + ", " +
                                  QString::number(k) + ")");
            }
        }

        return {max_match_index, max_match_index + max_match_length, textDiff, pinyinDiff};
    }

    QStringList LevenshteinDistance::fill_step_out(const QList<StepPair> &pairs, const bool &del_tip,
                                                   const bool &ins_tip, const bool &sub_tip) {
        QStringList output;
        for (int i = 0; i < pairs.size(); i++) {
            const auto first = pairs[i].raw;
            const auto second = pairs[i].res;
            if (first != second) {
                if (!first.isEmpty() && second.isEmpty() && del_tip)
                    output.append("(" + first + "->, " + QString::number(i) + ")");
                else if (first.isEmpty() && !second.isEmpty() && ins_tip)
                    output.append("(->" + second + ", " + QString::number(i) + ")");
                else if (!first.isEmpty() && second.isEmpty() && sub_tip)
                    output.append("(" + first + "->" + second + ", " + QString::number(i) + ")");
            }
        }
        return output;
    }

    matrix LevenshteinDistance::init_dp_matrix(const int &m, const int &n, const int &del_cost, const int &ins_cost) {
        matrix dp;
        dp.resize(m + 1);
        for (int i = 0; i < m + 1; i++) {
            dp[i] = QVector<int>(n + 1, 0);
        }

        for (int i = 0; i < m + 1; i++) {
            dp[i][0] = i * del_cost;
        }

        for (int j = 0; j < n + 1; j++) {
            dp[0][j] = j * ins_cost;
        }

        return dp;
    }

    int LevenshteinDistance::calculate_edit_distance_dp(matrix dp, const QStringList &substring,
                                                        const QStringList &target, const bool &del_cost,
                                                        const bool &ins_cost, const bool &sub_cost) {
        const int m = static_cast<int>(substring.size());
        const int n = static_cast<int>(target.size());
        for (int i = 1; i < m + 1; i++) {
            for (int j = 1; j < n + 1; j++) {
                if (substring[i - 1] == target[j - 1])
                    dp[i][j] = dp[i - 1][j - 1];
                else {
                    dp[i][j] = std::min(std::min(dp[i - 1][j - 1] + sub_cost, dp[i][j - 1] + ins_cost),
                                        dp[i - 1][j] + del_cost);
                }
            }
        }

        return dp[m][n];
    }

    QPair<QList<StepPair>, QList<StepPair>> LevenshteinDistance::backtrack_corresponding(matrix dp,
                                                                                         const QStringList &text,
                                                                                         const QStringList &substring,
                                                                                         const QStringList &target) {
        QList<StepPair> corresponding_texts;
        QList<StepPair> corresponding_characters;
        int i = static_cast<int>(substring.size());
        int j = static_cast<int>(target.size());

        while (i > 0 && j > 0) {
            if (substring[i - 1] == target[j - 1]) {
                corresponding_characters.insert(0, {target[j - 1], target[j - 1]});
                corresponding_texts.insert(0, {text[i - 1], text[i - 1]});
                i--;
                j--;
            } else {
                const int min_cost = std::min(std::min(dp[i - 1][j - 1], dp[i][j - 1]), dp[i - 1][j]);

                if (dp[i - 1][j - 1] == min_cost) {
                    corresponding_characters.insert(0, {substring[i - 1], target[j - 1]});
                    corresponding_texts.insert(0, {text[i - 1], target[j - 1]});
                    i--;
                    j--;
                } else if (dp[i][j - 1] == min_cost) {
                    corresponding_characters.insert(0, {QString(), target[j - 1]});
                    corresponding_texts.insert(0, {QString(), target[j - 1]});
                    j--;
                } else {
                    corresponding_characters.insert(0, {substring[i - 1], QString()});
                    corresponding_texts.insert(0, {text[i - 1], QString()});
                    i--;
                }
            }
        }

        return {corresponding_texts, corresponding_characters};
    }

    CalcuRes LevenshteinDistance::calculate_edit_distance(const QStringList &_text, const QStringList &substring,
                                                          const QStringList &target, const int &del_cost,
                                                          const int &ins_cost, const int &sub_cost) {

        const int m = static_cast<int>(substring.size());
        const int n = static_cast<int>(target.size());
        const matrix dp = init_dp_matrix(m, n, del_cost, ins_cost);
        const int edit_distance = calculate_edit_distance_dp(dp, substring, target, del_cost, ins_cost, sub_cost);
        const auto [fst, snd] = backtrack_corresponding(dp, _text, substring, target);
        const auto corresponding_texts = fst;
        const auto corresponding_characters = snd;

        QStringList text_res;
        QStringList pinyin_res;
        for (int i = 0; i < corresponding_texts.size(); i++) {
            const auto &x = corresponding_characters[i];
            const auto &y = corresponding_texts[i];
            if (x.raw == x.res) {
                pinyin_res.append(x.raw);
                text_res.append(y.raw);
            } else if (x.raw.isEmpty() && !x.res.isEmpty()) {
                pinyin_res.append(x.res);
                text_res.append(y.res);
            } else if (!x.raw.isEmpty() && !x.res.isEmpty()) {
                pinyin_res.append(x.raw);
                text_res.append(y.raw);
            }
        }

        return {edit_distance, text_res, pinyin_res, corresponding_texts, corresponding_characters};
    }

    CalcuRes LevenshteinDistance::findMinEditDistance(const QList<CalcuRes> &similar_substrings) {
        if (similar_substrings.empty()) {
            return {};
        }

        int min_edit_distance = INT_MAX;
        const auto *min_edit_res = new CalcuRes();

        for (const CalcuRes &res : similar_substrings) {
            if (res.edit_distance < min_edit_distance) {
                min_edit_distance = res.edit_distance;
                min_edit_res = &res;
            }
        }

        return *min_edit_res;
    }
}