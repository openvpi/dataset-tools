#include <dsfw/IDocumentFormat.h>
#include <dstools/DsDocumentAdapter.h>
#include <dstools/DsItemManager.h>

#include <dsfw/JsonHelper.h>

namespace dstools {

    DsDocumentAdapter::DsDocumentAdapter(DsDocument &doc) : m_doc(doc) {}

    bool DsDocumentAdapter::isModified() const {
        return m_doc.isModified();
    }

    Result<void> DsDocumentAdapter::load(const std::filesystem::path &path) {
        std::string error;
        if (!m_doc.load(path, error)) {
            return Err(error);
        }
        return Ok();
    }

    Result<void> DsDocumentAdapter::save() {
        std::string error;
        if (!m_doc.save(error)) {
            return Err(error);
        }
        return Ok();
    }

    Result<void> DsDocumentAdapter::saveAs(const std::filesystem::path &path) {
        std::string error;
        if (!m_doc.save(path, error)) {
            return Err(error);
        }
        return Ok();
    }

    IDocumentFormat *DsDocumentAdapter::format() const {
        return nullptr;
    }

} // namespace dstools
