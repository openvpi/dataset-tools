#pragma once

#include <dstools/DsItemRecord.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

class DsItemManager {
public:
    DsItemManager() = default;

    Result<void> load(const std::filesystem::path &dsitemPath, DsItemRecord &record);
    Result<void> save(const DsItemRecord &record, const std::filesystem::path &dsitemPath);

    Result<DsItemRecord::Status> queryStatus(const std::filesystem::path &dsitemPath);
    Result<bool> needsProcessing(const std::filesystem::path &dsitemPath);

    Result<void> markCompleted(const std::filesystem::path &dsitemPath);
    Result<void> markFailed(const std::filesystem::path &dsitemPath, const std::string &errorMsg);

    struct StepSummary {
        int total = 0;
        int completed = 0;
        int failed = 0;
        int pending = 0;
    };

    Result<StepSummary> summarizeStep(const std::filesystem::path &workingDir);
};

} // namespace dstools
