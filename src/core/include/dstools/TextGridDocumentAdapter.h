#pragma once

#include <dstools/IDocument.h>

#include <memory>

namespace dstools {
namespace phonemelabeler {
class TextGridDocument; // forward declaration
}

class TextGridDocumentAdapter : public IDocument {
public:
    explicit TextGridDocumentAdapter(
        std::shared_ptr<phonemelabeler::TextGridDocument> inner = nullptr);

    // Identity
    QString filePath() const override;
    DocumentFormat format() const override;
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
    phonemelabeler::TextGridDocument *inner() const;
    std::shared_ptr<phonemelabeler::TextGridDocument> innerShared() const;

private:
    std::shared_ptr<phonemelabeler::TextGridDocument> m_inner;
    QString m_filePath;
    bool m_modified = false;
};

} // namespace dstools
