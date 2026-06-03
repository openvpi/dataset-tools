#include <dsfw/PathUtils.h>
#include <dsfw/AtomicFileWriter.h>
#include <dsfw/EncodingUtils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <array>
#include <fstream>

namespace dsfw {

std::filesystem::path PathUtils::toStdPath(const QString& path) {
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

std::string PathUtils::toNarrowPath(const QString& path) {
#ifdef Q_OS_WIN
    return path.toLocal8Bit().toStdString();
#else
    return path.toStdString();
#endif
}

QString PathUtils::fromStdPath(const std::filesystem::path& path) {
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

std::string PathUtils::toUtf8(const std::filesystem::path& path) {
    auto u8 = path.u8string();
    return {u8.begin(), u8.end()};
}

std::wstring PathUtils::toWide(const std::filesystem::path& path) {
    return path.wstring();
}

std::filesystem::path PathUtils::join(const std::filesystem::path& base, const std::filesystem::path& relative) {
    return base / relative;
}

std::filesystem::path PathUtils::join(const std::filesystem::path& base, const std::string& relative) {
    return base / relative;
}

std::filesystem::path PathUtils::join(const std::filesystem::path& base, const char* relative) {
    return base / relative;
}

QString PathUtils::normalize(const QString& path) {
    QString result = QDir::cleanPath(path);

#ifdef Q_OS_MAC
    result = result.normalized(QString::NormalizationForm_C);
#endif

    return result;
}

FILE* PathUtils::openFile(const QString& path, const char* mode) {
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

bool PathUtils::hasNonAscii(const QString& path) {
    for (const QChar& ch : path) {
        if (ch.unicode() > 127)
            return true;
    }
    return false;
}

QString PathUtils::toNativeSeparators(const QString& path) {
    return QDir::toNativeSeparators(path);
}

QString PathUtils::toPosixSeparators(const QString& path) {
    QString result = path;
    result.replace('\\', '/');
    return result;
}

PathUtils::TextEncoding PathUtils::detectTextEncoding(const QByteArray& data) {
    return static_cast<TextEncoding>(EncodingUtils::detect(data));
}

QString PathUtils::decodeText(const QByteArray& data, TextEncoding encoding) {
    return EncodingUtils::toUtf8(data, static_cast<EncodingUtils::Encoding>(encoding));
}

QByteArray PathUtils::encodeText(const QString& text, TextEncoding encoding) {
    return EncodingUtils::fromUtf8(text, static_cast<EncodingUtils::Encoding>(encoding));
}

PathUtils::Encoding PathUtils::detectEncoding(const std::filesystem::path& path) {
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
    const auto detected = detectTextEncoding(data);

    switch (detected) {
        case TextEncoding::Utf8:
        case TextEncoding::Utf8Bom:
            return Encoding::Utf8;
        case TextEncoding::Utf16LE:
        case TextEncoding::Utf16BE:
            return Encoding::Utf8;
        case TextEncoding::Gbk:
            return Encoding::Ansi;
        case TextEncoding::Latin1:
            return Encoding::Ansi;
    }
    return Encoding::Unknown;
}

std::optional<std::filesystem::path> PathUtils::canonicalOrNull(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    auto canonical = std::filesystem::canonical(path, ec);
    if (ec)
        return std::nullopt;
    return canonical;
}

bool PathUtils::isSubPath(const std::filesystem::path& parent, const std::filesystem::path& child) noexcept {
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

std::filesystem::path PathUtils::relativeTo(const std::filesystem::path& path, const std::filesystem::path& base) {
    return std::filesystem::relative(path, base);
}

dstools::Result<QString> PathUtils::readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return dstools::Result<QString>::Error("Cannot open file: " + toUtf8(path.toStdString()));

    const QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty())
        return QString();

    const auto encoding = detectTextEncoding(data);
    return decodeText(data, encoding);
}

dstools::Result<QString> PathUtils::readFile(const std::string& path) {
    return readFile(QString::fromStdString(path));
}

dstools::Result<void> PathUtils::writeFile(const QString& path, const QString& text, TextEncoding encoding) {
    const QByteArray encoded = encodeText(text, encoding);
    const std::string content(encoded.constData(), encoded.size());
    return AtomicFileWriter::write(toStdPath(path), content);
}

dstools::Result<void> PathUtils::writeFile(const std::string& path, const QString& text, TextEncoding encoding) {
    return writeFile(QString::fromStdString(path), text, encoding);
}

namespace {

constexpr uint32_t kCrc32Polynomial = 0xEDB88320;

constexpr std::array<uint32_t, 256> makeCrc32Table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ kCrc32Polynomial;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

constexpr auto kCrc32Table = makeCrc32Table();

uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t size) {
    crc ^= 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        crc = kCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

} // anonymous namespace

uint32_t PathUtils::crc32(const uint8_t* data, size_t size) {
    return crc32Update(0, data, size);
}

dstools::Result<uint32_t> PathUtils::crc32(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return dstools::Result<uint32_t>::Error("Failed to open file for CRC32: " + toUtf8(path));
    }

    constexpr size_t kBufSize = 65536;
    std::array<char, kBufSize> buf{};
    uint32_t crc = 0;
    while (file.read(buf.data(), kBufSize) || file.gcount() > 0) {
        crc = crc32Update(crc, reinterpret_cast<const uint8_t*>(buf.data()), file.gcount());
    }

    if (file.bad()) {
        return dstools::Result<uint32_t>::Error("Read error during CRC32: " + toUtf8(path));
    }

    return dstools::Result<uint32_t>::Ok(crc);
}

} // namespace dsfw
