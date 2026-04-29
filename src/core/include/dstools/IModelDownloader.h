#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>
#include <string>

namespace dstools {

enum class DownloadStatus {
    Idle,
    Downloading,
    Verifying,
    Completed,
    Failed,
    Cancelled
};

using DownloadProgressCallback = std::function<void(int64_t received, int64_t total)>;

class IModelDownloader {
public:
    virtual ~IModelDownloader() = default;
    virtual bool startDownload(const QString &modelId, const QUrl &url, const QString &destPath, DownloadProgressCallback progress, std::string &error) = 0;
    virtual void cancelDownload(const QString &modelId) = 0;
    virtual DownloadStatus downloadStatus(const QString &modelId) const = 0;
    virtual bool verifyChecksum(const QString &filePath, const QString &expectedHash, std::string &error) const = 0;
};

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
