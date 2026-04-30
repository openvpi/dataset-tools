#pragma once

/// @file IG2PProvider.h
/// @brief Grapheme-to-phoneme (G2P) conversion interface.

#include <QString>
#include <QStringList>

#include <string>
#include <vector>

namespace dstools {

/// @brief Result of converting a single word to phonemes.
struct G2PResult {
    QString word;           ///< Original input word/grapheme.
    QStringList phonemes;   ///< Resulting phoneme sequence.
    QString language;       ///< Language code used for conversion.
};

/// @brief Abstract interface for grapheme-to-phoneme conversion.
class IG2PProvider {
public:
    virtual ~IG2PProvider() = default;
    /// @brief Return a unique identifier for this provider.
    /// @return Provider ID string.
    virtual QString providerId() const = 0;
    /// @brief Return the list of supported language codes.
    /// @return Language codes (e.g. "zh", "ja", "en").
    virtual QStringList supportedLanguages() const = 0;
    /// @brief Check whether a language is supported.
    /// @param langCode Language code to check.
    /// @return True if supported.
    virtual bool supportsLanguage(const QString &langCode) const = 0;
    /// @brief Convert a text string to phonemes.
    /// @param text Input text (may contain multiple words).
    /// @param langCode Language code for conversion.
    /// @param error Populated with error description on failure.
    /// @return Vector of per-word G2P results.
    virtual std::vector<G2PResult> convert(const QString &text, const QString &langCode, std::string &error) const = 0;
    /// @brief Convert a single word to phonemes.
    /// @param word Input word.
    /// @param langCode Language code for conversion.
    /// @param error Populated with error description on failure.
    /// @return G2P result for the word.
    virtual G2PResult convertWord(const QString &word, const QString &langCode, std::string &error) const = 0;
};

/// @brief No-op G2P provider stub for use as a placeholder.
class StubG2PProvider : public IG2PProvider {
public:
    QString providerId() const override { return "stub"; }
    QStringList supportedLanguages() const override { return {}; }
    bool supportsLanguage(const QString &) const override { return false; }
    std::vector<G2PResult> convert(const QString &, const QString &, std::string &error) const override {
        error = "G2P provider not implemented";
        return {};
    }
    G2PResult convertWord(const QString &, const QString &, std::string &error) const override {
        error = "G2P provider not implemented";
        return {};
    }
};

} // namespace dstools
