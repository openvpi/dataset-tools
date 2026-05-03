#include <dsfw/PathUtils.h>

#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_MAC
#include <QString>
#endif

namespace dsfw {

std::filesystem::path PathUtils::toStdPath(const QString &path) {
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

std::string PathUtils::toNarrowPath(const QString &path) {
#ifdef Q_OS_WIN
    // On Windows, use local 8-bit encoding for compatibility with legacy APIs.
    // Note: prefer toStdPath() + wide-char APIs where possible.
    return path.toLocal8Bit().toStdString();
#else
    return path.toStdString(); // UTF-8 on Unix
#endif
}

QString PathUtils::fromStdPath(const std::filesystem::path &path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

QString PathUtils::normalize(const QString &path) {
    QString result = QDir::cleanPath(path);

#ifdef Q_OS_MAC
    // macOS HFS+ uses NFD (decomposed Unicode). Normalize to NFC for consistent comparison.
    result = result.normalized(QString::NormalizationForm_C);
#endif

    return result;
}

FILE *PathUtils::openFile(const QString &path, const char *mode) {
#ifdef Q_OS_WIN
    // Use _wfopen for safe Unicode path handling on Windows
    std::wstring wPath = path.toStdWString();
    std::wstring wMode;
    while (*mode) {
        wMode += static_cast<wchar_t>(*mode);
        ++mode;
    }
    return _wfopen(wPath.c_str(), wMode.c_str());
#else
    return fopen(path.toUtf8().constData(), mode);
#endif
}

bool PathUtils::hasNonAscii(const QString &path) {
    for (const QChar &ch : path) {
        if (ch.unicode() > 127)
            return true;
    }
    return false;
}

QString PathUtils::toNativeSeparators(const QString &path) {
    return QDir::toNativeSeparators(path);
}

QString PathUtils::toPosixSeparators(const QString &path) {
    QString result = path;
    result.replace('\\', '/');
    return result;
}

} // namespace dsfw
