#pragma once

/// @file IExportFormat.h
/// @brief Export format interface for converting dataset items to output files.

#include <QString>
#include <QStringList>

#include <string>

namespace dstools {

/// @brief Descriptor for an export format.
struct ExportFormatInfo {
    QString id;             ///< Unique format identifier.
    QString displayName;    ///< Human-readable name.
    QString fileExtension;  ///< Output file extension (e.g. ".csv").
    QString description;    ///< Short description of the format.
};

/// @brief Abstract interface for exporting dataset items to a specific format.
class IExportFormat {
public:
    virtual ~IExportFormat() = default;
    /// @brief Return metadata about this export format.
    /// @return ExportFormatInfo descriptor.
    virtual ExportFormatInfo info() const = 0;
    /// @brief Export a single item.
    /// @param sourceFile Source file path.
    /// @param workingDir Working directory containing related assets.
    /// @param outputPath Destination output file path.
    /// @param error Populated with error description on failure.
    /// @return True on success.
    virtual bool exportItem(const QString &sourceFile, const QString &workingDir, const QString &outputPath, std::string &error) const = 0;
    /// @brief Export all items in a working directory.
    /// @param workingDir Source working directory.
    /// @param outputDir Destination output directory.
    /// @param error Populated with error description on failure.
    /// @return True on success.
    virtual bool exportAll(const QString &workingDir, const QString &outputDir, std::string &error) const = 0;
    /// @brief Return the list of input format IDs required by this exporter.
    /// @return List of format IDs, empty if none required.
    virtual QStringList requiredInputFormats() const { return {}; }
};

/// @brief No-op export format stub for use as a placeholder.
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
