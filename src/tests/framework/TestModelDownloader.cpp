#include <QTest>
#include <dstools/ModelDownloader.h>
#include <cstring>

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
    QVERIFY(iface->downloaderName() != nullptr);
}

void TestModelDownloader::testInitialStatus() {
    ModelDownloader downloader;
    QVERIFY(downloader.downloaderName() != nullptr);
    QVERIFY(strlen(downloader.downloaderName()) > 0);
}

QTEST_GUILESS_MAIN(TestModelDownloader)
#include "TestModelDownloader.moc"
