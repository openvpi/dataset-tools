#include <QTest>
#include <QTemporaryDir>
#include <QFile>

#include <dsfw/Encoding.h>
#include <dsfw/PathUtils.h>

using namespace dsfw;

class TestEncoding : public QObject {
    Q_OBJECT
private slots:
    void detect_utf8();
    void detect_utf8Bom();
    void detect_utf16LE();
    void detect_utf16BE();
    void detect_gbk();
    void detect_latin1();
    void detect_empty();
    void detect_asciiOnly();

    void decode_utf8();
    void decode_utf8Bom();
    void decode_utf16LE();
    void decode_utf16BE();
    void decode_gbk();
    void decode_latin1();

    void encode_utf8();
    void encode_utf8Bom();
    void encode_utf16LE();
    void encode_utf16BE();

    void readFile_utf8();
    void readFile_utf8Bom();
    void readFile_utf16LE();
    void readFile_utf16BE();
    void readFile_empty();
    void readFile_nonexistent();

    void writeFile_utf8();
    void writeFile_utf8Bom();
    void writeFile_utf16LE();

    void roundTrip_utf8();
    void roundTrip_utf16LE();
    void roundTrip_multilingual();

    void detect_partialUtf8();

    void roundTrip_utf16BE();
    void roundTrip_gbk();
    void roundTrip_latin1();
    void readFile_gbk();
    void writeFile_invalidPath();
    void encodeDecode_gbk();
    void encodeDecode_latin1();
};

void TestEncoding::detect_utf8() {
    QByteArray data = "Hello World";
    QCOMPARE(Encoding::detect(data), EncodingType::Utf8);
}

void TestEncoding::detect_utf8Bom() {
    QByteArray data;
    data.append('\xEF');
    data.append('\xBB');
    data.append('\xBF');
    data.append("Hello");
    QCOMPARE(Encoding::detect(data), EncodingType::Utf8Bom);
}

void TestEncoding::detect_utf16LE() {
    QByteArray data;
    data.append('\xFF');
    data.append('\xFE');
    data.append("H\0e\0l\0l\0o\0", 10);
    QCOMPARE(Encoding::detect(data), EncodingType::Utf16LE);
}

void TestEncoding::detect_utf16BE() {
    QByteArray data;
    data.append('\xFE');
    data.append('\xFF');
    data.append("\0H\0e\0l\0l\0o", 10);
    QCOMPARE(Encoding::detect(data), EncodingType::Utf16BE);
}

void TestEncoding::detect_gbk() {
    QByteArray data;
    data.append("Hello ");
    data.append('\xCA');
    data.append('\xC0');
    data.append('\xBD');
    data.append('\xE7');
    QCOMPARE(Encoding::detect(data), EncodingType::Gbk);
}

void TestEncoding::detect_latin1() {
    QByteArray data;
    data.append("Hello ");
    data.append('\x80');
    data.append('\xFF');
    QCOMPARE(Encoding::detect(data), EncodingType::Latin1);
}

void TestEncoding::detect_empty() {
    QByteArray data;
    QCOMPARE(Encoding::detect(data), EncodingType::Utf8);
}

void TestEncoding::detect_asciiOnly() {
    QByteArray data = "abcdefghijklmnopqrstuvwxyz0123456789";
    QCOMPARE(Encoding::detect(data), EncodingType::Utf8);
}

void TestEncoding::decode_utf8() {
    QByteArray data = "Hello World";
    QString result = Encoding::decode(data, EncodingType::Utf8);
    QCOMPARE(result, QStringLiteral("Hello World"));
}

