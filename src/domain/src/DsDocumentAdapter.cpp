#include <dstools/DsDocumentAdapter.h>

namespace dstools {

DsDocumentAdapter::DsDocumentAdapter(std::shared_ptr<DsDocument> inner)
    : m_inner(inner ? std::move(inner) : std::make_shared<DsDocument>()) {
}

QString DsDocumentAdapter::filePath() const {
    return m_filePath;
}

static const auto DsFileFormat = registerDocumentFormat("DsFile");

DocumentFormatId DsDocumentAdapter::format() const {
    return DsFileFormat;
}

QString DsDocumentAdapter::formatDisplayName() const {
    return QStringLiteral("DiffSinger DS");
}

bool DsDocumentAdapter::load(const QString &path, std::string &error) {
    auto result = loadFile(path);
    if (!result) {
        error = result.error();
        return false;
    }
    return true;
}

Result<void> DsDocumentAdapter::loadFile(const QString &path) {
    auto docResult = DsDocument::loadFile(path);
    if (!docResult)
        return Result<void>::Error(docResult.error());
    *m_inner = std::move(docResult.value());
    m_filePath = path;
    m_modified = false;
    return Result<void>::Ok();
}

bool DsDocumentAdapter::save(std::string &error) {
    auto result = saveFile();
    if (!result) {
        error = result.error();
        return false;
    }
    return true;
}

Result<void> DsDocumentAdapter::saveFile(const QString &path) {
    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty())
        return Result<void>::Error("No file path set");
    auto result = m_inner->saveFile(targetPath);
    if (!result)
        return result;
    if (path.isEmpty())
        m_modified = false;
    else {
        m_filePath = path;
        m_modified = false;
    }
    return Result<void>::Ok();
}

bool DsDocumentAdapter::saveAs(const QString &path, std::string &error) {
    auto result = saveFile(path);
    if (!result) {
        error = result.error();
        return false;
    }
    return true;
}

void DsDocumentAdapter::close() {
    m_inner = std::make_shared<DsDocument>();
    m_filePath.clear();
    m_modified = false;
}

bool DsDocumentAdapter::isModified() const {
    return m_modified;
}

void DsDocumentAdapter::setModified(bool modified) {
    m_modified = modified;
}

DocumentInfo DsDocumentAdapter::info() const {
    DocumentInfo di;
    di.filePath = m_filePath;
    di.format = DsFileFormat;
    di.isModified = m_modified;
    return di;
}

int DsDocumentAdapter::entryCount() const {
    return m_inner->sentenceCount();
}

double DsDocumentAdapter::durationSec() const {
    return m_inner->durationSec();
}

DsDocument *DsDocumentAdapter::inner() const {
    return m_inner.get();
}

std::shared_ptr<DsDocument> DsDocumentAdapter::innerShared() const {
    return m_inner;
}

} // namespace dstools
