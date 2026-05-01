#pragma once

/// @file DsItemRecord.h
/// @brief Data structure for a single dataset item's processing record.

#include <string>

namespace dstools {

/// @brief Tracks the processing state of one dataset item.
struct DsItemRecord {
    /// @brief Processing states for a dataset item.
    enum class Status {
        Pending = 0,   ///< Not yet processed.
        Completed = 1, ///< Successfully processed.
        Failed = 2     ///< Processing failed.
    };

    Status status = Status::Pending; ///< Current processing status.
    std::string inputFile;           ///< Path to the source input file.
    std::string outputFile;          ///< Path to the generated output file.
    std::string errorMsg;            ///< Error message (populated on failure).
    std::string timestamp;           ///< ISO timestamp of the last status change.
};

} // namespace dstools
