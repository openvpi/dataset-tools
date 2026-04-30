#include <QTest>
#include <QMap>
#include <QRegularExpression>
#include <dsfw/IFileIOProvider.h>
#include <dsfw/LocalFileIOProvider.h>

using namespace dstools;

class MockFileIOProvider : public IFileIOProvider {
public:
    QMap<QString, QByteArray> m_files;
    QMap<QString, bool> m_dirs;

    Result<QByteArray> readFile(const QString &path) override {
        if (!m_files.contains(path))
            return Result<QByteArray>::Error("file not found: " + path.toStdString());
        return Result<QByteArray>::Ok(m_files[path]);
    }

    Result<void> writeFile(const QString &path, const QByteArray &data) override {
        m_files[path] = data;
        return Result<void>::Ok();
    }

    Result<QString> readText(const QString &path) override {
        if (!m_files.contains(path))
            return Result<QString>::Error("file not found: " + path.toStdString());
        return Result<QString>::Ok(QString::fromUtf8(m_files[path]));
    }

    Result<void> writeText(const QString &path, const QString &text) override {
        m_files[path] = text.toUtf8();
        return Result<void>::Ok();
    }

    bool exists(const QString &path) override {
        return m_files.contains(path) || m_dirs.contains(path);
    }

    FileInfo fileInfo(const QString &path) override {
        FileInfo fi;
        fi.path = path;
        if (m_dirs.contains(path)) {
            fi.exists = true;
            fi.isDirectory = true;
        } else if (m_files.contains(path)) {
            fi.exists = true;
            fi.size = m_files[path].size();
        }
        return fi;
    }

    Result<QStringList> listFiles(const QString &directory, const QStringList &nameFilters,
                                  bool /*recursive*/) override {
        QStringList result;
        QString dirPrefix = directory.endsWith('/') ? directory : directory + '/';
        for (auto it = m_files.constBegin(); it != m_files.constEnd(); ++it) {
            if (!it.key().startsWith(dirPrefix))
                continue;
            // Check name filters
            if (nameFilters.isEmpty()) {
                result << it.key();
            } else {
                QString name = it.key().mid(dirPrefix.size());
                for (const auto &filter : nameFilters) {
                    if (QRegularExpression::fromWildcard(filter).match(name).hasMatch()) {
                        result << it.key();
                        break;
                    }
                }
            }
        }
        return Result<QStringList>::Ok(result);
    }

    Result<void> mkdirs(const QString &path) override {
        m_dirs[path] = true;
        return Result<void>::Ok();
    }

    Result<void> removeFile(const QString &path) override {
        if (!m_files.contains(path))
            return Result<void>::Error("cannot remove: " + path.toStdString());
        m_files.remove(path);
        return Result<void>::Ok();
    }

    Result<void> copyFile(const QString &src, const QString &dst) override {
        if (!m_files.contains(src))
            return Result<void>::Error("source not found: " + src.toStdString());
        m_files[dst] = m_files[src];
        return Result<void>::Ok();
    }
};

class TestIFileIOProviderMock : public QObject {
    Q_OBJECT
private slots:
    void testWriteAndReadFile();
    void testWriteAndReadText();
    void testExists();
    void testRemoveFile();
    void testCreateDir();
    void testListFiles();
    void testCopyFile();
    void testReadNonExistent();
    void testRemoveNonExistent();
    void testFileInfo();
    void testGlobalProviderSwap();
};

void TestIFileIOProviderMock::testWriteAndReadFile() {
    MockFileIOProvider io;
    auto wr = io.writeFile("/mock/test.bin", QByteArray("binary data"));
    QVERIFY(wr.ok());

    auto rd = io.readFile("/mock/test.bin");
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QByteArray("binary data"));
}

void TestIFileIOProviderMock::testWriteAndReadText() {
    MockFileIOProvider io;
    auto wr = io.writeText("/mock/test.txt", "hello text");
    QVERIFY(wr.ok());

    auto rd = io.readText("/mock/test.txt");
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QString("hello text"));
}

void TestIFileIOProviderMock::testExists() {
    MockFileIOProvider io;
    QVERIFY(!io.exists("/mock/no.txt"));

    io.writeFile("/mock/yes.txt", QByteArray("x"));
    QVERIFY(io.exists("/mock/yes.txt"));
}

void TestIFileIOProviderMock::testRemoveFile() {
    MockFileIOProvider io;
    io.writeFile("/mock/del.txt", QByteArray("bye"));
    QVERIFY(io.exists("/mock/del.txt"));

    auto r = io.removeFile("/mock/del.txt");
    QVERIFY(r.ok());
    QVERIFY(!io.exists("/mock/del.txt"));
}

void TestIFileIOProviderMock::testCreateDir() {
    MockFileIOProvider io;
    QVERIFY(!io.exists("/mock/a/b/c"));

    auto r = io.mkdirs("/mock/a/b/c");
    QVERIFY(r.ok());
    QVERIFY(io.exists("/mock/a/b/c"));

    auto fi = io.fileInfo("/mock/a/b/c");
    QVERIFY(fi.isDirectory);
}

void TestIFileIOProviderMock::testListFiles() {
    MockFileIOProvider io;
    io.writeFile("/dir/a.txt", QByteArray("a"));
    io.writeFile("/dir/b.txt", QByteArray("b"));
    io.writeFile("/dir/c.dat", QByteArray("c"));

    auto r = io.listFiles("/dir", {"*.txt"}, false);
    QVERIFY(r.ok());
    QCOMPARE(r.value().size(), 2);
}

void TestIFileIOProviderMock::testCopyFile() {
    MockFileIOProvider io;
    io.writeFile("/mock/src.txt", QByteArray("copy me"));

    auto r = io.copyFile("/mock/src.txt", "/mock/dst.txt");
    QVERIFY(r.ok());

    auto rd = io.readFile("/mock/dst.txt");
    QVERIFY(rd.ok());
    QCOMPARE(rd.value(), QByteArray("copy me"));
}

void TestIFileIOProviderMock::testReadNonExistent() {
    MockFileIOProvider io;
    auto r = io.readFile("/mock/nope.bin");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());

    auto rt = io.readText("/mock/nope.txt");
    QVERIFY(!rt.ok());
}

void TestIFileIOProviderMock::testRemoveNonExistent() {
    MockFileIOProvider io;
    auto r = io.removeFile("/mock/nope.txt");
    QVERIFY(!r.ok());
    QVERIFY(!r.error().empty());
}

void TestIFileIOProviderMock::testFileInfo() {
    MockFileIOProvider io;
    auto fi = io.fileInfo("/mock/nope.txt");
    QVERIFY(!fi.exists);
    QVERIFY(!fi.isDirectory);

    io.writeFile("/mock/f.txt", QByteArray("hello"));
    fi = io.fileInfo("/mock/f.txt");
    QVERIFY(fi.exists);
    QCOMPARE(fi.size, 5);
}

void TestIFileIOProviderMock::testGlobalProviderSwap() {
    // Save default
    IFileIOProvider *original = fileIOProvider();
    QVERIFY(original != nullptr);

    // Swap to mock
    MockFileIOProvider mock;
    setFileIOProvider(&mock);
    QCOMPARE(fileIOProvider(), &mock);

    // Reset to default
    resetFileIOProvider();
    IFileIOProvider *restored = fileIOProvider();
    QVERIFY(restored != nullptr);
    QVERIFY(restored != &mock);
}

QTEST_GUILESS_MAIN(TestIFileIOProviderMock)
#include "TestIFileIOProviderMock.moc"
