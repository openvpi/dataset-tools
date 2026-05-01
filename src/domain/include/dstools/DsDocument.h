#pragma once

/// @file DsDocument.h
/// @brief Centralized DS (DiffSinger) file I/O with round-trip safety.
///
/// DsDocument encapsulates the DS JSON array format, providing:
/// - Safe file I/O with correct path encoding (Unicode-safe on Windows)
/// - Round-trip preservation of unknown fields and original value types
/// - Atomic writes via JsonHelper
///
/// A DS file is a JSON array of sentence objects. Each sentence may contain
/// fields like offset, text, ph_seq, ph_dur, note_seq, note_dur, etc.
/// Fields such as offset and f0_timestep may be stored as number or string;
/// DsDocument preserves their original JSON type across load/save cycles.
///
/// Usage:
/// @code
///   QString error;
///   auto doc = DsDocument::load("song.ds", error);
///   if (doc.isEmpty()) { /* handle error */ }
///
///   // Read/modify sentence fields
///   auto &s = doc.sentence(0);
///   s["note_seq"] = "C4 D4 E4";
///
///   // Read a field that may be number or string
///   double offset = DsDocument::numberOrString(s, "offset", 0.0);
///
///   // Save (preserves all unknown fields and original types)
///   if (!doc.save("song.ds", error)) { /* handle error */ }
/// @endcode

#include <nlohmann/json.hpp>

#include <QString>
#include <dstools/Result.h>

#include <vector>

namespace dstools {

/// In-memory representation of a DS file (JSON array of sentence objects).
class DsDocument {
public:
    DsDocument() = default;

    // ── File I/O ──────────────────────────────────────────────────────

    /// Load a DS file. Returns an empty document on failure.
    /// @param path   File path (QString, Unicode-safe)
    /// @param error  Set to error description on failure, empty on success
    [[deprecated("Use loadFile() returning Result<DsDocument> instead")]]
    static DsDocument load(const QString &path, QString &error);

    /// Load a DS file (Result-based API).
    static Result<DsDocument> loadFile(const QString &path);

    /// Save the document to a DS file.
    /// @param path   File path (QString, Unicode-safe). Empty = use original path.
    /// @param error  Set to error description on failure
    /// @return true on success
    [[deprecated("Use saveFile() returning Result<void> instead")]]
    bool save(const QString &path, QString &error) const;
    [[deprecated("Use saveFile() returning Result<void> instead")]]
    bool save(QString &error) const;

    /// Save the document (Result-based API).
    Result<void> saveFile(const QString &path = {}) const;

    // ── Sentence access ───────────────────────────────────────────────

    /// Number of sentences in the document.
    int sentenceCount() const;

    /// Whether the document has no sentences.
    bool isEmpty() const;

    /// Access a sentence by index (mutable). No bounds check — caller must verify index.
    nlohmann::json &sentence(int index);
    const nlohmann::json &sentence(int index) const;

    /// Access the underlying array (for iteration).
    std::vector<nlohmann::json> &sentences();
    const std::vector<nlohmann::json> &sentences() const;

    /// Original file path (set by load()).
    const QString &filePath() const;
    void setFilePath(const QString &path);

    // ── Field helpers (number-or-string ambiguity) ────────────────────

    /// Read a field that may be stored as JSON number or string.
    /// Returns the numeric value regardless of storage type.
    /// Returns defaultValue if the key is absent or unparseable.
    static double numberOrString(const nlohmann::json &obj, const std::string &key,
                                 double defaultValue = 0.0);

    /// Calculate the total audio duration across all sentences.
    /// Sums offset + ph_dur for each sentence, returns the maximum end time.
    double durationSec() const;

    // ── Path encoding ─────────────────────────────────────────────────

    /// Convert QString to std::filesystem::path with correct encoding.
    /// Uses wstring on Windows to handle Unicode paths safely.
    static std::filesystem::path toFsPath(const QString &qpath);

private:
    std::vector<nlohmann::json> m_sentences;
    QString m_filePath;
};

} // namespace dstools
