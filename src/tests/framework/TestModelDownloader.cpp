#include <QTest>
#include <dsfw/ModelDownloader.h>

using namespace dstools;

class TestModelDownloader : public QObject {
    Q_OBJECT
private slots:
    void testConstruction();
    void testImplementsInterface();
    void testInitialStatus();
};

void TestModelDownloader::testConstruction() {
    ModelDownloader downloader;
    // Should not crash — basic smoke test
    QVERIFY(true);
}

void TestModelDownloader::testImplementsInterface() {
    ModelDownloader downloader;
    IModelDownloader *iface = &downloader;
    QVERIFY(iface != nullptr);
    // Verify it satisfies the interface contract
    QCOMPARE(iface->downloadStatus("nonexistent"), DownloadStatus::Idle);
}

void TestModelDownloader::testInitialStatus() {
    ModelDownloader downloader;
    // Unknown model should return Idle
    QCOMPARE(downloader.downloadStatus("any_model"), DownloadStatus::Idle);
}

QTEST_GUILESS_MAIN(TestModelDownloader)
#include "TestModelDownloader.moc"
