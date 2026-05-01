#pragma once

#include <dsfw/IDocument.h>
#include <dstools/DsDocument.h>

#include <memory>

namespace dstools {

class DsDocumentAdapter : public IDocument {
public:
    explicit DsDocumentAdapter(DsDocument &doc);

    bool isModified() const override;

    Result<void> load(const std::filesystem::path &path) override;
    Result<void> save() override;
    Result<void> saveAs(const std::filesystem::path &path) override;

    IDocumentFormat *format() const override;

private:
    DsDocument &m_doc;
};

} // namespace dstools
