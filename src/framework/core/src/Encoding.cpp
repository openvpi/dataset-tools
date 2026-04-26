#include <dsfw/Encoding.h>
#include <dsfw/PathUtils.h>

#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QTextStream>

namespace dsfw {

namespace {
bool isGbkFirstByte(unsigned char c) {
    return c >= 0x81 && c <= 0xFE;
}

bool isGbkSecondByte(unsigned char c) {
    return (c >= 0x40 && c <= 0x7E) || (c >= 0x80 && c <= 0xFE);
}

EncodingType tryGbkHeuristic(const QByteArray& data) {
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
            return EncodingType::Gbk;
    }

    return EncodingType::Latin1;
}
} // namespace

EncodingType Encoding::detect(const QByteArray& data) {
    if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
        static_cast<uint8_t>(data[2]) == 0xBF) {
        return EncodingType::Utf8Bom;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFF && static_cast<uint8_t>(data[1]) == 0xFE) {
        return EncodingType::Utf16LE;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFE && static_cast<uint8_t>(data[1]) == 0xFF) {
        return EncodingType::Utf16BE;
    }

    if (data.isEmpty())
        return EncodingType::Utf8;

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
    return EncodingType::Utf8;
}

QString Encoding::decode(const QByteArray& data, EncodingType encoding) {
    switch (encoding) {
        case EncodingType::Utf8Bom:
            if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
                static_cast<uint8_t>(data[2]) == 0xBF) {
                return QString::fromUtf8(data.constData() + 3, data.size() - 3);
            }
            return QString::fromUtf8(data);

        case EncodingType::Utf16LE: {
            const QByteArray stripped =
                (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFF && static_cast<uint8_t>(data[1]) == 0xFE)
                    ? QByteArray(data.constData() + 2, data.size() - 2)
                    : data;
            return QStringDecoder(QStringDecoder::Utf16LE)(stripped);
        }

        case EncodingType::Utf16BE: {
            const QByteArray stripped =
                (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFE && static_cast<uint8_t>(data[1]) == 0xFF)
                    ? QByteArray(data.constData() + 2, data.size() - 2)
                    : data;
            return QStringDecoder(QStringDecoder::Utf16BE)(stripped);
        }

        case EncodingType::Gbk: {
            QStringDecoder decoder("GBK");
            if (decoder.isValid())
                return decoder(data);
            return QString::fromUtf8(data);
        }

        case EncodingType::Latin1: {
            QString result;
            result.reserve(data.size());
            for (auto b : data)
                result.append(QChar(static_cast<unsigned char>(b)));
            return result;
        }

        case EncodingType::Utf8:
        default:
            return QString::fromUtf8(data);
    }
}

QByteArray Encoding::encode(const QString& text, EncodingType encoding) {
    switch (encoding) {
        case EncodingType::Utf8Bom:
            return QByteArray("\xEF\xBB\xBF") + text.toUtf8();

        case EncodingType::Utf16LE: {
            QByteArray result;
            result.append('\xFF');
            result.append('\xFE');
            result.append(QStringEncoder(QStringEncoder::Utf16LE)(text));
            return result;
        }

        case EncodingType::Utf16BE: {
            QByteArray result;
            result.append('\xFE');
            result.append('\xFF');
            result.append(QStringEncoder(QStringEncoder::Utf16BE)(text));
            return result;
        }

        case EncodingType::Gbk: {
            QStringEncoder encoder("GBK");
            if (encoder.isValid())
                return encoder(text);
            return text.toUtf8();
        }

        case EncodingType::Latin1: {
            QByteArray result;
            result.reserve(text.size());
            for (const auto& ch : text)
                result.append(static_cast<char>(ch.unicode() < 256 ? ch.unicode() : '?'));
            return result;
        }

        case EncodingType::Utf8:
        default:
            return text.toUtf8();
    }
}

dstools::Result<QString> Encoding::readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return dstools::Result<QString>::Error("Cannot open file: " + dsfw::PathUtils::toUtf8(path.toStdString()));

    const QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty())
        return QString();

    const auto encoding = detect(data);
    return decode(data, encoding);
}

dstools::Result<QString> Encoding::readFile(const std::string& path) {
    return readFile(QString::fromStdString(path));
}

dstools::Result<void> Encoding::writeFile(const QString& path, const QString& text, EncodingType encoding) {
    const QByteArray encoded = encode(text, encoding);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return dstools::Result<void>::Error("Cannot open file for writing: " + dsfw::PathUtils::toNarrowPath(path));

    const qint64 written = file.write(encoded);
    file.close();

    if (written != encoded.size())
        return dstools::Result<void>::Error("Failed to write file: " + dsfw::PathUtils::toNarrowPath(path));

    return dstools::Result<void>::Ok();
}

dstools::Result<void> Encoding::writeFile(const std::string& path, const QString& text, EncodingType encoding) {
    return writeFile(QString::fromStdString(path), text, encoding);
}

} // namespace dsfw