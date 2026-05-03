#pragma once
/// @file PathUtils.h
/// @brief Cross-platform path utilities handling encoding, normalization, and CJK paths.
///
/// Solves the common issue of Chinese/Japanese/Korean characters in file paths
/// on different platforms (Windows ANSI vs UTF-8, macOS NFD normalization, etc.).
/// All path I/O in the application should go through these utilities.

#include <QString>

#include <filesystem>
#include <string>

namespace dsfw {

/// @brief Cross-platform path utilities for safe handling of non-ASCII paths.
///
/// Key problems solved:
/// - Windows: legacy ANSI APIs (fopen, std::ifstream) cannot handle Chinese paths.
///   Must use wide-char (wchar_t) or Qt's QFile.
/// - macOS: HFS+ uses NFD (decomposed) Unicode for filenames. Qt handles this
///   internally, but interop with C libraries (sndfile, ffmpeg) may fail.
/// - Linux: Filenames are byte sequences (usually UTF-8). No special handling needed.
///
/// Usage:
/// @code
///   // Convert QString to std::filesystem::path (safe on all platforms)
///   auto fsPath = PathUtils::toStdPath(qStringPath);
///
///   // Convert for passing to C libraries that expect narrow strings
///   auto narrow = PathUtils::toNarrowPath(qStringPath); // UTF-8 on Unix, local 8-bit on Windows
///
///   // Open file safely (wraps platform differences)
///   auto file = PathUtils::openFile(qStringPath, "rb");
/// @endcode
class PathUtils {
public:
    /// Convert a QString path to std::filesystem::path.
    /// Uses wstring on Windows (safe for CJK), u8string on Unix.
    static std::filesystem::path toStdPath(const QString &path);

    /// Convert a QString path to a narrow-character string suitable for C APIs.
    /// On Windows: converts to local ANSI codepage (for legacy APIs) or uses
    /// the UTF-8 manifest if available. Prefer toStdPath() + wchar APIs where possible.
    /// On Unix: returns UTF-8.
    static std::string toNarrowPath(const QString &path);

    /// Convert a std::filesystem::path back to QString.
    static QString fromStdPath(const std::filesystem::path &path);

    /// Normalize a path for consistent comparison.
    /// Resolves `..`, removes trailing slashes, normalizes separators.
    /// On macOS, applies NFC normalization for Unicode.
    static QString normalize(const QString &path);

    /// Open a FILE* handle safely for any Unicode path.
    /// Uses _wfopen on Windows, fopen with UTF-8 on Unix.
    /// Caller must fclose() the returned handle.
    /// Returns nullptr on failure.
    static FILE *openFile(const QString &path, const char *mode);

    /// Check if a path contains non-ASCII characters that might cause
    /// problems with legacy C APIs on Windows.
    static bool hasNonAscii(const QString &path);

    /// Convert path separators to native format.
    static QString toNativeSeparators(const QString &path);

    /// Convert path separators to POSIX format (forward slashes).
    static QString toPosixSeparators(const QString &path);
};

} // namespace dsfw
