#ifndef SEQUENCEALIGNER_H
#define SEQUENCEALIGNER_H

#include <QVector>
#include <tuple>

namespace LyricFA {

    class SequenceAligner {
    public:
        explicit SequenceAligner(int deletion_cost = 1, int insertion_cost = 1, int substitution_cost = 1);

        std::tuple<int, QVector<QString>, QVector<QString>> compute_alignment(const QVector<QString> &seq1,
                                                                              const QVector<QString> &seq2) const;

        int compute_edit_distance(const QVector<QString> &seq1, const QVector<QString> &seq2) const;

        std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
            find_best_match(const QVector<QString> &input_seq, const QVector<QString> &reference_seq,
                            const QVector<QString> *reference_text = nullptr, double max_window_scale = 1.3,
                            int extra_window = 8) const;

        std::tuple<QString, QString, int, int, QString>
            find_best_match_and_return_lyrics(const QVector<QString> &input_pronunciation,
                                              const QVector<QString> &reference_text,
                                              const QVector<QString> &reference_pronunciation) const;

    private:
        int m_deletion_cost;
        int m_insertion_cost;
        int m_substitution_cost;
        static constexpr double OVERLAP_THRESHOLD = 0.3;

        static int find_exact_match(const QVector<QString> &input_seq, const QVector<QString> &reference_seq);
        static std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
            build_exact_match_result(int start, int length, const QVector<QString> &reference_seq,
                                     const QVector<QString> *reference_text);
        static int compute_lcs_length(const QVector<QString> &seq1, const QVector<QString> &seq2);
        static int determine_window_size(int input_len, int ref_len, double max_scale, int extra);

        std::pair<int, int> scan_windows(const QVector<QString> &input_seq, const QVector<QString> &reference_seq,
                                         int window_size) const;

        std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
            build_match_from_alignment(const QVector<QString> &input_seq, const QVector<QString> &reference_seq,
                                       const QVector<QString> *reference_text, int best_start, int window_size) const;
    };

    // 全局函数：计算两个序列的差异计数（逐元素比较 + 长度差）
    int calculate_difference_count(const QVector<QString> &seq1, const QVector<QString> &seq2);

} // namespace LyricFA

#endif // SEQUENCEALIGNER_H