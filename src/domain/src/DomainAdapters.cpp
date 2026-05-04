#include <dstools/DsDocumentAdapter.h>
#include <dstools/ModelDownloader.h>

#include <dsfw/JsonHelper.h>

#include <fstream>
#include <filesystem>

namespace dstools {

// ─── DsDocumentAdapter ────────────────────────────────────────────────────────

    DsDocumentAdapter::DsDocumentAdapter(DsDocument &doc) : m_doc(doc) {}

    bool DsDocumentAdapter::isModified() const {
        return false; // DsDocument does not track modification state
    }

    Result<void> DsDocumentAdapter::load(const std::filesystem::path &path) {
        QString qpath = QString::fromStdWString(path.wstring());
        auto result = DsDocument::loadFile(qpath);
        if (!result) {
            return Err(result.error());
        }
        m_doc = std::move(result.value());
        return Ok();
    }

    Result<void> DsDocumentAdapter::save() {
        return m_doc.saveFile();
    }

    Result<void> DsDocumentAdapter::saveAs(const std::filesystem::path &path) {
        QString qpath = QString::fromStdWString(path.wstring());
        return m_doc.saveFile(qpath);
    }

// ─── ModelDownloader ──────────────────────────────────────────────────────────

    ModelDownloader::ModelDownloader(QObject *parent) : QObject(parent) {}
    ModelDownloader::~ModelDownloader() = default;

    const char *ModelDownloader::downloaderName() const {
        return "ModelDownloader";
    }

    Result<void> ModelDownloader::startDownload(const std::string &modelId, const std::string &url,
                                                 const std::filesystem::path &destPath,
                                                 const std::function<void(int)> &progress) {
        (void)modelId;
        (void)url;
        (void)destPath;
        (void)progress;
        return Err("Download not implemented");
    }

    Result<void> ModelDownloader::verifyChecksum(const std::filesystem::path &filePath,
                                                   const std::string &expectedHash) {
        (void)filePath;
        (void)expectedHash;
        return Err("Checksum verification not implemented");
    }

} // namespace dstools
