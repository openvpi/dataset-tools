#include <dsfw/PathUtils.h>
#include <dstools/PathEncoding.h>

#include <QDir>
#include <QFileInfo>

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
    return path.toLocal8Bit().toStdString();
#else
    return path.toStdString();
#endif
}

QString PathUtils::fromStdPath(const std::filesystem::path &path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

std::string PathUtils::toUtf8(const std::filesystem::path &path) {
    return dstools::pathToUtf8(path);
}

std::wstring PathUtils::toWide(const std::filesystem::path &path) {
    return dstools::pathToWide(path);
}

std::filesystem::path PathUtils::join(const std::filesystem::path &base,
                                       const std::filesystem::path &relative) {
    return base / relative;
}

std::filesystem::path PathUtils::join(const std::filesystem::path &base,
                                       const std::string &relative) {
    return base / relative;
}

std::filesystem::path PathUtils::join(const std::filesystem::path &base,
                                       const char *relative) {
    return base / relative;
}

QString PathUtils::normalize(const QString &path) {
    QString result = QDir::cleanPath(path);

#ifdef Q_OS_MAC
    result = result.normalized(QString::NormalizationForm_C);
#endif

    return result;
}

FILE *PathUtils::openFile(const QString &path, const char *mode) {
#ifdef Q_OS_WIN
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
