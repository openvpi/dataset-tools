#pragma once

#include <dsfw/IQualityMetrics.h>

namespace dstools {

class QualityMetrics : public IQualityMetrics {
public:
    const char *metricsName() const override;

    Result<ItemQualityReport> evaluate(const std::filesystem::path &sourceFile,
                                       const std::filesystem::path &workingDir) override;

    Result<std::vector<ItemQualityReport>> evaluateBatch(const std::filesystem::path &workingDir) override;
};

} // namespace dstools
