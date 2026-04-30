#pragma once

#include <QString>
#include <QStringList>

#include <string>

namespace dstools {

struct ExportFormatInfo {
    QString id;
    QString displayName;
    QString fileExtension;
    QString description;
};

class IExportFormat {
public:
    virtual ~IExportFormat() = default;
    virtual ExportFormatInfo info() const = 0;
    virtual bool exportItem(const QString &sourceFile, const QString &workingDir, const QString &outputPath, std::string &error) const = 0;
    virtual bool exportAll(const QString &workingDir, const QString &outputDir, std::string &error) const = 0;
    virtual QStringList requiredInputFormats() const { return {}; }
};

class StubExportFormat : public IExportFormat {
public:
    ExportFormatInfo info() const override {
        return {"stub", "Stub Format", ".stub", "Not implemented"};
    }
    bool exportItem(const QString &, const QString &, const QString &, std::string &error) const override {
        error = "Export format not implemented";
        return false;
    }
    bool exportAll(const QString &, const QString &, std::string &error) const override {
        error = "Export format not implemented";
        return false;
    }
};

} // namespace dstools
