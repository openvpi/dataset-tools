#pragma once

/// @file DsItemManager.h
/// @brief Manager for .dsitem record persistence and status tracking.

#include <dstools/DsItemRecord.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

/// @brief Handles loading, saving, and querying processing status of
///        individual dataset items (.dsitem files).
class DsItemManager {
public:
    DsItemManager() = default;

    /// @brief Load a record from a .dsitem file.
    /// @param dsitemPath Path to the .dsitem file.
    /// @param record Output record populated from the file.
    /// @return Success or an error description.
    Result<void> load(const std::filesystem::path &dsitemPath, DsItemRecord &record);

    /// @brief Persist a record to a .dsitem file.
    /// @param record Record to save.
    /// @param dsitemPath Destination .dsitem file path.
    /// @return Success or an error description.
    Result<void> save(const DsItemRecord &record, const std::filesystem::path &dsitemPath);

    /// @brief Query the processing status of an item.
    /// @param dsitemPath Path to the .dsitem file.
    /// @return The item's current Status or an error.
    Result<DsItemRecord::Status> queryStatus(const std::filesystem::path &dsitemPath);

    /// @brief Check whether an item still requires processing.
    /// @param dsitemPath Path to the .dsitem file.
    /// @return True if the item is pending, or an error.
    Result<bool> needsProcessing(const std::filesystem::path &dsitemPath);

    /// @brief Mark an item as successfully completed.
    /// @param dsitemPath Path to the .dsitem file.
    /// @return Success or an error description.
    Result<void> markCompleted(const std::filesystem::path &dsitemPath);

    /// @brief Mark an item as failed with a reason.
    /// @param dsitemPath Path to the .dsitem file.
    /// @param errorMsg Description of the failure.
    /// @return Success or an error description.
    Result<void> markFailed(const std::filesystem::path &dsitemPath, const std::string &errorMsg);

    /// @brief Aggregate status counts for a processing step.
    struct StepSummary {
        int total = 0;     ///< Total number of items.
        int completed = 0; ///< Items completed successfully.
        int failed = 0;    ///< Items that failed.
        int pending = 0;   ///< Items still pending.
    };

    /// @brief Aggregate status counts across all items in a directory.
    /// @param workingDir Directory containing .dsitem files.
    /// @return A StepSummary or an error.
    Result<StepSummary> summarizeStep(const std::filesystem::path &workingDir);
};

} // namespace dstools
