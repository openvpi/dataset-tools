#pragma once

/// @file PhNumCalculator.h
/// @brief Phoneme count calculator for DiffSinger ph_num attribute.
/// Equivalent to Python add_ph_num.py.

#include <QSet>
#include <QString>
#include <vector>

#include <dsfw/Result.h>

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
    /// Default special words {SP, AP, EP, GS} (each phNum=1 standalone).
    /// Default vowel {um}. All phonemes are classified from the dictionary.
    [[nodiscard]] dsfw::Result<void> loadDictionary(const QString& dictPath);

    /// Load vowels from a file (one phoneme per line).
    [[nodiscard]] dsfw::Result<void> loadVowelsFromFile(const QString& path);

    /// Load consonants from a file (one phoneme per line).
    [[nodiscard]] dsfw::Result<void> loadConsonantsFromFile(const QString& path);

    /// Manually set vowel/consonant sets.
    void setVowels(const QSet<QString>& vowels);
    void setConsonants(const QSet<QString>& consonants);

    /// Set special words that each have exactly 1 phoneme (e.g. SP, AP, EP, GS).
    /// These are treated as standalone phNum=1 units regardless of surrounding consonants.
    void setSpecialWords(const QSet<QString>& words);

    /// Whether dictionary is loaded.
    bool isLoaded() const;

    /// Calculate ph_num for a single ph_seq string.
    /// Returns space-separated integers.
    [[nodiscard]] dsfw::Result<QString> calculate(const QString& phSeq) const;

    /// Batch: fill phNum for each row.
    [[nodiscard]] dsfw::Result<void> calculateBatch(std::vector<TranscriptionRow>& rows) const;

private:
    QSet<QString> m_vowels;
    QSet<QString> m_consonants;
    QSet<QString> m_specialWords;
    bool m_loaded = false;
};

}  // namespace dstools
