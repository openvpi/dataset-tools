#include <QTest>
#include <QTemporaryDir>
#include <dsfw/widgets/RecentPathStore.h>
#include <dsfw/AppSettings.h>


class TestRecentPathStore : public QObject {
    Q_OBJECT

private slots:
    void testLoadEmptyKey() {
        QString result = RecentPathStore::load({});
        QVERIFY(result.isEmpty());
    }

    void testLoadNonExistentKey() {
        QString result = RecentPathStore::load(QStringLiteral("test/nonexistent_key_12345"));
        QVERIFY(result.isEmpty());
    }

    void testSaveAndLoad() {
        QString key = QStringLiteral("test/recent_path_store_test");
        QString testPath = QStringLiteral("C:/some/test/path");

        RecentPathStore::save(key, testPath);
        QString loaded = RecentPathStore::load(key);
        QCOMPARE(loaded, testPath);
    }

    void testSaveExtractsDirectory() {
        QString key = QStringLiteral("test/recent_path_store_dir_test");
        QString testFilePath = QStringLiteral("C:/some/test/path/file.wav");

        RecentPathStore::save(key, testFilePath);
        QString loaded = RecentPathStore::load(key);
        QCOMPARE(loaded, QStringLiteral("C:/some/test/path"));
    }

    void testSaveEmptyKey() {
        // Should not crash
        RecentPathStore::save({}, QStringLiteral("C:/some/path"));
    }

    void testSaveDirectoryPath() {
        QString key = QStringLiteral("test/recent_path_store_dirpath_test");
        QString dirPath = QStringLiteral("C:/some/directory");

        RecentPathStore::save(key, dirPath);
        QString loaded = RecentPathStore::load(key);
        QCOMPARE(loaded, dirPath);
    }
};

QTEST_MAIN(TestRecentPathStore)
#include "TestRecentPathStore.moc"
