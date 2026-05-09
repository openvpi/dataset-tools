#pragma once

/// @file PhNumCalculator.h
/// @brief Phoneme count calculator for DiffSinger ph_num attribute.
/// Equivalent to Python add_ph_num.py.

#include <QSet>
#include <QString>
#include <vector>

namespace dstools {

struct TranscriptionRow;

/// Calculate ph_num from phoneme sequences using vowel/consonant classification.
class PhNumCalculator {
public:
    PhNumCalculator() = default;

    /// Load vowel/consonant sets from a tab-separated dictionary file.
    /// Format: "syllable\tphoneme1 [phoneme2]"
    ///   - 1 phoneme → vowel
    ///   - 2 phonemes → first=consonant, second=vowel
    /// Default vowels {SP, AP, EP, GS, um} are always included.
    bool loadDictionary(const QString &dictPath, QString &error);

    /// Load vowels from a file (one phoneme per line).
    bool loadVowelsFromFile(const QString &path, QString &error);

    /// Load consonants from a file (one phoneme per line).
    bool loadConsonantsFromFile(const QString &path, QString &error);

    /// Manually set vowel/consonant sets.
    void setVowels(const QSet<QString> &vowels);
    void setConsonants(const QSet<QString> &consonants);

    /// Whether dictionary is loaded.
    bool isLoaded() const;

    /// Calculate ph_num for a single ph_seq string.
    /// Returns space-separated integers.
    bool calculate(const QString &phSeq, QString &phNum, QString &error) const;

    /// Batch: fill phNum for each row.
    bool calculateBatch(std::vector<TranscriptionRow> &rows, QString &error) const;

private:
    QSet<QString> m_vowels;
    QSet<QString> m_consonants;
    bool m_loaded = false;
};

} // namespace dstools
