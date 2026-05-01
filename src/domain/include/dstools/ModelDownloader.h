#pragma once

#include <dsfw/IModelDownloader.h>

#include <QNetworkAccessManager>
#include <QMap>

class QNetworkReply;

namespace dstools {

class ModelDownloader : public QObject, public IModelDownloader {
    Q_OBJECT
public:
    explicit ModelDownloader(QObject *parent = nullptr);
    ~ModelDownloader() override;

    const char *downloaderName() const override;

    Result<void> startDownload(const std::string &modelId, const std::string &url,
                               const std::filesystem::path &destPath,
                               const std::function<void(int)> &progress) override;

    Result<void> verifyChecksum(const std::filesystem::path &filePath,
                                const std::string &expectedHash) override;

private:
    QNetworkAccessManager m_nam;
    QMap<QString, QNetworkReply *> m_activeDownloads;
};

} // namespace dstools
