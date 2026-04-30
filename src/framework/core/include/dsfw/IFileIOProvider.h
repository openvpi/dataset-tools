#pragma once

/// @file IFileIOProvider.h
/// @brief Abstract file I/O provider for reading, writing, and querying files.

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

#include <dstools/Result.h>

#include <vector>

namespace dstools {

/// @brief Metadata about a single file or directory.
struct FileInfo {
    QString path;            ///< Absolute path.
    qint64 size = 0;         ///< File size in bytes.
    QDateTime lastModified;  ///< Last modification timestamp.
    bool exists = false;     ///< True if the path exists on disk.
    bool isDirectory = false; ///< True if the path is a directory.
};

/// @brief Abstract interface for filesystem operations.
///
/// Implementations may target the local filesystem, in-memory stores, or remote backends.
class IFileIOProvider {
public:
    virtual ~IFileIOProvider() = default;

    /// @brief Read a file as raw bytes.
    /// @param path Absolute file path.
    /// @return File contents or error.
    virtual Result<QByteArray> readFile(const QString &path) = 0;
    /// @brief Write raw bytes to a file.
    /// @param path Absolute file path.
    /// @param data Bytes to write.
    /// @return Success or error.
    virtual Result<void> writeFile(const QString &path, const QByteArray &data) = 0;

    /// @brief Read a file as UTF-8 text.
    /// @param path Absolute file path.
    /// @return File text or error.
    virtual Result<QString> readText(const QString &path) = 0;
    /// @brief Write UTF-8 text to a file.
    /// @param path Absolute file path.
    /// @param text Text to write.
    /// @return Success or error.
    virtual Result<void> writeText(const QString &path, const QString &text) = 0;

    /// @brief Check whether a path exists.
    /// @param path Absolute path.
    /// @return True if the path exists.
    virtual bool exists(const QString &path) = 0;
    /// @brief Query metadata about a file or directory.
    /// @param path Absolute path.
    /// @return FileInfo struct.
    virtual FileInfo fileInfo(const QString &path) = 0;
    /// @brief List files in a directory, optionally filtered and recursive.
    /// @param directory Directory to list.
    /// @param nameFilters Glob patterns (e.g. "*.wav").
    /// @param recursive True to recurse into subdirectories.
    /// @return List of matching file paths or error.
    virtual Result<QStringList> listFiles(const QString &directory, const QStringList &nameFilters,
                                          bool recursive) = 0;

    /// @brief Create directories recursively (like mkdir -p).
    /// @param path Directory path to create.
    /// @return Success or error.
    virtual Result<void> mkdirs(const QString &path) = 0;
    /// @brief Delete a file.
    /// @param path Absolute file path.
    /// @return Success or error.
    virtual Result<void> removeFile(const QString &path) = 0;
    /// @brief Copy a file from src to dst.
    /// @param src Source file path.
    /// @param dst Destination file path.
    /// @return Success or error.
    virtual Result<void> copyFile(const QString &src, const QString &dst) = 0;
};

}
