#pragma once

/// @file QualityMetrics.h
/// @brief Default dataset quality metrics implementation.

#include <dsfw/IQualityMetrics.h>

namespace dstools {

/// @brief Concrete IQualityMetrics that evaluates phoneme count and
///        duration metrics.
class QualityMetrics : public IQualityMetrics {
public:
    /// @brief Return the human-readable metrics name.
    const char *metricsName() const override;

    /// @brief Evaluate quality metrics for a single source file.
    /// @param sourceFile Path to the source dataset file.
    /// @param workingDir Working directory with intermediate data.
    /// @return An ItemQualityReport or an error.
    Result<ItemQualityReport> evaluate(const std::filesystem::path &sourceFile,
                                       const std::filesystem::path &workingDir) override;

    /// @brief Evaluate quality metrics for all items in a directory.
    /// @param workingDir Directory containing dataset items.
    /// @return A vector of ItemQualityReport or an error.
    Result<std::vector<ItemQualityReport>> evaluateBatch(const std::filesystem::path &workingDir) override;
};

} // namespace dstools
