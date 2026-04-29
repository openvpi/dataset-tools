#include <dstools/DsDocumentAdapter.h>

namespace dstools {

DsDocumentAdapter::DsDocumentAdapter(std::shared_ptr<DsDocument> inner)
    : m_inner(inner ? std::move(inner) : std::make_shared<DsDocument>()) {
}

QString DsDocumentAdapter::filePath() const {
    return m_filePath;
}

DocumentFormat DsDocumentAdapter::format() const {
    return DocumentFormat::DsFile;
}

QString DsDocumentAdapter::formatDisplayName() const {
    return QStringLiteral("DiffSinger DS");
}

bool DsDocumentAdapter::load(const QString &path, std::string &error) {
    QString qerror;
    auto doc = DsDocument::load(path, qerror);
    if (doc.isEmpty() && !qerror.isEmpty()) {
        error = qerror.toStdString();
        return false;
    }
    *m_inner = std::move(doc);
    m_filePath = path;
    m_modified = false;
    return true;
}

bool DsDocumentAdapter::save(std::string &error) {
    if (m_filePath.isEmpty()) {
        error = "No file path set";
        return false;
    }
    QString qerror;
    if (!m_inner->save(m_filePath, qerror)) {
        error = qerror.toStdString();
        return false;
    }
    m_modified = false;
    return true;
}

bool DsDocumentAdapter::saveAs(const QString &path, std::string &error) {
    QString qerror;
    if (!m_inner->save(path, qerror)) {
        error = qerror.toStdString();
        return false;
    }
    m_filePath = path;
    m_modified = false;
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
    di.format = DocumentFormat::DsFile;
    di.isModified = m_modified;
    return di;
}

int DsDocumentAdapter::entryCount() const {
    return m_inner->sentenceCount();
}

double DsDocumentAdapter::durationSec() const {
    return 0.0; // TODO: sum offsets + durations across sentences
}

DsDocument *DsDocumentAdapter::inner() const {
    return m_inner.get();
}

std::shared_ptr<DsDocument> DsDocumentAdapter::innerShared() const {
    return m_inner;
}

} // namespace dstools
