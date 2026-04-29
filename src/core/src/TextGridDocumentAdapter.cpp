// TODO: TextGridDocumentAdapter is a stub. The real implementation requires
// access to dstools::phonemelabeler::TextGridDocument which lives in the
// PhonemeLabeler app target. Full implementation deferred to TASK-3.2
// (PhonemeLabeler page componentization) when the dependency graph allows it.

#include <dstools/TextGridDocumentAdapter.h>

namespace dstools {

TextGridDocumentAdapter::TextGridDocumentAdapter(
    std::shared_ptr<phonemelabeler::TextGridDocument> inner)
    : m_inner(std::move(inner)) {
}

QString TextGridDocumentAdapter::filePath() const {
    return m_filePath;
}

DocumentFormat TextGridDocumentAdapter::format() const {
    return DocumentFormat::TextGrid;
}

QString TextGridDocumentAdapter::formatDisplayName() const {
    return QStringLiteral("Praat TextGrid");
}

bool TextGridDocumentAdapter::load(const QString & /*path*/, std::string &error) {
    // TODO: Requires TextGridDocument — stub until TASK-3.2
    error = "TextGridDocumentAdapter::load() not yet implemented";
    return false;
}

bool TextGridDocumentAdapter::save(std::string &error) {
    error = "TextGridDocumentAdapter::save() not yet implemented";
    return false;
}

bool TextGridDocumentAdapter::saveAs(const QString & /*path*/, std::string &error) {
    error = "TextGridDocumentAdapter::saveAs() not yet implemented";
    return false;
}

void TextGridDocumentAdapter::close() {
    m_inner.reset();
    m_filePath.clear();
    m_modified = false;
}

bool TextGridDocumentAdapter::isModified() const {
    return m_modified;
}

void TextGridDocumentAdapter::setModified(bool modified) {
    m_modified = modified;
}

DocumentInfo TextGridDocumentAdapter::info() const {
    DocumentInfo di;
    di.filePath = m_filePath;
    di.format = DocumentFormat::TextGrid;
    di.isModified = m_modified;
    return di;
}

int TextGridDocumentAdapter::entryCount() const {
    return 0; // TODO: delegate to m_inner->tierCount()
}

double TextGridDocumentAdapter::durationSec() const {
    return 0.0; // TODO: delegate to m_inner->totalDuration()
}

phonemelabeler::TextGridDocument *TextGridDocumentAdapter::inner() const {
    return m_inner.get();
}

std::shared_ptr<phonemelabeler::TextGridDocument>
TextGridDocumentAdapter::innerShared() const {
    return m_inner;
}

} // namespace dstools
