#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include <dsfw/PathUtils.h>


class TestPathUtils : public QObject {
    Q_OBJECT
private slots:
    void normalize_slashes();
    void normalize_multipleSlashes();
    void toNativeSeparators();
    void toPosixSeparators();
    void join_simple();
    void join_multiple();
    void relativeTo_simple();
    void relativeTo_differentRoot();
    void isSubPath_true();
    void isSubPath_false();
    void isSubPath_self();
    void canonicalOrNull_valid();
    void canonicalOrNull_invalid();
    void toUtf8();
    void toWide();
    void toStdPath();
    void fromStdPath();
    void hasNonAscii_true();
    void hasNonAscii_false();
    void crc32_sameFile();
    void crc32_differentFiles();
    void crc32_emptyFile();
    void crc32_nonexistentFile();
    void crc32_memoryData();

    // ── Text encoding（merged from TestEncoding） ──────────────────────
    void detect_utf8();
    void detect_utf8Bom();
    void detect_utf16LE();
    void detect_utf16BE();
    void detect_gbk();
    void detect_latin1();
    void detect_empty();
    void detect_asciiOnly();
    void detect_partialUtf8();

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
    void readFile_gbk();

    void writeFile_utf8();
    void writeFile_utf8Bom();
    void writeFile_utf16LE();
    void writeFile_invalidPath();

    void roundTrip_utf8();
    void roundTrip_utf16LE();
    void roundTrip_utf16BE();
    void roundTrip_gbk();
    void roundTrip_latin1();
    void roundTrip_multilingual();
    void encodeDecode_gbk();
    void encodeDecode_latin1();
};

// ── Path normalization ─────────────────────────────────────────────

void TestPathUtils::normalize_slashes() {
    const QString normalized = PathUtils::normalize(QStringLiteral("C:/Users/test/file.txt"));
#ifdef Q_OS_WIN
    QCOMPARE(normalized, QStringLiteral("C:\\Users\\test\\file.txt"));
#else
    QCOMPARE(normalized, QStringLiteral("C:/Users/test/file.txt"));
#endif
}

void TestPathUtils::normalize_multipleSlashes() {
    const QString normalized = PathUtils::normalize(QStringLiteral("C://Users///test//file.txt"));
    QVERIFY(!normalized.contains("//"));
    QVERIFY(!normalized.contains("\\\\"));
}

// ── Separator conversion ───────────────────────────────────────────

void TestPathUtils::toNativeSeparators() {
    const QString path = QStringLiteral("C:/Users/test/file.txt");
    const QString native = PathUtils::toNativeSeparators(path);
#ifdef Q_OS_WIN
    QCOMPARE(native, QStringLiteral("C:\\Users\\test\\file.txt"));
#else
    QCOMPARE(native, path);
#endif
}

void TestPathUtils::toPosixSeparators() {
    const QString path = QStringLiteral("C:\\Users\\test\\file.txt");
    const QString posix = PathUtils::toPosixSeparators(path);
    QCOMPARE(posix, QStringLiteral("C:/Users/test/file.txt"));
}

// ── Path join ──────────────────────────────────────────────────────

void TestPathUtils::join_simple() {
    auto result = PathUtils::join(std::filesystem::path("C:/base"), std::filesystem::path("sub/file.txt"));
    QVERIFY(result.filename() == "file.txt");
}

void TestPathUtils::join_multiple() {
    auto base = std::filesystem::path("C:/base");
    auto joined = PathUtils::join(base, std::string("a/b/c"));
    QVERIFY(joined.string().find("a") != std::string::npos);
    QVERIFY(joined.string().find("c") != std::string::npos);
}

// ── Relative path ──────────────────────────────────────────────────

void TestPathUtils::relativeTo_simple() {
    auto base = std::filesystem::path("C:/base");
    auto child = std::filesystem::path("C:/base/sub/file.txt");
    auto rel = PathUtils::relativeTo(child, base);
    QCOMPARE(QString::fromStdString(rel.string()), QStringLiteral("sub\\file.txt"));
}

void TestPathUtils::relativeTo_differentRoot() {
    auto base = std::filesystem::path("C:/base");
    auto child = std::filesystem::path("D:/other/file.txt");
    auto rel = PathUtils::relativeTo(child, base);
    QCOMPARE(QString::fromStdString(rel.string()), QString::fromStdString(child.string()));
}

// ── isSubPath ──────────────────────────────────────────────────────

void TestPathUtils::isSubPath_true() {
    auto parent = std::filesystem::path("C:/base");
    auto child = std::filesystem::path("C:/base/sub/file.txt");
    QVERIFY(PathUtils::isSubPath(parent, child));
}

