#pragma once

/// @file IFileIOProvider.h
/// @brief Abstract file I/O interface for filesystem operations.
///
/// IFileIOProvider defines a pure virtual interface for file I/O, enabling
/// dependency injection and testability. The default implementation uses the
/// local filesystem via Qt's QFile/QDir classes.

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

#include <string>
#include <vector>

namespace dstools {

/// Metadata about a filesystem entry.
struct FileInfo {
    QString path;         ///< Absolute or canonical path.
    qint64 size = 0;      ///< File size in bytes.
    QDateTime lastModified; ///< Last modification timestamp.
    bool exists = false;  ///< Whether the entry exists.
    bool isDirectory = false; ///< Whether the entry is a directory.
};

/// Pure virtual interface for file I/O operations.
///
/// All methods report errors via @p error (set to a non-empty string on
/// failure, cleared on success).
class IFileIOProvider {
public:
    virtual ~IFileIOProvider() = default;

    // ── Binary I/O ───────────────────────────────────────────────────

    /// Read entire file contents as raw bytes.
    virtual QByteArray readFile(const QString &path, std::string &error) = 0;

    /// Write raw bytes to a file, creating parent directories as needed.
    virtual bool writeFile(const QString &path, const QByteArray &data, std::string &error) = 0;

    // ── Text I/O ─────────────────────────────────────────────────────

    /// Read entire file contents as UTF-8 text.
    virtual QString readText(const QString &path, std::string &error) = 0;

    /// Write UTF-8 text to a file, creating parent directories as needed.
    virtual bool writeText(const QString &path, const QString &text, std::string &error) = 0;

    // ── Queries ──────────────────────────────────────────────────────

    /// Check whether a file or directory exists.
    virtual bool exists(const QString &path) = 0;

    /// Retrieve metadata for a filesystem entry.
    virtual FileInfo fileInfo(const QString &path) = 0;

    /// List files matching @p nameFilters under @p directory.
    /// @param recursive  If true, recurse into subdirectories.
    virtual QStringList listFiles(const QString &directory, const QStringList &nameFilters,
                                  bool recursive, std::string &error) = 0;

    // ── Mutations ────────────────────────────────────────────────────

    /// Create directories (including intermediate parents).
    virtual bool mkdirs(const QString &path, std::string &error) = 0;

    /// Remove a single file.
    virtual bool removeFile(const QString &path, std::string &error) = 0;

    /// Copy a file from @p src to @p dst, overwriting if the destination exists.
    virtual bool copyFile(const QString &src, const QString &dst, std::string &error) = 0;
};

} // namespace dstools
