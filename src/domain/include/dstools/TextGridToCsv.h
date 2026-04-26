#pragma once

/// @file TextGridToCsv.h
/// @brief TextGrid → TranscriptionRow conversion for DiffSinger pipeline Step 6.

#include <QString>
#include <dstools/TranscriptionCsv.h>
#include <dstools/Result.h>
#include <vector>

namespace dstools {

/// TextGrid → TranscriptionRow 转换
class TextGridToCsv {
public:
    [[nodiscard]] static Result<TranscriptionRow> extractFromTextGrid(const QString& tgPath);

    [[nodiscard]] static Result<std::vector<TranscriptionRow>> extractDirectory(const QString& dir);
};

} // namespace dstools