void TestEncoding::decode_utf8Bom() {
    QByteArray data;
    data.append('\xEF');
    data.append('\xBB');
    data.append('\xBF');
    data.append("Hello");
    QString result = Encoding::decode(data, EncodingType::Utf8Bom);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestEncoding::decode_utf16LE() {
    QByteArray data;
    data.append('\xFF');
    data.append('\xFE');
    data.append("H\0e\0l\0l\0o\0", 10);
    QString result = Encoding::decode(data, EncodingType::Utf16LE);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestEncoding::decode_utf16BE() {
    QByteArray data;
    data.append('\xFE');
    data.append('\xFF');
    data.append("\0H\0e\0l\0l\0o", 10);
    QString result = Encoding::decode(data, EncodingType::Utf16BE);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestEncoding::decode_gbk() {
    QByteArray data;
    data.append("Hello ");
    data.append('\xCA');
    data.append('\xC0');
    data.append('\xBD');
    data.append('\xE7');
    QString result = Encoding::decode(data, EncodingType::Gbk);
    QVERIFY(result.startsWith(QStringLiteral("Hello ")));
    QCOMPARE(result, QString::fromLocal8Bit(data));
}

void TestEncoding::decode_latin1() {
    QByteArray data;
    data.append("Hello");
    data.append('\xE9');
    QString result = Encoding::decode(data, EncodingType::Latin1);
    QVERIFY(result.startsWith(QStringLiteral("Hello")));
    QCOMPARE(result.size(), 6);
}

void TestEncoding::encode_utf8() {
    QString text = QStringLiteral("Hello World");
    QByteArray result = Encoding::encode(text, EncodingType::Utf8);
    QCOMPARE(result, QByteArray("Hello World"));
}

void TestEncoding::encode_utf8Bom() {
    QString text = QStringLiteral("Hello");
    QByteArray result = Encoding::encode(text, EncodingType::Utf8Bom);
    QVERIFY(result.size() > 3);
    QCOMPARE(static_cast<uint8_t>(result[0]), uint8_t(0xEF));
    QCOMPARE(static_cast<uint8_t>(result[1]), uint8_t(0xBB));
    QCOMPARE(static_cast<uint8_t>(result[2]), uint8_t(0xBF));
    QCOMPARE(result.mid(3), QByteArray("Hello"));
}

void TestEncoding::encode_utf16LE() {
    QString text = QStringLiteral("Hello");
    QByteArray result = Encoding::encode(text, EncodingType::Utf16LE);
    QVERIFY(result.size() >= 2);
    QCOMPARE(static_cast<uint8_t>(result[0]), uint8_t(0xFF));
    QCOMPARE(static_cast<uint8_t>(result[1]), uint8_t(0xFE));
}

void TestEncoding::encode_utf16BE() {
    QString text = QStringLiteral("Hello");
    QByteArray result = Encoding::encode(text, EncodingType::Utf16BE);
    QVERIFY(result.size() >= 2);
    QCOMPARE(static_cast<uint8_t>(result[0]), uint8_t(0xFE));
    QCOMPARE(static_cast<uint8_t>(result[1]), uint8_t(0xFF));
}

void TestEncoding::readFile_utf8() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("utf8.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("Hello UTF-8");
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello UTF-8"));
}

void TestEncoding::readFile_utf8Bom() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("utf8bom.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("\xEF\xBB\xBF");
        f.write("Hello BOM");
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello BOM"));
}

void TestEncoding::readFile_utf16LE() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("utf16le.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("\xFF\xFE");
        f.write("H\0e\0l\0l\0o\0", 10);
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello"));
}

void TestEncoding::readFile_utf16BE() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("utf16be.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("\xFE\xFF");
        f.write("\0H\0e\0l\0l\0o", 10);
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello"));
}

void TestEncoding::readFile_empty() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("empty.txt");

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QVERIFY(result.value().isEmpty());
}

void TestEncoding::readFile_nonexistent() {
    auto result = Encoding::readFile("/nonexistent/path/file.txt");
    QVERIFY(!result.ok());
}

void TestEncoding::writeFile_utf8() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("write_utf8.txt");

    auto result = Encoding::writeFile(path, QStringLiteral("Hello UTF-8"), EncodingType::Utf8);
    QVERIFY(result.ok());

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("Hello UTF-8"));
}

void TestEncoding::writeFile_utf8Bom() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("write_bom.txt");

    auto result = Encoding::writeFile(path, QStringLiteral("Hello"), EncodingType::Utf8Bom);
    QVERIFY(result.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), QStringLiteral("Hello"));
}

