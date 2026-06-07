#include <dstools/DsDocumentAdapter.h>

#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>

#include <fstream>
#include <filesystem>

namespace dstools {

// ─── DsDocumentAdapter ────────────────────────────────────────────────────────

DsDocumentAdapter::DsDocumentAdapter(DsDocument& doc) : m_doc(doc) {
}

bool DsDocumentAdapter::isModified() const {
    return false;  // DsDocument does not track modification state
}

dsfw::Result<void> DsDocumentAdapter::load(const std::filesystem::path& path) {
    QString qpath = dsfw::PathUtils::fromStdPath(path);
    auto result = DsDocument::loadFile(qpath);
    if (!result) {
        return dsfw::Err(result.error());
    }
    m_doc = std::move(result.value());
    return dsfw::Ok();
}

dsfw::Result<void> DsDocumentAdapter::save() {
    return m_doc.saveFile();
}

dsfw::Result<void> DsDocumentAdapter::saveAs(const std::filesystem::path& path) {
    QString qpath = dsfw::PathUtils::fromStdPath(path);
    return m_doc.saveFile(qpath);
}

}  // namespace dstools
