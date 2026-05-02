#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
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
    void testReadTextMissingFile();
    void testWriteToInvalidPath();
    void testListFilesNonExistentDir();
    void testCopyFromNonExistentSource();
    void testRemoveNonExistentFile();
    void testFileInfoNonExistent();
    void testWriteOverwritesExisting();
    void testCopyOverwritesDestination();
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

void TestLocalFileIOProvider::testReadTextMissingFile() {
    LocalFileIOProvider io;
    auto r = io.readText("/nonexistent/path/file.txt");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestLocalFileIOProvider::testWriteToInvalidPath() {
    LocalFileIOProvider io;
#ifdef Q_OS_WIN
    QString invalidPath = QStringLiteral("Z:\\nonexistent_root_dir\\sub\\file.bin");
#else
    // Create a temporary regular file, then attempt to write under it as if
    // it were a directory.  mkpath() must fail because the parent component
    // is a file, not a directory — this works regardless of user privileges.
    QTemporaryFile barrier;
    QVERIFY(barrier.open());
    QString invalidPath = barrier.fileName() + "/sub/file.bin";
#endif
    auto r = io.writeFile(invalidPath, QByteArray("data"));
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestLocalFileIOProvider::testListFilesNonExistentDir() {
    LocalFileIOProvider io;
    auto r = io.listFiles("/nonexistent_dir", {"*.txt"}, false);
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestLocalFileIOProvider::testCopyFromNonExistentSource() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;
    auto r = io.copyFile("/nonexistent/src.txt", tmp.path() + "/dst.txt");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestLocalFileIOProvider::testRemoveNonExistentFile() {
    LocalFileIOProvider io;
    auto r = io.removeFile("/nonexistent/file.txt");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestLocalFileIOProvider::testFileInfoNonExistent() {
    LocalFileIOProvider io;
    auto fi = io.fileInfo("/nonexistent/file.txt");
    QVERIFY(!fi.exists);
    QVERIFY(!fi.isDirectory);
    QCOMPARE(fi.size, qint64(0));
}

void TestLocalFileIOProvider::testWriteOverwritesExisting() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;
    QString path = tmp.path() + "/overwrite.txt";
    io.writeText(path, "first");
    io.writeText(path, "second");
    auto rd = io.readText(path);
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QString("second"));
}

void TestLocalFileIOProvider::testCopyOverwritesDestination() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalFileIOProvider io;
    QString src = tmp.path() + "/src.txt";
    QString dst = tmp.path() + "/dst.txt";
    io.writeText(src, "source");
    io.writeText(dst, "old_dest");
    auto r = io.copyFile(src, dst);
    QVERIFY(r.ok());
    auto rd = io.readText(dst);
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QString("source"));
}

QTEST_GUILESS_MAIN(TestLocalFileIOProvider)
#include "TestLocalFileIOProvider.moc"