void TestPathUtils::isSubPath_false() {
    auto parent = std::filesystem::path("C:/base");
    auto child = std::filesystem::path("C:/other/file.txt");
    QVERIFY(!PathUtils::isSubPath(parent, child));
}

void TestPathUtils::isSubPath_self() {
    auto path = std::filesystem::path("C:/base");
    QVERIFY(PathUtils::isSubPath(path, path));
}

// ── canonicalOrNull ───────────────────────────────────────────────

void TestPathUtils::canonicalOrNull_valid() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString filePath = dir.path() + "/test.txt";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray("test"));
    file.close();

    auto result = PathUtils::canonicalOrNull(PathUtils::toStdPath(filePath));
    QVERIFY(result.has_value());
}

void TestPathUtils::canonicalOrNull_invalid() {
    auto result = PathUtils::canonicalOrNull(std::filesystem::path("/nonexistent/path/file.txt"));
    QVERIFY(!result.has_value());
}

// ── toUtf8 / toWide / toStdPath / fromStdPath ─────────────────────

void TestPathUtils::toUtf8() {
    auto p = std::filesystem::path("C:/test/file.txt");
    auto utf8 = PathUtils::toUtf8(p);
#ifdef Q_OS_WIN
    QCOMPARE(QString::fromStdString(utf8), QStringLiteral("C:\\test\\file.txt"));
#else
    QCOMPARE(QString::fromStdString(utf8), QStringLiteral("C:/test/file.txt"));
#endif
}

void TestPathUtils::toWide() {
    auto p = std::filesystem::path("C:/test/file.txt");
    auto wide = PathUtils::toWide(p);
#ifdef Q_OS_WIN
    QCOMPARE(QString::fromWCharArray(wide.c_str()), QStringLiteral("C:\\test\\file.txt"));
#else
    QCOMPARE(QString::fromStdWString(wide), QStringLiteral("C:/test/file.txt"));
#endif
}

void TestPathUtils::toStdPath() {
    auto result = PathUtils::toStdPath(QStringLiteral("C:/test/file.txt"));
    QVERIFY(result.filename() == "file.txt");
}

void TestPathUtils::fromStdPath() {
    auto p = std::filesystem::path("C:/test/file.txt");
    auto qstr = PathUtils::fromStdPath(p);
#ifdef Q_OS_WIN
    QCOMPARE(qstr, QStringLiteral("C:\\test\\file.txt"));
#else
    QCOMPARE(qstr, QStringLiteral("C:/test/file.txt"));
#endif
}

// ── hasNonAscii ────────────────────────────────────────────────────

void TestPathUtils::hasNonAscii_true() {
    QVERIFY(PathUtils::hasNonAscii(QStringLiteral("C:/test/文件.txt")));
}

void TestPathUtils::hasNonAscii_false() {
    QVERIFY(!PathUtils::hasNonAscii(QStringLiteral("C:/test/file.txt")));
}

// ── CRC32 ──────────────────────────────────────────────────────────

void TestPathUtils::crc32_sameFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.bin";

    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray("Hello CRC32"));
        file.close();
    }

    auto crc1 = PathUtils::crc32(PathUtils::toStdPath(filePath));
    auto crc2 = PathUtils::crc32(PathUtils::toStdPath(filePath));
    QVERIFY(crc1.ok());
    QVERIFY(crc2.ok());
    QCOMPARE(crc1.value(), crc2.value());
}

void TestPathUtils::crc32_differentFiles() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString file1 = dir.path() + "/test1.bin";
    QString file2 = dir.path() + "/test2.bin";

    {
        QFile f1(file1);
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.write(QByteArray("AAAA"));
        f1.close();
    }
    {
        QFile f2(file2);
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write(QByteArray("BBBB"));
        f2.close();
    }

    auto crc1 = PathUtils::crc32(PathUtils::toStdPath(file1));
    auto crc2 = PathUtils::crc32(PathUtils::toStdPath(file2));
    QVERIFY(crc1.ok());
    QVERIFY(crc2.ok());
    QVERIFY(crc1.value() != crc2.value());
}

void TestPathUtils::crc32_emptyFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/empty.bin";

    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
    }

    auto crc = PathUtils::crc32(PathUtils::toStdPath(filePath));
    QVERIFY(crc.ok());
    QCOMPARE(crc.value(), 0u);
}

void TestPathUtils::crc32_nonexistentFile() {
    auto crc = PathUtils::crc32(std::filesystem::path("/nonexistent/file.bin"));
    QVERIFY(!crc.ok());
}

