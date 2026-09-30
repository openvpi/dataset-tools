#ifndef TIFA_PATHUTILS_H
#define TIFA_PATHUTILS_H

#include <filesystem>
#include <string>

#include <QString>

namespace Tifa {

    /**
     * Convert a path shown in the interface into a filesystem path.
     *
     * QString::toStdString() is UTF-8 on every platform, which is what the
     * native narrow encoding expects on POSIX, while Windows decodes narrow
     * strings with the ANSI code page and needs the wide form.
     */
    inline std::filesystem::path toPath(const QString &path) {
#ifdef Q_OS_WINDOWS
        return std::filesystem::path(path.toStdWString());
#else
        return std::filesystem::path(path.toStdString());
#endif
    }

    /**
     * Path embedded into a message, always UTF-8.
     *
     * Message strings are decoded with QString::fromStdString(), which expects
     * UTF-8, while std::filesystem::path::string() returns the ANSI code page
     * on Windows and mangles every character outside of it.
     */
    inline std::string pathToUtf8(const std::filesystem::path &path) {
#ifdef Q_OS_WINDOWS
        return QString::fromStdWString(path.wstring()).toStdString();
#else
        return path.string();
#endif
    }

    /** Inverse of toPath; keeps non-ASCII characters readable in messages. */
    inline QString fromPath(const std::filesystem::path &path) {
#ifdef Q_OS_WINDOWS
        return QString::fromStdWString(path.wstring());
#else
        return QString::fromStdString(path.string());
#endif
    }

} // namespace Tifa

#endif // TIFA_PATHUTILS_H
