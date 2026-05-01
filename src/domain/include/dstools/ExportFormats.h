#pragma once

/// @file ExportFormats.h
/// @brief Concrete export format implementations (HTS labels, Sinsy XML).

#include <dsfw/IExportFormat.h>

namespace dstools {

/// @brief Exports dataset items to HTS-style label files.
class HtsLabelExportFormat : public IExportFormat {
public:
    /// @brief Return the human-readable format name.
    const char *formatName() const override;

    /// @brief Return the file extension used by this format.
    const char *formatExtension() const override;

    /// @brief Export a single item to an HTS label file.
    /// @param sourceFile Path to the source dataset file.
    /// @param workingDir Working directory for intermediate data.
    /// @param outputPath Destination file path.
    /// @return Success or an error description.
    Result<void> exportItem(const std::filesystem::path &sourceFile,
                            const std::filesystem::path &workingDir,
                            const std::filesystem::path &outputPath) override;

    /// @brief Export all items in a directory to HTS label files.
    /// @param workingDir Directory containing source items.
    /// @param outputDir Destination directory.
    /// @return Success or an error description.
    Result<void> exportAll(const std::filesystem::path &workingDir,
                           const std::filesystem::path &outputDir) override;
};

/// @brief Exports dataset items to Sinsy XML format.
class SinsyXmlExportFormat : public IExportFormat {
public:
    /// @brief Return the human-readable format name.
    const char *formatName() const override;

    /// @brief Return the file extension used by this format.
    const char *formatExtension() const override;

    /// @brief Export a single item to Sinsy XML.
    /// @param sourceFile Path to the source dataset file.
    /// @param workingDir Working directory for intermediate data.
    /// @param outputPath Destination file path.
    /// @return Success or an error description.
    Result<void> exportItem(const std::filesystem::path &sourceFile,
                            const std::filesystem::path &workingDir,
                            const std::filesystem::path &outputPath) override;

    /// @brief Export all items in a directory to Sinsy XML files.
    /// @param workingDir Directory containing source items.
    /// @param outputDir Destination directory.
    /// @return Success or an error description.
    Result<void> exportAll(const std::filesystem::path &workingDir,
                           const std::filesystem::path &outputDir) override;
};

} // namespace dstools
