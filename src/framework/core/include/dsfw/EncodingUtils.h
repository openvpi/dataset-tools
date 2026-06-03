#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>

namespace dsfw {

/// @brief Text encoding detection, decoding, and encoding utilities.
///
/// Provides BOM detection, UTF-8 validation, GBK heuristic detection,
/// and conversion between QByteArray and QString.
class EncodingUtils {
public:
    enum class Encoding : std::uint8_t { Utf8, Utf8Bom, Utf16LE, Utf16BE, Gbk, Latin1 };

    static constexpr int kInterfaceVersion = 1;

    /// @brief Detect the text encoding of raw byte data.
    /// Checks BOM first, then validates UTF-8 multibyte sequences,
    /// falling back to GBK heuristic if validation fails.
    static Encoding detect(const QByteArray &data);

    /// @brief Decode raw byte data to a QString using the specified encoding.
    static QString toUtf8(const QByteArray &data, Encoding encoding);

    /// @brief Encode a QString to raw bytes using the specified encoding.
    static QByteArray fromUtf8(const QString &text, Encoding encoding);

private:
    static bool isGbkFirstByte(unsigned char c);
    static bool isGbkSecondByte(unsigned char c);
    static Encoding tryGbkHeuristic(const QByteArray &data);
};

} // namespace dsfw