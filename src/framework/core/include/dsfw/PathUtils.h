#pragma once

#include <QString>

#include <filesystem>
#include <string>

namespace dsfw {

class PathUtils {
public:
    static std::filesystem::path toStdPath(const QString &path);

    static std::string toNarrowPath(const QString &path);

    static QString fromStdPath(const std::filesystem::path &path);

    static std::string toUtf8(const std::filesystem::path &path);

    static std::wstring toWide(const std::filesystem::path &path);

    static std::filesystem::path join(const std::filesystem::path &base,
                                      const std::filesystem::path &relative);

    static std::filesystem::path join(const std::filesystem::path &base,
                                      const std::string &relative);

    static std::filesystem::path join(const std::filesystem::path &base,
                                      const char *relative);

    static QString normalize(const QString &path);

    static FILE *openFile(const QString &path, const char *mode);

    static bool hasNonAscii(const QString &path);

    static QString toNativeSeparators(const QString &path);

    static QString toPosixSeparators(const QString &path);
};

} // namespace dsfw
