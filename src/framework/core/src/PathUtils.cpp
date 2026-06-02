#include <dsfw/PathUtils.h>
#include <dsfw/AtomicFileWriter.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <QStringEncoder>

#include <array>
#include <fstream>

namespace dsfw {

namespace {

    bool isGbkFirstByte(unsigned char c) {
        return c >= 0x81 && c <= 0xFE;
    }

    bool isGbkSecondByte(unsigned char c) {
        return (c >= 0x40 && c <= 0x7E) || (c >= 0x80 && c <= 0xFE);
    }

    PathUtils::TextEncoding tryGbkHeuristic(const QByteArray &data) {
        int gbkPairs = 0;
        int nonAsciiBytes = 0;

        for (int i = 0; i < data.size(); ++i) {
            unsigned char c = data[i];
            if (c < 0x80)
                continue;
            ++nonAsciiBytes;
            if (isGbkFirstByte(c) && i + 1 < data.size() && isGbkSecondByte(data[i + 1])) {
                ++gbkPairs;
                ++i;
            }
        }

        if (nonAsciiBytes > 0) {
            double gbkRatio = static_cast<double>(gbkPairs * 2) / static_cast<double>(nonAsciiBytes);
            if (gbkRatio > 0.5)
                return PathUtils::TextEncoding::Gbk;
        }

        return PathUtils::TextEncoding::Latin1;
    }

} // namespace

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

PathUtils::TextEncoding PathUtils::detectTextEncoding(const QByteArray &data) {
    if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
        static_cast<uint8_t>(data[2]) == 0xBF) {
        return TextEncoding::Utf8Bom;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFF && static_cast<uint8_t>(data[1]) == 0xFE) {
        return TextEncoding::Utf16LE;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFE && static_cast<uint8_t>(data[1]) == 0xFF) {
        return TextEncoding::Utf16BE;
    }

    if (data.isEmpty())
        return TextEncoding::Utf8;

    for (int i = 0; i < data.size(); ++i) {
        unsigned char c = data[i];
        if (c > 0x7F) {
            if (c >= 0xC0 && c < 0xE0) {
                if (i + 1 >= data.size())
                    return tryGbkHeuristic(data);
                unsigned char c2 = data[++i];
                if ((c2 & 0xC0) != 0x80)
                    return tryGbkHeuristic(data);
            } else if (c >= 0xE0 && c < 0xF0) {
                if (i + 2 >= data.size())
                    return tryGbkHeuristic(data);
                bool valid = true;
                for (int j = 1; j <= 2; ++j) {
                    if ((static_cast<unsigned char>(data[i + j]) & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                }
                if (!valid)
                    return tryGbkHeuristic(data);
                i += 2;
            } else if (c >= 0xF0) {
                if (i + 3 >= data.size())
                    return tryGbkHeuristic(data);
                bool valid = true;
                for (int j = 1; j <= 3; ++j) {
                    if ((static_cast<unsigned char>(data[i + j]) & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                }
                if (!valid)
                    return tryGbkHeuristic(data);
                i += 3;
            } else {
                return tryGbkHeuristic(data);
            }
        }
    }
    return TextEncoding::Utf8;
}

QString PathUtils::decodeText(const QByteArray &data, TextEncoding encoding) {
    switch (encoding) {
        case TextEncoding::Utf8Bom:
            if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
                static_cast<uint8_t>(data[2]) == 0xBF) {
                return QString::fromUtf8(data.constData() + 3, data.size() - 3);
            }
            return QString::fromUtf8(data);

        case TextEncoding::Utf16LE: {
            const QByteArray stripped =
                (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFF && static_cast<uint8_t>(data[1]) == 0xFE)
                    ? QByteArray(data.constData() + 2, data.size() - 2)
                    : data;
            return QStringDecoder(QStringDecoder::Utf16LE)(stripped);
        }

        case TextEncoding::Utf16BE: {
            const QByteArray stripped =
                (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFE && static_cast<uint8_t>(data[1]) == 0xFF)
                    ? QByteArray(data.constData() + 2, data.size() - 2)
                    : data;
            return QStringDecoder(QStringDecoder::Utf16BE)(stripped);
        }

        case TextEncoding::Gbk: {
            QStringDecoder decoder("GBK");
            if (decoder.isValid())
                return decoder(data);
            return QString::fromUtf8(data);
        }

        case TextEncoding::Latin1: {
            QString result;
            result.reserve(data.size());
            for (auto b : data)
                result.append(QChar(static_cast<unsigned char>(b)));
            return result;
        }

        case TextEncoding::Utf8:
        default:
            return QString::fromUtf8(data);
    }
}

QByteArray PathUtils::encodeText(const QString &text, TextEncoding encoding) {
    switch (encoding) {
        case TextEncoding::Utf8Bom:
            return QByteArray("\xEF\xBB\xBF") + text.toUtf8();

        case TextEncoding::Utf16LE: {
            QByteArray result;
            result.append('\xFF');
            result.append('\xFE');
            result.append(QStringEncoder(QStringEncoder::Utf16LE)(text));
            return result;
        }

        case TextEncoding::Utf16BE: {
            QByteArray result;
            result.append('\xFE');
            result.append('\xFF');
            result.append(QStringEncoder(QStringEncoder::Utf16BE)(text));
            return result;
        }

        case TextEncoding::Gbk: {
            QStringEncoder encoder("GBK");
            if (encoder.isValid())
                return encoder(text);
            return text.toUtf8();
        }

        case TextEncoding::Latin1: {
            QByteArray result;
            result.reserve(text.size());
            for (const auto &ch : text)
                result.append(static_cast<char>(ch.unicode() < 256 ? ch.unicode() : '?'));
            return result;
        }

        case TextEncoding::Utf8:
        default:
            return text.toUtf8();
    }
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

dstools::Result<QString> PathUtils::readFile(const QString &path) {
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

dstools::Result<QString> PathUtils::readFile(const std::string &path) {
    return readFile(QString::fromStdString(path));
}

dstools::Result<void> PathUtils::writeFile(const QString &path, const QString &text, TextEncoding encoding) {
    const QByteArray encoded = encodeText(text, encoding);
    const std::string content(encoded.constData(), encoded.size());
    return AtomicFileWriter::write(toStdPath(path), content);
}

dstools::Result<void> PathUtils::writeFile(const std::string &path, const QString &text, TextEncoding encoding) {
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

    uint32_t crc32Update(uint32_t crc, const uint8_t *data, size_t size) {
        crc ^= 0xFFFFFFFF;
        for (size_t i = 0; i < size; ++i) {
            crc = kCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }

} // anonymous namespace

uint32_t PathUtils::crc32(const uint8_t *data, size_t size) {
    return crc32Update(0, data, size);
}

dstools::Result<uint32_t> PathUtils::crc32(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return dstools::Result<uint32_t>::Error("Failed to open file for CRC32: " + path.string());
    }

    constexpr size_t kBufSize = 65536;
    std::array<char, kBufSize> buf{};
    uint32_t crc = 0;
    while (file.read(buf.data(), kBufSize) || file.gcount() > 0) {
        crc = crc32Update(crc, reinterpret_cast<const uint8_t *>(buf.data()), file.gcount());
    }

    if (file.bad()) {
        return dstools::Result<uint32_t>::Error("Read error during CRC32: " + path.string());
    }

    return dstools::Result<uint32_t>::Ok(crc);
}

} // namespace dsfw
