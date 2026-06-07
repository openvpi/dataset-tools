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
///   auto result = DsDocument::loadFile("song.ds");
///   if (!result.ok()) { /* handle error */ }
///   auto &doc = result.value();
///
///   // Read sentence fields via typed view
///   auto sv = doc.sentenceView(0);
///   double offset = sv.offset;
///   QString phSeq = sv.phSeq;
///
///   // Add or modify sentences via typed or raw access
///   doc.addRawSentence("{\"text\":\"hello\"}");
///   doc.setSentenceView(0, view);
///
///   // Save (preserves all unknown fields and original types)
///   auto saveResult = doc.saveFile("song.ds");
///   if (!saveResult.ok()) { /* handle error */ }
/// @endcode

#include <QString>
#include <dsfw/Result.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace dstools {

class DsDocument {
public:
    struct SentenceView {
        QString text;
        QString phSeq;
        QString phDur;
        QString phNum;
        QString noteSeq;
        QString noteDur;
        QString noteSlur;
        QString noteGlide;
        QString f0Seq;
        QString wordSpan;
        double offset = 0.0;
        double f0Timestep = 0.0;
    };

    DsDocument();
    ~DsDocument();

    DsDocument(DsDocument&& other) noexcept;
    DsDocument& operator=(DsDocument&& other) noexcept;

    DsDocument(const DsDocument&) = delete;
    DsDocument& operator=(const DsDocument&) = delete;

    // ── File I/O ──────────────────────────────────────────────────────

    static dsfw::Result<DsDocument> loadFile(const QString& path);

    dsfw::Result<void> saveFile(const QString& path = {}) const;

    // ── Typed sentence access ─────────────────────────────────────────

    int sentenceCount() const;
    bool isEmpty() const;

    SentenceView sentenceView(int index) const;
    void setSentenceView(int index, const SentenceView& view);
    void addSentenceView(const SentenceView& view);

    // ── Raw JSON access (for fields not covered by SentenceView) ──────

    std::string rawSentenceJsonString(int index) const;
    void addRawSentence(const std::string& jsonStr);
    void setRawSentence(int index, const std::string& jsonStr);

    // ── Field helpers ──────────────────────────────────────────────────

    const QString& filePath() const;
    void setFilePath(const QString& path);

    double durationSec() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace dstools