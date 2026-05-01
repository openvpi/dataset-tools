#pragma once

/// @file IModelDownloader.h
/// @brief Model download service interface.

#include <dsfw/ModelTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <functional>
#include <string>

namespace dstools {

/// @brief Abstract interface for downloading and verifying ML model files.
class IModelDownloader {
public:
    virtual ~IModelDownloader() = default;

    /// @brief Return the downloader identifier.
    /// @return Downloader name string.
    virtual const char *downloaderName() const = 0;

    /// @brief Begin an async model download with progress reporting.
    /// @param modelId Model identifier.
    /// @param url Download URL.
    /// @param destPath Destination file path.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    virtual Result<void> startDownload(const std::string &modelId, const std::string &url,
                                       const std::filesystem::path &destPath,
                                       const std::function<void(int)> &progress) = 0;

    /// @brief Verify file integrity against an expected hash.
    /// @param filePath Path to the file to verify.
    /// @param expectedHash Expected checksum string.
    /// @return Success or error.
    virtual Result<void> verifyChecksum(const std::filesystem::path &filePath,
                                        const std::string &expectedHash) = 0;
};

} // namespace dstools
