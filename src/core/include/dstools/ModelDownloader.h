#pragma once

#include <dstools/IModelDownloader.h>

#include <QCryptographicHash>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace dstools {

class ModelDownloader : public QObject, public IModelDownloader {
    Q_OBJECT
public:
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
