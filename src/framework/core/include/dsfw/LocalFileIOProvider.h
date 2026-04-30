#pragma once

/// @file LocalFileIOProvider.h
/// @brief Local filesystem implementation of IFileIOProvider.

#include <dsfw/IFileIOProvider.h>

namespace dstools {

/// @brief IFileIOProvider implementation backed by the local filesystem.
class LocalFileIOProvider final : public IFileIOProvider {
public:
    /// @brief Read a file as raw bytes from the local filesystem.
    /// @param path Absolute file path.
    /// @return File contents or error.
    Result<QByteArray> readFile(const QString &path) override;
    /// @brief Write raw bytes to a local file.
    /// @param path Absolute file path.
    /// @param data Bytes to write.
    /// @return Success or error.
    Result<void> writeFile(const QString &path, const QByteArray &data) override;

    /// @brief Read a local file as UTF-8 text.
    /// @param path Absolute file path.
    /// @return File text or error.
    Result<QString> readText(const QString &path) override;
    /// @brief Write UTF-8 text to a local file.
    /// @param path Absolute file path.
    /// @param text Text to write.
    /// @return Success or error.
    Result<void> writeText(const QString &path, const QString &text) override;

    /// @brief Check whether a local path exists.
    /// @param path Absolute path.
    /// @return True if the path exists.
    bool exists(const QString &path) override;
    /// @brief Query metadata about a local file or directory.
    /// @param path Absolute path.
    /// @return FileInfo struct.
    FileInfo fileInfo(const QString &path) override;
    /// @brief List files in a local directory.
    /// @param directory Directory to list.
    /// @param nameFilters Glob patterns (e.g. "*.wav").
    /// @param recursive True to recurse into subdirectories.
    /// @return List of matching file paths or error.
    Result<QStringList> listFiles(const QString &directory, const QStringList &nameFilters,
                                  bool recursive) override;

    /// @brief Create local directories recursively.
    /// @param path Directory path to create.
    /// @return Success or error.
    Result<void> mkdirs(const QString &path) override;
    /// @brief Delete a local file.
    /// @param path Absolute file path.
    /// @return Success or error.
    Result<void> removeFile(const QString &path) override;
    /// @brief Copy a local file.
    /// @param src Source file path.
    /// @param dst Destination file path.
    /// @return Success or error.
    Result<void> copyFile(const QString &src, const QString &dst) override;
};

/// @brief Return the global file I/O provider instance (defaults to LocalFileIOProvider).
/// @return Pointer to the active IFileIOProvider.
IFileIOProvider *fileIOProvider();
/// @brief Set the global file I/O provider instance.
/// @param provider Pointer to the new provider (caller retains ownership).
void setFileIOProvider(IFileIOProvider *provider);
/// @brief Reset the global file I/O provider to the default LocalFileIOProvider.
void resetFileIOProvider();

}
