#pragma once

/// @file ExportFormats.h
/// @brief Concrete IExportFormat implementations for HTS Label and Sinsy XML.

#include <dstools/IExportFormat.h>

#include <QStringList>

namespace dstools {

/// Exports DS files to HTS full-context label format.
/// Output: one line per phoneme with start_time end_time phoneme
/// Times are in 100ns units (HTK standard).
class HtsLabelExportFormat : public IExportFormat {
public:
    ExportFormatInfo info() const override;
    QStringList requiredInputFormats() const override;
    bool exportItem(const QString &sourceFile, const QString &workingDir,
                    const QString &outputPath, std::string &error) const override;
    bool exportAll(const QString &workingDir, const QString &outputDir,
                   std::string &error) const override;
};

/// Exports DS files to Sinsy-compatible MusicXML format.
/// Generates a basic MusicXML with note/lyric data from DS sentences.
class SinsyXmlExportFormat : public IExportFormat {
public:
    ExportFormatInfo info() const override;
    QStringList requiredInputFormats() const override;
    bool exportItem(const QString &sourceFile, const QString &workingDir,
                    const QString &outputPath, std::string &error) const override;
    bool exportAll(const QString &workingDir, const QString &outputDir,
                   std::string &error) const override;
};

} // namespace dstools
