#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include <dsfw/PathUtils.h>

using namespace dsfw;

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

QTEST_MAIN(TestPathUtils)
#include "TestPathUtils.moc"