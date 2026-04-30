#pragma once

/// @file IModelDownloader.h
/// @brief Model download interface with progress reporting and checksum verification.

#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>
#include <string>

namespace dstools {

/// @brief Status of a model download operation.
enum class DownloadStatus {
    Idle,        ///< No download in progress.
    Downloading, ///< Download is actively transferring.
    Verifying,   ///< Download complete, verifying integrity.
    Completed,   ///< Download and verification succeeded.
    Failed,      ///< Download failed.
    Cancelled    ///< Download was cancelled by the user.
};

/// @brief Callback for download progress updates.
/// @param received Bytes received so far.
/// @param total Total bytes expected (-1 if unknown).
using DownloadProgressCallback = std::function<void(int64_t received, int64_t total)>;

/// @brief Abstract interface for downloading ML model files.
class IModelDownloader {
public:
    virtual ~IModelDownloader() = default;
    /// @brief Start downloading a model.
    /// @param modelId Unique model identifier.
    /// @param url Download URL.
    /// @param destPath Local destination file path.
    /// @param progress Progress callback invoked during transfer.
    /// @param error Populated with error description on failure.
    /// @return True if the download was started successfully.
    virtual bool startDownload(const QString &modelId, const QUrl &url, const QString &destPath, DownloadProgressCallback progress, std::string &error) = 0;
    /// @brief Cancel an active download.
    /// @param modelId Model identifier of the download to cancel.
    virtual void cancelDownload(const QString &modelId) = 0;
    /// @brief Query the status of a download.
    /// @param modelId Model identifier.
    /// @return Current download status.
    virtual DownloadStatus downloadStatus(const QString &modelId) const = 0;
    /// @brief Verify a downloaded file's SHA-256 checksum.
    /// @param filePath Path to the file to verify.
    /// @param expectedHash Expected hex-encoded hash string.
    /// @param error Populated with error description on failure.
    /// @return True if the checksum matches.
    virtual bool verifyChecksum(const QString &filePath, const QString &expectedHash, std::string &error) const = 0;
};

/// @brief No-op model downloader stub for use as a placeholder.
class StubModelDownloader : public IModelDownloader {
public:
    bool startDownload(const QString &, const QUrl &, const QString &, DownloadProgressCallback, std::string &error) override {
        error = "Model downloading not implemented";
        return false;
    }
    void cancelDownload(const QString &) override {}
    DownloadStatus downloadStatus(const QString &) const override { return DownloadStatus::Idle; }
    bool verifyChecksum(const QString &, const QString &, std::string &error) const override {
        error = "Checksum verification not implemented";
        return false;
    }
};

} // namespace dstools
