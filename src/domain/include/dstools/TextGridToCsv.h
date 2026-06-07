#pragma once

/// @file TextGridToCsv.h
/// @brief TextGrid → TranscriptionRow conversion for DiffSinger pipeline Step 6.

#include <QString>
#include <dstools/TranscriptionCsv.h>
#include <dsfw/Result.h>
#include <vector>

namespace dstools {

/// TextGrid → TranscriptionRow 转换
class TextGridToCsv {
public:
    [[nodiscard]] static dsfw::Result<TranscriptionRow> extractFromTextGrid(const QString& tgPath);

    [[nodiscard]] static dsfw::Result<std::vector<TranscriptionRow>> extractDirectory(const QString& dir);
};

} // namespace dstools
