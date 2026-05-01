#pragma once

/// @file ModelDownloader.h
/// @brief HTTP-based model downloader implementation using Qt Network.

#include <dsfw/IModelDownloader.h>

#include <QNetworkAccessManager>
#include <QMap>

class QNetworkReply;

namespace dstools {

/// @brief Concrete IModelDownloader using QNetworkAccessManager for
///        downloading ML models with progress tracking.
class ModelDownloader : public QObject, public IModelDownloader {
    Q_OBJECT
public:
    /// @brief Construct a ModelDownloader.
    /// @param parent Optional parent QObject.
    explicit ModelDownloader(QObject *parent = nullptr);

    /// @brief Destructor; cancels any active downloads.
    ~ModelDownloader() override;

    /// @brief Return the human-readable downloader name.
    const char *downloaderName() const override;

    /// @brief Start downloading a model from a URL.
    /// @param modelId Unique identifier for the model.
    /// @param url Download URL.
    /// @param destPath Local destination path.
    /// @param progress Callback invoked with percent complete (0–100).
    /// @return Success or an error description.
    Result<void> startDownload(const std::string &modelId, const std::string &url,
                               const std::filesystem::path &destPath,
                               const std::function<void(int)> &progress) override;

    /// @brief Verify a downloaded file against an expected hash.
    /// @param filePath Path to the file to verify.
    /// @param expectedHash Expected hash string.
    /// @return Success or an error description.
    Result<void> verifyChecksum(const std::filesystem::path &filePath,
                                const std::string &expectedHash) override;

private:
    QNetworkAccessManager m_nam;                   ///< Qt network access manager.
    QMap<QString, QNetworkReply *> m_activeDownloads; ///< In-flight download replies keyed by model ID.
};

} // namespace dstools
