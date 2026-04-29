#pragma once

#include <dstools/IQualityMetrics.h>

namespace dstools {

class QualityMetrics : public IQualityMetrics {
public:
    QString providerName() const override;
    QStringList availableMetrics() const override;
    ItemQualityReport evaluate(const QString &sourceFile, const QString &workingDir, std::string &error) const override;
    std::vector<ItemQualityReport> evaluateBatch(const QString &workingDir, std::string &error) const override;
    double aggregateScore(const std::vector<ItemQualityReport> &reports) const override;
};

} // namespace dstools