void TestPathUtils::crc32_memoryData() {
    const char* data = "Hello CRC32";
    uint32_t crcMem = PathUtils::crc32(reinterpret_cast<const uint8_t*>(data), 11);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.bin";

    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray(data, 11));
        file.close();
    }

    auto crcFile = PathUtils::crc32(PathUtils::toStdPath(filePath));
    QVERIFY(crcFile.ok());
    QCOMPARE(crcMem, crcFile.value());
}

// ── Text encoding detection（merged from TestEncoding） ───────────────────

void TestPathUtils::detect_utf8() {
    QByteArray data = "Hello World";
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf8);
}

void TestPathUtils::detect_utf8Bom() {
    QByteArray data;
    data.append('\xEF');
    data.append('\xBB');
    data.append('\xBF');
    data.append("Hello");
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf8Bom);
}

void TestPathUtils::detect_utf16LE() {
    QByteArray data;
    data.append('\xFF');
    data.append('\xFE');
    data.append("H\0e\0l\0l\0o\0", 10);
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf16LE);
}

void TestPathUtils::detect_utf16BE() {
    QByteArray data;
    data.append('\xFE');
    data.append('\xFF');
    data.append("\0H\0e\0l\0l\0o", 10);
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf16BE);
}

void TestPathUtils::detect_gbk() {
    QByteArray data;
    data.append('\xCE');
    data.append('\xD2');
    data.append('\xB0');
    data.append('\xAE');
    data.append('\xC4');
    data.append('\xE3');
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Gbk);
}

void TestPathUtils::detect_latin1() {
    QByteArray data;
    data.append('\xE9');
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Latin1);
}

void TestPathUtils::detect_empty() {
    QByteArray data;
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf8);
}

void TestPathUtils::detect_asciiOnly() {
    QByteArray data = "ASCII only";
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Utf8);
}

void TestPathUtils::detect_partialUtf8() {
    QByteArray data;
    data.append('\xCE');
    QCOMPARE(PathUtils::detectTextEncoding(data), PathUtils::TextEncoding::Latin1);
}

// ── Text decoding ────────────────────────────────────────────────────────

void TestPathUtils::decode_utf8() {
    QByteArray data = "Hello World";
    QString result = PathUtils::decodeText(data, PathUtils::TextEncoding::Utf8);
    QCOMPARE(result, QStringLiteral("Hello World"));
}

void TestPathUtils::decode_utf8Bom() {
    QByteArray data;
    data.append('\xEF');
    data.append('\xBB');
    data.append('\xBF');
    data.append("Hello");
    QString result = PathUtils::decodeText(data, PathUtils::TextEncoding::Utf8Bom);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestPathUtils::decode_utf16LE() {
    QByteArray data;
    data.append('\xFF');
    data.append('\xFE');
    data.append("H\0e\0l\0l\0o\0", 10);
    QString result = PathUtils::decodeText(data, PathUtils::TextEncoding::Utf16LE);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestPathUtils::decode_utf16BE() {
    QByteArray data;
    data.append('\xFE');
    data.append('\xFF');
    data.append("\0H\0e\0l\0l\0o", 10);
    QString result = PathUtils::decodeText(data, PathUtils::TextEncoding::Utf16BE);
    QCOMPARE(result, QStringLiteral("Hello"));
}

void TestPathUtils::decode_gbk() {
    QByteArray data;
    data.append('\xCE');
    data.append('\xD2');
    QCOMPARE(PathUtils::decodeText(data, PathUtils::TextEncoding::Gbk), QString::fromLocal8Bit(data));
}

void TestPathUtils::decode_latin1() {
    QByteArray data;
    data.append('\xE9');
    QString result = PathUtils::decodeText(data, PathUtils::TextEncoding::Latin1);
    QCOMPARE(result, QStringLiteral("é"));
}

// ── Text encoding ────────────────────────────────────────────────────────

void TestPathUtils::encode_utf8() {
    QByteArray result = PathUtils::encodeText(QStringLiteral("Hello"), PathUtils::TextEncoding::Utf8);
    QCOMPARE(result, QByteArray("Hello"));
}

void TestPathUtils::encode_utf8Bom() {
    QByteArray result = PathUtils::encodeText(QStringLiteral("Hello"), PathUtils::TextEncoding::Utf8Bom);
    QVERIFY(result.startsWith(QByteArray("\xEF\xBB\xBF", 3)));
    QCOMPARE(result.mid(3), QByteArray("Hello"));
}

void TestPathUtils::encode_utf16LE() {
    QByteArray result = PathUtils::encodeText(QStringLiteral("A"), PathUtils::TextEncoding::Utf16LE);
    QCOMPARE(result, QByteArray("A\0", 2));
}

void TestPathUtils::encode_utf16BE() {
    QByteArray result = PathUtils::encodeText(QStringLiteral("A"), PathUtils::TextEncoding::Utf16BE);
    QCOMPARE(result, QByteArray("\0A", 2));
}

// ── File read ────────────────────────────────────────────────────────────

void TestPathUtils::readFile_utf8() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Hello World");
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello World"));
}

void TestPathUtils::readFile_utf8Bom() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xEF\xBB\xBF");
        file.write("Hello");
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello"));
}

