#pragma once

/// @file IExportFormat.h
/// @brief Export format interface for converting dataset items to external formats.

#include <dsfw/ExportTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>

namespace dstools {

/// @brief Abstract interface for dataset export formats (e.g. HTS labels, Sinsy XML).
class IExportFormat {
public:
    virtual ~IExportFormat() = default;

    /// @brief Return the human-readable format name.
    /// @return Format name string.
    virtual const char *formatName() const = 0;

    /// @brief Return the file extension for this format.
    /// @return File extension string.
    virtual const char *formatExtension() const = 0;

    /// @brief Export a single dataset item.
    /// @param sourceFile Path to the source file.
    /// @param workingDir Working directory for intermediate files.
    /// @param outputPath Destination output path.
    /// @return Success or error.
    virtual Result<void> exportItem(const std::filesystem::path &sourceFile,
                                    const std::filesystem::path &workingDir,
                                    const std::filesystem::path &outputPath) = 0;

    /// @brief Batch export all items in a directory.
    /// @param workingDir Working directory containing source items.
    /// @param outputDir Destination output directory.
    /// @return Success or error.
    virtual Result<void> exportAll(const std::filesystem::path &workingDir,
                                   const std::filesystem::path &outputDir) = 0;
};

} // namespace dstools
