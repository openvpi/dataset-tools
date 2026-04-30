#include <QTest>
#include <QTemporaryDir>
#include <dsfw/LocalFileIOProvider.h>

using namespace dstools;

class TestLocalFileIOProvider : public QObject {
    Q_OBJECT
private slots:
    void testWriteAndReadFile();
    void testWriteAndReadText();
    void testExists();
    void testListFiles();
    void testMkdirs();
    void testRemoveFile();
    void testCopyFile();
    void testReadMissingFile();
};

void TestLocalFileIOProvider::testWriteAndReadFile() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString path = tmp.path() + "/test.bin";
    QByteArray data("hello bytes");
    auto wr = io.writeFile(path, data);
    QVERIFY(wr.ok());

    auto rd = io.readFile(path);
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), data);
}

void TestLocalFileIOProvider::testWriteAndReadText() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString path = tmp.path() + "/test.txt";
    QString text = "hello text";
    auto wr = io.writeText(path, text);
    QVERIFY(wr.ok());

    auto rd = io.readText(path);
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), text);
}

void TestLocalFileIOProvider::testExists() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString path = tmp.path() + "/exists.txt";
    QVERIFY(!io.exists(path));

    io.writeText(path, "x");
    QVERIFY(io.exists(path));
}

void TestLocalFileIOProvider::testListFiles() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    io.writeText(tmp.path() + "/a.txt", "a");
    io.writeText(tmp.path() + "/b.txt", "b");
    io.writeText(tmp.path() + "/c.dat", "c");

    auto r = io.listFiles(tmp.path(), {"*.txt"}, false);
    QVERIFY(r.ok());
    QCOMPARE(r.value().size(), 2);
}

void TestLocalFileIOProvider::testMkdirs() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString dir = tmp.path() + "/a/b/c";
    auto r = io.mkdirs(dir);
    QVERIFY(r.ok());
    QVERIFY(QDir(dir).exists());
}

void TestLocalFileIOProvider::testRemoveFile() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString path = tmp.path() + "/del.txt";
    io.writeText(path, "bye");
    QVERIFY(io.exists(path));

    auto r = io.removeFile(path);
    QVERIFY(r.ok());
    QVERIFY(!io.exists(path));
}

void TestLocalFileIOProvider::testCopyFile() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;

    QString src = tmp.path() + "/src.txt";
    QString dst = tmp.path() + "/dst.txt";
    io.writeText(src, "copy me");

    auto r = io.copyFile(src, dst);
    QVERIFY(r.ok());

    auto rd = io.readText(dst);
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QString("copy me"));
}

void TestLocalFileIOProvider::testReadMissingFile() {
    LocalFileIOProvider io;
    auto r = io.readFile("/nonexistent/path/file.bin");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

QTEST_GUILESS_MAIN(TestLocalFileIOProvider)
#include "TestLocalFileIOProvider.moc"
