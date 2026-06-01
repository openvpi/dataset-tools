#pragma once

#include <QString>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace dsfw {

    class PathUtils {
    public:
        enum class Encoding : std::uint8_t { Utf8, Ansi, Unknown };

        static std::filesystem::path toStdPath(const QString &path);

        static std::string toNarrowPath(const QString &path);

        static QString fromStdPath(const std::filesystem::path &path);

        static std::string toUtf8(const std::filesystem::path &path);

        static std::wstring toWide(const std::filesystem::path &path);

        static std::filesystem::path join(const std::filesystem::path &base, const std::filesystem::path &relative);

        static std::filesystem::path join(const std::filesystem::path &base, const std::string &relative);

        static std::filesystem::path join(const std::filesystem::path &base, const char *relative);

        static QString normalize(const QString &path);

        static FILE *openFile(const QString &path, const char *mode);

        static bool hasNonAscii(const QString &path);

        static QString toNativeSeparators(const QString &path);

        static QString toPosixSeparators(const QString &path);

        static Encoding detectEncoding(const std::filesystem::path &path);

        static std::optional<std::filesystem::path> canonicalOrNull(const std::filesystem::path &path) noexcept;

        static bool isSubPath(const std::filesystem::path &parent, const std::filesystem::path &child) noexcept;

        static std::filesystem::path relativeTo(const std::filesystem::path &path, const std::filesystem::path &base);
    };

} // namespace dsfw
