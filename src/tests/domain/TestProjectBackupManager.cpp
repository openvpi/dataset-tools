#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <dstools/ProjectBackupManager.h>
#include <dsfw/PathUtils.h>

using namespace dstools;
using namespace dsfw;

class TestProjectBackupManager : public QObject {
    Q_OBJECT
private slots:
    void backup_creates_new_file();
    void backup_rotates_old();
    void prune_removes_excess();
    void restore_from_backup();
    void find_latest_backup();
    void list_backups_empty();
    void list_backups_multiple();
    void backup_then_restore_roundtrip();
};

void TestProjectBackupManager::backup_creates_new_file() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\"}"));
        f.close();
    }

    auto result = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
    QVERIFY(result.ok());

    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QCOMPARE(backups.value().size(), 1u);
}

void TestProjectBackupManager::backup_rotates_old() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\"}"));
        f.close();
    }

    for (int i = 0; i < 5; ++i) {
        auto result = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
        QVERIFY(result.ok());
    }

    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QCOMPARE(backups.value().size(), 5u);
}

void TestProjectBackupManager::prune_removes_excess() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\"}"));
        f.close();
    }

    for (int i = 0; i < 15; ++i) {
        auto result = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
        QVERIFY(result.ok());
    }

    auto pruneResult = ProjectBackupManager::pruneBackups(PathUtils::toStdPath(projectPath), 5);
    QVERIFY(pruneResult.ok());

    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QVERIFY(backups.value().size() <= 5u);
}

void TestProjectBackupManager::restore_from_backup() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    const char* originalContent = "{\"version\":\"3.0.0\",\"data\":\"original\"}";
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(originalContent));
        f.close();
    }

    auto bakResult = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
    QVERIFY(bakResult.ok());

    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\",\"data\":\"corrupted\"}"));
        f.close();
    }

    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QVERIFY(!backups.value().empty());

    auto restoreResult = ProjectBackupManager::restoreFromBackup(backups.value()[0], PathUtils::toStdPath(projectPath));
    QVERIFY(restoreResult.ok());

    QFile restored(projectPath);
    QVERIFY(restored.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(restored.readAll()), QString::fromUtf8(originalContent));
}

void TestProjectBackupManager::find_latest_backup() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\"}"));
        f.close();
    }

    for (int i = 0; i < 3; ++i) {
        auto result = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
        QVERIFY(result.ok());
    }

    auto latest = ProjectBackupManager::findLatestBackup(PathUtils::toStdPath(projectPath));
    QVERIFY(latest.ok());
    QVERIFY(!latest.value().empty());
}

void TestProjectBackupManager::list_backups_empty() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("nonexistent.dsproj");
    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QCOMPARE(backups.value().size(), 0u);
}

void TestProjectBackupManager::list_backups_multiple() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("test.dsproj");
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("{\"version\":\"3.0.0\"}"));
        f.close();
    }

    for (int i = 0; i < 3; ++i) {
        auto result = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
        QVERIFY(result.ok());
    }

    auto backups = ProjectBackupManager::listBackups(PathUtils::toStdPath(projectPath));
    QVERIFY(backups.ok());
    QCOMPARE(backups.value().size(), 3u);
}

void TestProjectBackupManager::backup_then_restore_roundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString projectPath = tmpDir.filePath("roundtrip.dsproj");
    QString content = "{\"version\":\"3.0.0\",\"items\":[{\"id\":\"item1\"},{\"id\":\"item2\"}]}";
    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(content.toUtf8());
        f.close();
    }

    auto bakResult = ProjectBackupManager::createBackup(PathUtils::toStdPath(projectPath));
    QVERIFY(bakResult.ok());

    {
        QFile f(projectPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(QByteArray("corrupted"));
        f.close();
    }

    auto latest = ProjectBackupManager::findLatestBackup(PathUtils::toStdPath(projectPath));
    QVERIFY(latest.ok());
    QVERIFY(!latest.value().empty());

    auto restoreResult = ProjectBackupManager::restoreFromBackup(latest.value(), PathUtils::toStdPath(projectPath));
    QVERIFY(restoreResult.ok());

    QFile restored(projectPath);
    QVERIFY(restored.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(restored.readAll()), content);
}

QTEST_MAIN(TestProjectBackupManager)
#include "TestProjectBackupManager.moc"