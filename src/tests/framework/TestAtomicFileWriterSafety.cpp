#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <dsfw/AtomicFileWriter.h>
#include <dsfw/PathUtils.h>
#include <nlohmann/json.hpp>

class TestAtomicFileWriterSafety : public QObject {
    Q_OBJECT
private slots:
    void write_and_read_back();
    void write_json_valid();
    void write_json_invalid_triggers_restore();
    void no_backup_does_not_create_bak();
    void missing_parent_dir_auto_created();
    void empty_content_writes_successfully();

    void backup_restoration_preserves_content();
    void validation_disabled_allows_invalid_json();
    void write_failure_readonly_directory();
    void multiple_consecutive_writes();
    void large_content_write();
};

void TestAtomicFileWriterSafety::write_and_read_back() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("test.txt"));
    const std::string content = "hello world";

    auto res = dsfw::AtomicFileWriter::write(path, content);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("test.txt"));
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QTextStream(&f).readAll(), QString::fromStdString(content));
}

void TestAtomicFileWriterSafety::write_json_valid() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("test.json"));
    const nlohmann::json j = {{"key", "value"}, {"num", 42}};
    const std::string content = j.dump();

    auto res = dsfw::AtomicFileWriter::writeJson(path, content);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("test.json"));
    QVERIFY(f.exists());
}

void TestAtomicFileWriterSafety::write_json_invalid_triggers_restore() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("invalid.json"));

    {
        nlohmann::json j = {{"original", "data"}};
        auto writeRes = dsfw::AtomicFileWriter::writeJson(path, j.dump());
        QVERIFY(writeRes.ok());
    }

    dsfw::AtomicFileWriter::setBackupEnabled(true);
    dsfw::AtomicFileWriter::setValidationEnabled(true);

    const std::string invalid = "{ this is not valid json }";

    auto res = dsfw::AtomicFileWriter::writeJson(path, invalid);
    QVERIFY(!res.ok());
}

void TestAtomicFileWriterSafety::no_backup_does_not_create_bak() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("nobak.txt"));

    {
        auto res = dsfw::AtomicFileWriter::write(path, "original");
        QVERIFY(res.ok());
    }

    dsfw::AtomicFileWriter::setBackupEnabled(false);

    auto res = dsfw::AtomicFileWriter::write(path, "updated");
    QVERIFY(res.ok());

    QVERIFY(!QFile::exists(tmpDir.filePath("nobak.txt.bak")));

    dsfw::AtomicFileWriter::setBackupEnabled(true);
}

void TestAtomicFileWriterSafety::missing_parent_dir_auto_created() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto nestedPath = dsfw::PathUtils::toStdPath(tmpDir.filePath("subdir/deep/nested.txt"));
    const std::string content = "auto-created parent";

    auto res = dsfw::AtomicFileWriter::write(nestedPath, content);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("subdir/deep/nested.txt"));
    QVERIFY(f.exists());
}

void TestAtomicFileWriterSafety::empty_content_writes_successfully() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("empty.txt"));
    const std::string empty;

    auto res = dsfw::AtomicFileWriter::write(path, empty);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("empty.txt"));
    QVERIFY(f.exists());
    QVERIFY(f.size() == 0);
}

void TestAtomicFileWriterSafety::backup_restoration_preserves_content() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("restore_test.json"));

    dsfw::AtomicFileWriter::setBackupEnabled(true);
    dsfw::AtomicFileWriter::setValidationEnabled(true);

    {
        nlohmann::json j = {{"version", 1}, {"data", "original"}};
        auto res = dsfw::AtomicFileWriter::writeJson(path, j.dump());
        QVERIFY(res.ok());
    }

    QFile origFile(tmpDir.filePath("restore_test.json"));
    QVERIFY(origFile.open(QIODevice::ReadOnly));
    QByteArray originalContent = origFile.readAll();
    origFile.close();

    const std::string invalid = "not json at all";
    auto res = dsfw::AtomicFileWriter::writeJson(path, invalid);
    QVERIFY(!res.ok());

    QFile restoredFile(tmpDir.filePath("restore_test.json"));
    QVERIFY(restoredFile.open(QIODevice::ReadOnly));
    QCOMPARE(restoredFile.readAll(), originalContent);
}

void TestAtomicFileWriterSafety::validation_disabled_allows_invalid_json() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("novalidate.json"));

    dsfw::AtomicFileWriter::setValidationEnabled(false);
    dsfw::AtomicFileWriter::setBackupEnabled(false);

    const std::string invalid = "{ not valid }";
    auto res = dsfw::AtomicFileWriter::writeJson(path, invalid);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("novalidate.json"));
    QVERIFY(f.exists());

    dsfw::AtomicFileWriter::setValidationEnabled(true);
    dsfw::AtomicFileWriter::setBackupEnabled(true);
}

void TestAtomicFileWriterSafety::write_failure_readonly_directory() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("readonly_dir/test.txt"));

    QDir(tmpDir.path()).mkdir("readonly_dir");
    QFile dirFile(tmpDir.filePath("readonly_dir"));
    QVERIFY(dirFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::ReadGroup |
                                    QFileDevice::ReadOther | QFileDevice::ExeOwner |
                                    QFileDevice::ExeGroup | QFileDevice::ExeOther));

    auto res = dsfw::AtomicFileWriter::write(path, "should fail");
    QVERIFY(!res.ok());

    dirFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                           QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                           QFileDevice::ReadOther | QFileDevice::ExeOther);
}

void TestAtomicFileWriterSafety::multiple_consecutive_writes() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("consecutive.txt"));

    dsfw::AtomicFileWriter::setBackupEnabled(false);

    for (int i = 0; i < 50; ++i) {
        std::string content = "iteration_" + std::to_string(i);
        auto res = dsfw::AtomicFileWriter::write(path, content);
        QVERIFY2(res.ok(), qPrintable(QStringLiteral("write failed at iteration %1").arg(i)));
    }

    QFile f(tmpDir.filePath("consecutive.txt"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(QTextStream(&f).readAll(), QStringLiteral("iteration_49"));

    dsfw::AtomicFileWriter::setBackupEnabled(true);
}

void TestAtomicFileWriterSafety::large_content_write() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const auto path = dsfw::PathUtils::toStdPath(tmpDir.filePath("large.txt"));

    std::string content;
    for (int i = 0; i < 10000; ++i)
        content += "line_" + std::to_string(i) + ": some test data for atomic writer stress testing\n";

    auto res = dsfw::AtomicFileWriter::write(path, content);
    QVERIFY(res.ok());

    QFile f(tmpDir.filePath("large.txt"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(QTextStream(&f).readAll().toStdString(), content);
}

QTEST_MAIN(TestAtomicFileWriterSafety)
#include "TestAtomicFileWriterSafety.moc"