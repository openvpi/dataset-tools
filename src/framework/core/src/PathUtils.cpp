#include <dsfw/PathUtils.h>
#include <dsfw/Encoding.h>

#include <QDir>
#include <QFileInfo>

#include <array>
#include <fstream>

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
    auto u8 = path.u8string();
    return {u8.begin(), u8.end()};
}

std::wstring PathUtils::toWide(const std::filesystem::path &path) {
    return path.wstring();
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

PathUtils::Encoding PathUtils::detectEncoding(const std::filesystem::path &path) {
    constexpr size_t kHeaderSize = 4096;
    std::array<char, kHeaderSize> buf{};

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return Encoding::Unknown;

    file.read(buf.data(), buf.size());
    const auto bytesRead = static_cast<int>(file.gcount());
    if (bytesRead <= 0)
        return Encoding::Utf8;

    QByteArray data(buf.data(), bytesRead);
    const auto detected = Encoding::detect(data);

    switch (detected) {
        case EncodingType::Utf8:
        case EncodingType::Utf8Bom:
            return Encoding::Utf8;
        case EncodingType::Utf16LE:
        case EncodingType::Utf16BE:
            return Encoding::Utf8;
        case EncodingType::Gbk:
            return Encoding::Ansi;
        case EncodingType::Latin1:
            return Encoding::Ansi;
    }
    return Encoding::Unknown;
}

std::optional<std::filesystem::path> PathUtils::canonicalOrNull(const std::filesystem::path &path) noexcept {
    std::error_code ec;
    auto canonical = std::filesystem::canonical(path, ec);
    if (ec)
        return std::nullopt;
    return canonical;
}

bool PathUtils::isSubPath(const std::filesystem::path &parent, const std::filesystem::path &child) noexcept {
    std::error_code ec;
    const auto canonParent = std::filesystem::canonical(parent, ec);
    if (ec)
        return false;
    const auto canonChild = std::filesystem::canonical(child, ec);
    if (ec)
        return false;

    auto itParent = canonParent.begin();
    auto itChild = canonChild.begin();
    for (; itParent != canonParent.end() && itChild != canonChild.end(); ++itParent, ++itChild) {
        if (*itParent != *itChild)
            return false;
    }
    return itParent == canonParent.end();
}

std::filesystem::path PathUtils::relativeTo(const std::filesystem::path &path, const std::filesystem::path &base) {
    return std::filesystem::relative(path, base);
}

} // namespace dsfw
