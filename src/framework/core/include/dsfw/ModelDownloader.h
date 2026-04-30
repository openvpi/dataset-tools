#pragma once

/// @file ModelDownloader.h
/// @brief IModelDownloader implementation using QNetworkAccessManager.

#include <dsfw/IModelDownloader.h>

#include <QCryptographicHash>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace dstools {

/// @brief Concrete model downloader using Qt networking.
///
/// Manages concurrent downloads via QNetworkAccessManager and tracks per-model status.
/// @note Requires a running Qt event loop for network callbacks.
class ModelDownloader : public QObject, public IModelDownloader {
    Q_OBJECT
public:
    /// @brief Construct a model downloader.
    /// @param parent Optional QObject parent.
    explicit ModelDownloader(QObject *parent = nullptr);
    ~ModelDownloader() override;

    /// @brief Start downloading a model file.
    /// @param modelId Unique model identifier.
    /// @param url Download URL.
    /// @param destPath Local destination file path.
    /// @param progress Progress callback.
    /// @param error Populated with error description on failure.
    /// @return True if the download was started successfully.
    bool startDownload(const QString &modelId, const QUrl &url, const QString &destPath, DownloadProgressCallback progress, std::string &error) override;
    /// @brief Cancel an active download.
    /// @param modelId Model identifier.
    void cancelDownload(const QString &modelId) override;
    /// @brief Query the status of a download.
    /// @param modelId Model identifier.
    /// @return Current download status.
    DownloadStatus downloadStatus(const QString &modelId) const override;
    /// @brief Verify a downloaded file's checksum.
    /// @param filePath Path to the downloaded file.
    /// @param expectedHash Expected hex-encoded SHA-256 hash.
    /// @param error Populated with error description on failure.
    /// @return True if the checksum matches.
    bool verifyChecksum(const QString &filePath, const QString &expectedHash, std::string &error) const override;

private:
    QNetworkAccessManager m_nam;
    QMap<QString, QNetworkReply *> m_activeDownloads;
    QMap<QString, DownloadStatus> m_statuses;
};

} // namespace dstools
