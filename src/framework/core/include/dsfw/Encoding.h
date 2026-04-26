#pragma once

#include <dstools/Result.h>

#include <QByteArray>
#include <QString>
#include <string>

namespace dsfw {

    enum class EncodingType {
        Utf8,
        Utf8Bom,
        Utf16LE,
        Utf16BE,
        Gbk,
        Latin1,
    };

    class Encoding {
    public:
        static EncodingType detect(const QByteArray &data);

        static QString decode(const QByteArray &data, EncodingType encoding);

        static QByteArray encode(const QString &text, EncodingType encoding);

        static dstools::Result<QString> readFile(const std::string &path);

        static dstools::Result<QString> readFile(const QString &path);

        static dstools::Result<void> writeFile(const std::string &path,
                                               const QString &text,
                                               EncodingType encoding = EncodingType::Utf8);

        static dstools::Result<void> writeFile(const QString &path,
                                               const QString &text,
                                               EncodingType encoding = EncodingType::Utf8);
    };

} // namespace dsfw