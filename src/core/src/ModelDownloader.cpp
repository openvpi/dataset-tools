#include <dstools/ModelDownloader.h>

#include <QFile>

namespace dstools {

ModelDownloader::ModelDownloader(QObject *parent) : QObject(parent) {
}

ModelDownloader::~ModelDownloader() {
    for (auto *reply : m_activeDownloads) {
        reply->abort();
        reply->deleteLater();
    }
}

bool ModelDownloader::startDownload(const QString &modelId, const QUrl &url, const QString &destPath, DownloadProgressCallback progress, std::string &error) {
    if (m_activeDownloads.contains(modelId)) {
        error = "Download already in progress for model: " + modelId.toStdString();
        return false;
    }

    auto *file = new QFile(destPath, this);
    if (!file->open(QIODevice::WriteOnly)) {
        error = "Failed to open destination file: " + destPath.toStdString();
        delete file;
        return false;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_nam.get(request);
    m_activeDownloads.insert(modelId, reply);
    m_statuses.insert(modelId, DownloadStatus::Downloading);

    connect(reply, &QNetworkReply::readyRead, this, [file, reply]() {
        file->write(reply->readAll());
    });

    if (progress) {
        connect(reply, &QNetworkReply::downloadProgress, this, [progress](qint64 received, qint64 total) {
            progress(static_cast<int64_t>(received), static_cast<int64_t>(total));
        });
    }

    connect(reply, &QNetworkReply::finished, this, [this, modelId, file, reply]() {
        const QString filePath = file->fileName();
        file->close();
        file->deleteLater();

        m_activeDownloads.remove(modelId);

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            m_statuses[modelId] = DownloadStatus::Cancelled;
            QFile::remove(filePath);
        } else if (reply->error() != QNetworkReply::NoError) {
            m_statuses[modelId] = DownloadStatus::Failed;
            QFile::remove(filePath);
        } else {
            m_statuses[modelId] = DownloadStatus::Completed;
        }

        reply->deleteLater();
    });

    return true;
}

void ModelDownloader::cancelDownload(const QString &modelId) {
    if (auto *reply = m_activeDownloads.value(modelId, nullptr)) {
        reply->abort();
    }
}

DownloadStatus ModelDownloader::downloadStatus(const QString &modelId) const {
    return m_statuses.value(modelId, DownloadStatus::Idle);
}

bool ModelDownloader::verifyChecksum(const QString &filePath, const QString &expectedHash, std::string &error) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Failed to open file for checksum: " + filePath.toStdString();
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        error = "Failed to read file data for checksum";
        return false;
    }

    const QString computed = QString::fromLatin1(hash.result().toHex());
    if (computed.compare(expectedHash, Qt::CaseInsensitive) != 0) {
        error = "Checksum mismatch: expected " + expectedHash.toStdString() + ", got " + computed.toStdString();
        return false;
    }

    return true;
}

} // namespace dstools
