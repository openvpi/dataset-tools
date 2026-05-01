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

    bool startDownload(const QString &modelId, const QUrl &url, const QString &destPath, DownloadProgressCallback progress, std::string &error) override;
    void cancelDownload(const QString &modelId) override;
    DownloadStatus downloadStatus(const QString &modelId) const override;
    bool verifyChecksum(const QString &filePath, const QString &expectedHash, std::string &error) const override;

private:
    QNetworkAccessManager m_nam;
    QMap<QString, QNetworkReply *> m_activeDownloads;
    QMap<QString, DownloadStatus> m_statuses;
};

} // namespace dstools
