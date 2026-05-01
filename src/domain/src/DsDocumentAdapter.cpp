#include <dsfw/IDocumentFormat.h>
#include <dstools/DsDocumentAdapter.h>
#include <dstools/DsItemManager.h>

#include <dsfw/JsonHelper.h>

namespace dstools {

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

    IDocumentFormat *DsDocumentAdapter::format() const {
        return nullptr;
    }

} // namespace dstools
