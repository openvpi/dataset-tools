#pragma once

/// @file StringUtils.h
/// @brief String parsing and formatting utilities for DiffSinger data fields.

#include <QString>
#include <QStringList>

#include <string>
#include <tuple>
#include <vector>

namespace dstools {

/// @brief Format a duration to at most 6 decimal places, removing trailing zeros.
/// @param d Duration value.
/// @return Formatted string.
inline QString formatDur6(double d) {
    QString s = QString::number(d, 'f', 6);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0')))
            s.chop(1);
        if (s.endsWith(QLatin1Char('.')))
            s.chop(1);
    }
    return s;
}

/// @brief Parse a space-separated phoneme sequence.
/// @param phSeq Space-delimited phoneme string.
/// @return Vector of phoneme strings.
inline std::vector<std::string> parsePhSeq(const QString &phSeq) {
    const QStringList parts = phSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    std::vector<std::string> result;
    result.reserve(parts.size());
    for (const auto &s : parts)
        result.push_back(s.toStdString());
    return result;
}

/// @brief Parse space-separated phoneme durations.
/// @param phDur Space-delimited duration string.
/// @return Vector of duration values.
inline std::vector<float> parsePhDur(const QString &phDur) {
    const QStringList parts = phDur.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    std::vector<float> result;
    result.reserve(parts.size());
    for (const auto &s : parts)
        result.push_back(s.toFloat());
    return result;
}

/// @brief Parse space-separated phoneme counts.
/// @param phNum Space-delimited count string.
/// @return Vector of phoneme counts per note.
inline std::vector<int> parsePhNum(const QString &phNum) {
    const QStringList parts = phNum.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    std::vector<int> result;
    result.reserve(parts.size());
    for (const auto &s : parts)
        result.push_back(s.toInt());
    return result;
}

struct TranscriptionRow;

/// @brief Populate a TranscriptionRow with note sequence data.
/// @param row Output row to populate.
/// @param notes Vector of (noteName, duration, slur) tuples.
inline void writeNotesToRow(TranscriptionRow &row,
                            const std::vector<std::tuple<std::string, float, int>> &notes);

} // namespace dstools

#include <dstools/TranscriptionCsv.h>

namespace dstools {

inline void writeNotesToRow(TranscriptionRow &row,
                            const std::vector<std::tuple<std::string, float, int>> &notes) {
    QStringList noteSeq, noteDur, noteSlur;
    noteSeq.reserve(static_cast<int>(notes.size()));
    noteDur.reserve(static_cast<int>(notes.size()));
    noteSlur.reserve(static_cast<int>(notes.size()));

    for (const auto &[name, dur, slur] : notes) {
        noteSeq << QString::fromStdString(name);
        noteDur << formatDur6(dur);
        noteSlur << QString::number(slur);
    }

    row.noteSeq = noteSeq.join(QLatin1Char(' '));
    row.noteDur = noteDur.join(QLatin1Char(' '));
    row.noteSlur = noteSlur.join(QLatin1Char(' '));
}

} // namespace dstools
