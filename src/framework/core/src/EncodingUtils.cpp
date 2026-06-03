#include <dsfw/EncodingUtils.h>

#include <QStringDecoder>
#include <QStringEncoder>

namespace dsfw {

bool EncodingUtils::isGbkFirstByte(unsigned char c) {
    return c >= 0x81 && c <= 0xFE;
}

bool EncodingUtils::isGbkSecondByte(unsigned char c) {
    return (c >= 0x40 && c <= 0x7E) || (c >= 0x80 && c <= 0xFE);
}

EncodingUtils::Encoding EncodingUtils::tryGbkHeuristic(const QByteArray &data) {
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
            return Encoding::Gbk;
    }

    return Encoding::Latin1;
}

EncodingUtils::Encoding EncodingUtils::detect(const QByteArray &data) {
    // BOM detection
    if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
        static_cast<uint8_t>(data[2]) == 0xBF) {
        return Encoding::Utf8Bom;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFF && static_cast<uint8_t>(data[1]) == 0xFE) {
        return Encoding::Utf16LE;
    }
    if (data.size() >= 2 && static_cast<uint8_t>(data[0]) == 0xFE && static_cast<uint8_t>(data[1]) == 0xFF) {
        return Encoding::Utf16BE;
    }

    if (data.isEmpty())
        return Encoding::Utf8;

    // UTF-8 validation
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
    return Encoding::Utf8;
}

QString EncodingUtils::toUtf8(const QByteArray &data, Encoding encoding) {
    switch (encoding) {
        case Encoding::Utf8Bom:
            if (data.size() >= 3 && static_cast<uint8_t>(data[0]) == 0xEF && static_cast<uint8_t>(data[1]) == 0xBB &&
                static_cast<uint8_t>(data[2]) == 0xBF) {
                return QString::fromUtf8(data.constData() + 3, data.size() - 3);
            }
            return QString::fromUtf8(data);

        case Encoding::Utf16LE: {
            auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
            return decoder(data);
        }
        case Encoding::Utf16BE: {
            auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
            return decoder(data);
        }
        case Encoding::Gbk: {
            auto decoder = QStringDecoder("GBK");
            return decoder(data);
        }
        case Encoding::Latin1: {
            auto decoder = QStringDecoder(QStringDecoder::Latin1);
            return decoder(data);
        }
        case Encoding::Utf8:
        default:
            return QString::fromUtf8(data);
    }
}

QByteArray EncodingUtils::fromUtf8(const QString &text, Encoding encoding) {
    switch (encoding) {
        case Encoding::Utf8Bom: {
            QByteArray bom;
            bom.append(static_cast<char>(0xEF));
            bom.append(static_cast<char>(0xBB));
            bom.append(static_cast<char>(0xBF));
            return bom + text.toUtf8();
        }
        case Encoding::Utf16LE: {
            auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
            return encoder(text);
        }
        case Encoding::Utf16BE: {
            auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
            return encoder(text);
        }
        case Encoding::Gbk: {
            auto encoder = QStringEncoder("GBK");
            return encoder(text);
        }
        case Encoding::Latin1: {
            auto encoder = QStringEncoder(QStringEncoder::Latin1);
            return encoder(text);
        }
        case Encoding::Utf8:
        default:
            return text.toUtf8();
    }
}

} // namespace dsfw