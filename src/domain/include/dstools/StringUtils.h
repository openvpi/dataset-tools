#pragma once

#include <QString>
#include <QStringList>
#include <dsfw/string_utils.h>
#include <string>
#include <tuple>
#include <vector>

namespace dstools {

    inline std::vector<std::string> parsePhSeq(const QString &phSeq) {
        return dsfw::splitToVector<std::string>(phSeq, QLatin1Char(' '));
    }

    inline std::vector<float> parsePhDur(const QString &phDur) {
        return dsfw::splitToVector<float>(phDur, QLatin1Char(' '));
    }

    inline std::vector<int> parsePhNum(const QString &phNum) {
        return dsfw::splitToVector<int>(phNum, QLatin1Char(' '));
    }

    struct TranscriptionRow;

    inline void writeNotesToRow(TranscriptionRow &row, const std::vector<std::tuple<std::string, float, int>> &notes);

} // namespace dstools

#include <dstools/TranscriptionCsv.h>

namespace dstools {

    inline void writeNotesToRow(TranscriptionRow &row, const std::vector<std::tuple<std::string, float, int>> &notes) {
        QStringList noteSeq, noteDur, noteSlur;
        noteSeq.reserve(static_cast<int>(notes.size()));
        noteDur.reserve(static_cast<int>(notes.size()));
        noteSlur.reserve(static_cast<int>(notes.size()));

        for (const auto &[name, dur, slur] : notes) {
            noteSeq << QString::fromStdString(name);
            noteDur << dsfw::formatDur6(dur);
            noteSlur << QString::number(slur);
        }

        row.noteSeq = noteSeq.join(QLatin1Char(' '));
        row.noteDur = noteDur.join(QLatin1Char(' '));
        row.noteSlur = noteSlur.join(QLatin1Char(' '));
    }

} // namespace dstools