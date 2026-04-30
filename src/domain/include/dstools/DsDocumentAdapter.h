#pragma once

#include <dsfw/IDocument.h>
#include <dstools/DsDocument.h>

#include <memory>

namespace dstools {

class DsDocumentAdapter : public IDocument {
public:
    explicit DsDocumentAdapter(std::shared_ptr<DsDocument> inner = nullptr);

    // Identity
    QString filePath() const override;
    DocumentFormatId format() const override;
    QString formatDisplayName() const override;

    // Lifecycle
    bool load(const QString &path, std::string &error) override;
    bool save(std::string &error) override;
    bool saveAs(const QString &path, std::string &error) override;
    void close() override;

    // State
    bool isModified() const override;
    void setModified(bool modified) override;
    DocumentInfo info() const override;

    // Content access
    int entryCount() const override;
    double durationSec() const override;

    // Inner access
    DsDocument *inner() const;
    std::shared_ptr<DsDocument> innerShared() const;

private:
    std::shared_ptr<DsDocument> m_inner;
    QString m_filePath;
    bool m_modified = false;
};

} // namespace dstools
