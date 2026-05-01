#pragma once

/// @file IQualityMetrics.h
/// @brief Dataset quality evaluation interface.

#include <dsfw/QualityTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

/// @brief Abstract interface for computing quality metrics on dataset items.
class IQualityMetrics {
public:
    virtual ~IQualityMetrics() = default;

    /// @brief Return the metric set identifier (e.g. "snr", "pesq").
    /// @return Null-terminated metric name string.
    virtual const char *metricsName() const = 0;

    /// @brief Evaluate quality metrics for a single dataset item.
    /// @param sourceFile Path to the source audio file.
    /// @param workingDir Working directory containing associated data.
    /// @return ItemQualityReport on success, Err on failure.
    virtual Result<ItemQualityReport> evaluate(const std::filesystem::path &sourceFile,
                                               const std::filesystem::path &workingDir) = 0;

    /// @brief Evaluate quality metrics for all items in a directory.
    /// @param workingDir Directory containing dataset items to evaluate.
    /// @return Vector of ItemQualityReport on success, Err on failure.
    virtual Result<std::vector<ItemQualityReport>> evaluateBatch(const std::filesystem::path &workingDir) = 0;
};

} // namespace dstools
