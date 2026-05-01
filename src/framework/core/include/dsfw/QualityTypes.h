#pragma once

/// @file QualityTypes.h
/// @brief Data types for dataset quality evaluation reports.

#include <string>

namespace dstools {

/// @brief Quality metrics for a single dataset item.
struct ItemQualityReport {
    std::string sourceFile;      ///< Path to the source audio file.
    int phonemeCount = 0;        ///< Number of phonemes in the item.
    float durationSec = 0.0f;    ///< Audio duration in seconds.
};

} // namespace dstools
