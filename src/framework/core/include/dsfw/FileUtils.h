#pragma once

#include <QByteArray>
#include <dsfw/Result.h>

#include <cstdint>
#include <filesystem>

namespace dsfw {

    /// Unified file I/O utilities.
    /// All paths use std::filesystem::path for cross-platform compatibility.
    /// Atomic writes are delegated to AtomicFileWriter.
    class FileUtils {
    public:
        /// Read entire file content as raw bytes.
        static dsfw::Result<QByteArray> readAll(const std::filesystem::path &path);

        /// Write data atomically (via AtomicFileWriter).
        /// The parent directory is created automatically if it does not exist.
        static dsfw::Result<void> writeAll(const std::filesystem::path &path, const QByteArray &data);

        /// Safe file copy with Unicode path support.
        /// Overwrites destination if it already exists.
        static dsfw::Result<void> copy(const std::filesystem::path &src, const std::filesystem::path &dst);

        /// Check whether the file exists (and is a regular file).
        static bool exists(const std::filesystem::path &path);

        /// Get file size in bytes.
        /// Returns Error if the file does not exist or cannot be queried.
        static dsfw::Result<uint64_t> fileSize(const std::filesystem::path &path);
    };

} // namespace dsfw