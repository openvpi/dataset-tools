#pragma once

#include <dsfw/QualityTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

class IQualityMetrics {
public:
    virtual ~IQualityMetrics() = default;

    virtual const char *metricsName() const = 0;

    virtual Result<ItemQualityReport> evaluate(const std::filesystem::path &sourceFile,
                                               const std::filesystem::path &workingDir) = 0;

    virtual Result<std::vector<ItemQualityReport>> evaluateBatch(const std::filesystem::path &workingDir) = 0;
};

} // namespace dstools
