#pragma once

#include <dsfw/Result.h>

#include <QString>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace dsfw {

class PathUtils {
public:
    enum class Encoding : std::uint8_t { Utf8, Ansi, Unknown };

    enum class TextEncoding : std::uint8_t { Utf8, Utf8Bom, Utf16LE, Utf16BE, Gbk, Latin1 };

    static std::filesystem::path toStdPath(const QString& path);

    static std::string toNarrowPath(const QString& path);

    static QString fromStdPath(const std::filesystem::path& path);

    static std::string toUtf8(const std::filesystem::path& path);

    static std::wstring toWide(const std::filesystem::path& path);

    static std::filesystem::path join(const std::filesystem::path& base, const std::filesystem::path& relative);

    static std::filesystem::path join(const std::filesystem::path& base, const std::string& relative);

    static std::filesystem::path join(const std::filesystem::path& base, const char* relative);

    static QString normalize(const QString& path);

    static FILE* openFile(const QString& path, const char* mode);

    static std::ifstream openIfstream(const std::filesystem::path& path,
                                      std::ios::openmode mode = std::ios::in);

    static std::ofstream openOfstream(const std::filesystem::path& path,
                                      std::ios::openmode mode = std::ios::out);

    static bool hasNonAscii(const QString& path);

    static QString toNativeSeparators(const QString& path);

    static QString toPosixSeparators(const QString& path);

    static Encoding detectEncoding(const std::filesystem::path& path);

    static std::optional<std::filesystem::path> canonicalOrNull(const std::filesystem::path& path) noexcept;

    static bool isSubPath(const std::filesystem::path& parent, const std::filesystem::path& child) noexcept;

    static bool isPathWithinSandbox(const std::filesystem::path& path, const std::filesystem::path& root) noexcept;

    static dsfw::Result<std::filesystem::path> sanitizePath(const std::filesystem::path& userPath,
                                                              const std::filesystem::path& root);

    static dsfw::Result<std::filesystem::path> validatePath(const std::filesystem::path& path,
                                                              const std::filesystem::path& root,
                                                              bool allowAbsolute = false);

    static std::filesystem::path relativeTo(const std::filesystem::path& path, const std::filesystem::path& base);

    static TextEncoding detectTextEncoding(const QByteArray& data);

    static QString decodeText(const QByteArray& data, TextEncoding encoding);

    static QByteArray encodeText(const QString& text, TextEncoding encoding);

    static dsfw::Result<QString> readFile(const QString& path);

    static dsfw::Result<QString> readFile(const std::string& path);

    static dsfw::Result<void> writeFile(const QString& path, const QString& text,
                                           TextEncoding encoding = TextEncoding::Utf8);

    static dsfw::Result<void> writeFile(const std::string& path, const QString& text,
                                           TextEncoding encoding = TextEncoding::Utf8);

    static dsfw::Result<uint32_t> crc32(const std::filesystem::path& path);

    static uint32_t crc32(const uint8_t* data, size_t size);
};

} // namespace dsfw
