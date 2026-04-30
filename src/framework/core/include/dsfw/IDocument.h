#pragma once

#include <QString>
#include <QDateTime>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

enum class DocumentFormat {
    DsFile,
    TextGrid,
    TransCsv,
    LabFile,
    JsonLabel,
    Unknown
};

struct DocumentInfo {
    QString filePath;
    DocumentFormat format = DocumentFormat::Unknown;
    QDateTime lastModified;
    bool isModified = false;
};

class IDocument {
public:
    virtual ~IDocument() = default;

    virtual QString filePath() const = 0;
    virtual DocumentFormat format() const = 0;
    virtual QString formatDisplayName() const = 0;

    virtual bool load(const QString &path, std::string &error) = 0;
    virtual bool save(std::string &error) = 0;
    virtual bool saveAs(const QString &path, std::string &error) = 0;
    virtual void close() = 0;

    virtual bool isModified() const = 0;
    virtual void setModified(bool modified) = 0;
    virtual DocumentInfo info() const = 0;

    virtual int entryCount() const { return 0; }
    virtual double durationSec() const { return 0.0; }
};

} // namespace dstools
