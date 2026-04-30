#pragma once

/// @file IDocument.h
/// @brief Document interface with load/save/close lifecycle and format metadata.

#include <QString>
#include <QDateTime>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace dstools {

/// @brief Extensible document format identifier (replaces fixed enum).
class DocumentFormatId {
public:
    constexpr DocumentFormatId() : m_id(-1) {}
    constexpr explicit DocumentFormatId(int id) : m_id(id) {}
    constexpr int id() const { return m_id; }
    constexpr bool isValid() const { return m_id >= 0; }
    constexpr bool operator==(const DocumentFormatId &o) const { return m_id == o.m_id; }
    constexpr bool operator!=(const DocumentFormatId &o) const { return m_id != o.m_id; }
    constexpr bool operator<(const DocumentFormatId &o) const { return m_id < o.m_id; }
private:
    int m_id;
};

/// @brief Register (or look up) a named document format, returning a stable ID.
inline DocumentFormatId registerDocumentFormat(const std::string &name) {
    static int s_nextId = 0;
    static std::unordered_map<std::string, int> s_registry;
    auto it = s_registry.find(name);
    if (it != s_registry.end())
        return DocumentFormatId(it->second);
    int id = s_nextId++;
    s_registry[name] = id;
    return DocumentFormatId(id);
}

/// @brief Well-known "Unknown" format (invalid ID).
inline const DocumentFormatId DocFmt_Unknown{};

/// @brief Snapshot of a document's metadata.
struct DocumentInfo {
    QString filePath;                            ///< Absolute file path.
    DocumentFormatId format = DocFmt_Unknown;     ///< File format.
    QDateTime lastModified;                      ///< Last modification timestamp.
    bool isModified = false;                     ///< True if there are unsaved changes.
};

/// @brief Abstract interface for loadable/saveable documents.
class IDocument {
public:
    virtual ~IDocument() = default;

    /// @brief Return the document's file path.
    /// @return Absolute file path.
    virtual QString filePath() const = 0;
    /// @brief Return the document's format.
    /// @return Document format ID.
    virtual DocumentFormatId format() const = 0;
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