void TestPathUtils::readFile_utf16LE() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xFF\xFE");
        file.write("H\0e\0l\0l\0o\0", 10);
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello"));
}

void TestPathUtils::readFile_utf16BE() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xFE\xFF");
        file.write("\0H\0e\0l\0l\0o", 10);
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QStringLiteral("Hello"));
}

void TestPathUtils::readFile_empty() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/empty.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
    QVERIFY(result.value().isEmpty());
}

void TestPathUtils::readFile_nonexistent() {
    auto result = PathUtils::readFile(QStringLiteral("/nonexistent/file.txt"));
    QVERIFY(!result.ok());
}

void TestPathUtils::readFile_gbk() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xCE\xD2\xB0\xAE\xC4\xE3");
        file.close();
    }
    auto result = PathUtils::readFile(filePath);
    QVERIFY(result.ok());
}

// ── File write ───────────────────────────────────────────────────────────

void TestPathUtils::writeFile_utf8() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    auto result = PathUtils::writeFile(filePath, QStringLiteral("Hello"), PathUtils::TextEncoding::Utf8);
    QVERIFY(result.ok());
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("Hello"));
}

void TestPathUtils::writeFile_utf8Bom() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    auto result = PathUtils::writeFile(filePath, QStringLiteral("Hello"), PathUtils::TextEncoding::Utf8Bom);
    QVERIFY(result.ok());
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(data.startsWith(QByteArray("\xEF\xBB\xBF", 3)));
    QCOMPARE(data.mid(3), QByteArray("Hello"));
}

void TestPathUtils::writeFile_utf16LE() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.path() + "/test.txt";
    auto result = PathUtils::writeFile(filePath, QStringLiteral("A"), PathUtils::TextEncoding::Utf16LE);
    QVERIFY(result.ok());
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("A\0", 2));
}

void TestPathUtils::writeFile_invalidPath() {
    auto result = PathUtils::writeFile(QStringLiteral("/nonexistent/dir/file.txt"), QStringLiteral("test"),
                                       PathUtils::TextEncoding::Utf8);
    QVERIFY(!result.ok());
}

// ── Round-trip ───────────────────────────────────────────────────────────

void TestPathUtils::roundTrip_utf8() {
    const QString original = QStringLiteral("Hello World");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Utf8);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Utf8);
    QCOMPARE(decoded, original);
}

void TestPathUtils::roundTrip_utf16LE() {
    const QString original = QStringLiteral("Hello World");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Utf16LE);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Utf16LE);
    QCOMPARE(decoded, original);
}

void TestPathUtils::roundTrip_utf16BE() {
    const QString original = QStringLiteral("Hello World");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Utf16BE);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Utf16BE);
    QCOMPARE(decoded, original);
}

void TestPathUtils::roundTrip_gbk() {
    const QString original = QStringLiteral("你好世界");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Gbk);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Gbk);
    QCOMPARE(decoded, original);
}

void TestPathUtils::roundTrip_latin1() {
    const QString original = QStringLiteral("é");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Latin1);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Latin1);
    QCOMPARE(decoded, original);
}

void TestPathUtils::roundTrip_multilingual() {
    const QString original = QStringLiteral("Hello 你好 こんにちは");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Utf8);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Utf8);
    QCOMPARE(decoded, original);
}

void TestPathUtils::encodeDecode_gbk() {
    const QString original = QStringLiteral("你好");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Gbk);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Gbk);
    QCOMPARE(decoded, original);
}

void TestPathUtils::encodeDecode_latin1() {
    const QString original = QStringLiteral("café");
    QByteArray encoded = PathUtils::encodeText(original, PathUtils::TextEncoding::Latin1);
    QString decoded = PathUtils::decodeText(encoded, PathUtils::TextEncoding::Latin1);
    QCOMPARE(decoded, original);
}

QTEST_MAIN(TestPathUtils)
#include "TestPathUtils.moc"
