#pragma once

/// @file IDocument.h
/// @brief Document interface with load/save/close lifecycle and format metadata.

#include <QString>
#include <QDateTime>

#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

/// @brief Supported document file formats.
enum class DocumentFormat {
    DsFile,     ///< DiffSinger .ds JSON file.
    TextGrid,   ///< Praat TextGrid file.
    TransCsv,   ///< Transcription CSV file.
    LabFile,    ///< HTK .lab label file.
    JsonLabel,  ///< JSON-based label file.
    Unknown     ///< Unrecognized format.
};

/// @brief Snapshot of a document's metadata.
struct DocumentInfo {
    QString filePath;                              ///< Absolute file path.
    DocumentFormat format = DocumentFormat::Unknown; ///< File format.
    QDateTime lastModified;                        ///< Last modification timestamp.
    bool isModified = false;                       ///< True if there are unsaved changes.
};

/// @brief Abstract interface for loadable/saveable documents.
class IDocument {
public:
    virtual ~IDocument() = default;

    /// @brief Return the document's file path.
    /// @return Absolute file path.
    virtual QString filePath() const = 0;
    /// @brief Return the document's format.
    /// @return Document format enum.
    virtual DocumentFormat format() const = 0;
    /// @brief Return a human-readable format name (e.g. "TextGrid").
    /// @return Display name string.
    virtual QString formatDisplayName() const = 0;

    /// @brief Load a document from the given path.
    /// @param path File path to load.
    /// @param error Populated with error description on failure.
    /// @return True on success.
    virtual bool load(const QString &path, std::string &error) = 0;
    /// @brief Save the document to its current path.
    /// @param error Populated with error description on failure.
    /// @return True on success.
    virtual bool save(std::string &error) = 0;
    /// @brief Save the document to a new path.
    /// @param path Destination file path.
    /// @param error Populated with error description on failure.
    /// @return True on success.
    virtual bool saveAs(const QString &path, std::string &error) = 0;
    /// @brief Close the document and release resources.
    virtual void close() = 0;

    /// @brief Check whether the document has unsaved modifications.
    /// @return True if modified.
    virtual bool isModified() const = 0;
    /// @brief Set the modification flag.
    /// @param modified New modification state.
    virtual void setModified(bool modified) = 0;
    /// @brief Return a snapshot of the document's metadata.
    /// @return DocumentInfo struct.
    virtual DocumentInfo info() const = 0;

    /// @brief Return the number of entries (utterances, segments, etc.).
    /// @return Entry count, or 0 if not applicable.
    virtual int entryCount() const { return 0; }
    /// @brief Return the total audio duration in seconds.
    /// @return Duration in seconds, or 0.0 if unknown.
    virtual double durationSec() const { return 0.0; }
};

} // namespace dstools
