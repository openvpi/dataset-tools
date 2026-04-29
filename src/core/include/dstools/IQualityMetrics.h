#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

#include <string>
#include <vector>

namespace dstools {

struct MetricResult {
    QString metricName;
    double value = 0.0;
    QString unit;
    QString description;
};

struct ItemQualityReport {
    QString sourceFile;
    std::vector<MetricResult> metrics;
    double overallScore = 0.0;
};

class IQualityMetrics {
public:
    virtual ~IQualityMetrics() = default;
    virtual QString providerName() const = 0;
    virtual QStringList availableMetrics() const = 0;
    virtual ItemQualityReport evaluate(const QString &sourceFile, const QString &workingDir, std::string &error) const = 0;
    virtual std::vector<ItemQualityReport> evaluateBatch(const QString &workingDir, std::string &error) const = 0;
    virtual double aggregateScore(const std::vector<ItemQualityReport> &reports) const = 0;
};

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
