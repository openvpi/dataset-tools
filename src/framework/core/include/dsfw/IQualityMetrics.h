#pragma once

/// @file IQualityMetrics.h
/// @brief Quality assessment interface for evaluating dataset items.

#include <QString>
#include <QStringList>
#include <QMap>

#include <string>
#include <vector>

namespace dstools {

/// @brief A single quality metric measurement.
struct MetricResult {
    QString metricName;  ///< Metric identifier (e.g. "SNR", "MOS").
    double value = 0.0;  ///< Measured value.
    QString unit;        ///< Unit string (e.g. "dB", "score").
    QString description; ///< Human-readable description of the metric.
};

/// @brief Quality report for a single dataset item.
struct ItemQualityReport {
    QString sourceFile;                  ///< Source audio file path.
    std::vector<MetricResult> metrics;   ///< Individual metric results.
    double overallScore = 0.0;           ///< Aggregate quality score.
};

/// @brief Abstract interface for computing quality metrics on dataset items.
class IQualityMetrics {
public:
    virtual ~IQualityMetrics() = default;
    /// @brief Return a human-readable name for this metrics provider.
    /// @return Provider name string.
    virtual QString providerName() const = 0;
    /// @brief Return the list of metric names this provider can compute.
    /// @return List of metric name strings.
    virtual QStringList availableMetrics() const = 0;
    /// @brief Evaluate quality metrics for a single item.
    /// @param sourceFile Source audio file path.
    /// @param workingDir Working directory containing related assets.
    /// @param error Populated with error description on failure.
    /// @return Quality report for the item.
    virtual ItemQualityReport evaluate(const QString &sourceFile, const QString &workingDir, std::string &error) const = 0;
    /// @brief Evaluate quality metrics for all items in a directory.
    /// @param workingDir Working directory to scan.
    /// @param error Populated with error description on failure.
    /// @return Vector of quality reports.
    virtual std::vector<ItemQualityReport> evaluateBatch(const QString &workingDir, std::string &error) const = 0;
    /// @brief Compute an aggregate score from multiple reports.
    /// @param reports Individual item reports.
    /// @return Aggregate quality score.
    virtual double aggregateScore(const std::vector<ItemQualityReport> &reports) const = 0;
};

/// @brief No-op quality metrics stub for use as a placeholder.
class StubQualityMetrics : public IQualityMetrics {
public:
    QString providerName() const override { return "stub"; }
    QStringList availableMetrics() const override { return {}; }
    ItemQualityReport evaluate(const QString &, const QString &, std::string &error) const override {
        error = "Quality metrics not implemented";
        return {};
    }
    std::vector<ItemQualityReport> evaluateBatch(const QString &, std::string &error) const override {
        error = "Quality metrics not implemented";
        return {};
    }
    double aggregateScore(const std::vector<ItemQualityReport> &) const override { return 0.0; }
};

} // namespace dstools