void TestEncoding::writeFile_utf16LE() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("write_utf16le.txt");

    auto writeResult = Encoding::writeFile(path, QStringLiteral("Hello"), EncodingType::Utf16LE);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), QStringLiteral("Hello"));
}

void TestEncoding::roundTrip_utf8() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("rt_utf8.txt");

    QString original = QStringLiteral("Hello World! 12345");
    auto writeResult = Encoding::writeFile(path, original, EncodingType::Utf8);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), original);
}

void TestEncoding::roundTrip_utf16LE() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("rt_utf16le.txt");

    QString original = QStringLiteral("Hello UTF-16! テスト 🎵");
    auto writeResult = Encoding::writeFile(path, original, EncodingType::Utf16LE);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), original);
}

void TestEncoding::roundTrip_multilingual() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString sample = QString::fromUtf8("Hello 世界\n日本語テスト\n한국어\n🎵🎶");

    for (auto enc : {EncodingType::Utf8, EncodingType::Utf8Bom, EncodingType::Utf16LE, EncodingType::Utf16BE}) {
        QString path = tmp.filePath(QStringLiteral("rt_multi_%1.txt").arg(static_cast<int>(enc)));

        auto writeResult = Encoding::writeFile(path, sample, enc);
        QVERIFY2(writeResult.ok(), qPrintable(QStringLiteral("write failed for enc=%1").arg(static_cast<int>(enc))));

        auto readResult = Encoding::readFile(path);
        QVERIFY2(readResult.ok(), qPrintable(QStringLiteral("read failed for enc=%1").arg(static_cast<int>(enc))));
        QCOMPARE(readResult.value(), sample);
    }
}

void TestEncoding::detect_partialUtf8() {
    QByteArray data;
    data.append('\xC0');
    data.append('a');
    QCOMPARE(Encoding::detect(data), EncodingType::Latin1);
}

void TestEncoding::roundTrip_utf16BE() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("rt_utf16be.txt");

    QString original = QStringLiteral("Hello UTF-16BE! テスト");
    auto writeResult = Encoding::writeFile(path, original, EncodingType::Utf16BE);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), original);
}

void TestEncoding::roundTrip_gbk() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("rt_gbk.txt");

    QString original = QString::fromLocal8Bit("Hello 世界！中文测试");
    auto writeResult = Encoding::writeFile(path, original, EncodingType::Gbk);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), original);
}

void TestEncoding::roundTrip_latin1() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("rt_latin1.txt");

    QString original;
    for (int i = 0; i < 256; ++i)
        original.append(QChar(i));
    auto writeResult = Encoding::writeFile(path, original, EncodingType::Latin1);
    QVERIFY(writeResult.ok());

    auto readResult = Encoding::readFile(path);
    QVERIFY(readResult.ok());
    QCOMPARE(readResult.value(), original);
}

void TestEncoding::readFile_gbk() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.filePath("read_gbk.txt");

    QString text = QString::fromLocal8Bit("测试GBK读取");
    QByteArray encoded = Encoding::encode(text, EncodingType::Gbk);
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(encoded);
    }

    auto result = Encoding::readFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), text);
}

void TestEncoding::writeFile_invalidPath() {
    auto result = Encoding::writeFile("/invalid/dir/../nonexistent/file.txt", QStringLiteral("data"), EncodingType::Utf8);
    QVERIFY(!result.ok());
}

void TestEncoding::encodeDecode_gbk() {
    QString original = QString::fromLocal8Bit("你好世界");
    QByteArray encoded = Encoding::encode(original, EncodingType::Gbk);
    QVERIFY(!encoded.isEmpty());

    QString decoded = Encoding::decode(encoded, EncodingType::Gbk);
    QCOMPARE(decoded, original);
}

void TestEncoding::encodeDecode_latin1() {
    QString original;
    for (int i = 32; i < 256; ++i)
        original.append(QChar(i));
    QByteArray encoded = Encoding::encode(original, EncodingType::Latin1);
    QCOMPARE(encoded.size(), original.size());

    QString decoded = Encoding::decode(encoded, EncodingType::Latin1);
    QCOMPARE(decoded, original);
}

QTEST_MAIN(TestEncoding)
#include "TestEncoding.moc"