#pragma once
/// @file SequenceAligner.h
/// @brief Edit-distance based sequence alignment for phonetic matching.

#include <QVector>
#include <tuple>

namespace LyricFA {

    /// @brief Performs sequence alignment using configurable edit distance costs,
    ///        supporting exact match detection, windowed best-match search, and
    ///        full alignment computation.
    class SequenceAligner {
    public:
        /// @brief Construct a sequence aligner with configurable costs.
        /// @param deletion_cost Cost of a deletion operation.
        /// @param insertion_cost Cost of an insertion operation.
        /// @param substitution_cost Cost of a substitution operation.
        explicit SequenceAligner(int deletion_cost = 1, int insertion_cost = 1, int substitution_cost = 1);

        /// @brief Compute full alignment between two sequences.
        /// @param seq1 First sequence.
        /// @param seq2 Second sequence.
        /// @return Tuple of (edit_distance, aligned_seq1, aligned_seq2).
        std::tuple<int, QVector<QString>, QVector<QString>> compute_alignment(const QVector<QString> &seq1,
                                                                              const QVector<QString> &seq2) const;

        /// @brief Compute edit distance between two sequences.
        /// @param seq1 First sequence.
        /// @param seq2 Second sequence.
        /// @return The edit distance value.
        int compute_edit_distance(const QVector<QString> &seq1, const QVector<QString> &seq2) const;

        /// @brief Find the best matching window in a reference sequence.
        /// @param input_seq Input phonetic sequence to match.
        /// @param reference_seq Reference phonetic sequence to search within.
        /// @param reference_text Optional reference text for output mapping.
        /// @param max_window_scale Maximum window scale factor.
        /// @param extra_window Extra window padding size.
        /// @return Tuple of (matched_text, start, end, aligned_input, aligned_ref, highlighted_diff).
        std::tuple<QString, int, int, QVector<QString>, QVector<QString>, QString>
            find_best_match(const QVector<QString> &input_seq, const QVector<QString> &reference_seq,
                            const QVector<QString> *reference_text = nullptr, double max_window_scale = 1.3,
                            int extra_window = 8) const;

        /// @brief Find best match and return corresponding lyric text.
        /// @param input_pronunciation Input phonetic sequence.
        /// @param reference_text Reference lyric text sequence.
        /// @param reference_pronunciation Reference phonetic sequence.
        /// @return Tuple of (matched_text, matched_phonetic, start, end, highlighted_diff).
        std::tuple<QString, QString, int, int, QString>
            find_best_match_and_return_lyrics(const QVector<QString> &input_pronunciation,
                                              const QVector<QString> &reference_text,
                                              const QVector<QString> &reference_pronunciation) const;

    private:
        int m_deletion_cost;     ///< Cost of a deletion operation.
        int m_insertion_cost;    ///< Cost of an insertion operation.
        int m_substitution_cost; ///< Cost of a substitution operation.
        static constexpr double OVERLAP_THRESHOLD = 0.3; ///< Minimum overlap ratio for match validity.

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

    /// @brief Calculate element-wise difference count plus length difference between two sequences.
    /// @param seq1 First sequence.
    /// @param seq2 Second sequence.
    /// @return Total difference count.
    int calculate_difference_count(const QVector<QString> &seq1, const QVector<QString> &seq2);

} // namespace LyricFA
