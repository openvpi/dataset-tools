#include <QtTest/QtTest>

#include <dsfw/widgets/PathSelector.h>
#include <dsfw/widgets/ViewportController.h>
#include <dsfw/widgets/RunProgressRow.h>
#include <dsfw/widgets/FileProgressTracker.h>

using namespace dsfw::widgets;

class TestDsfwWidgets : public QObject {
    Q_OBJECT

private slots:
    void testPathSelectorConstruction() {
        PathSelector ps(PathSelector::OpenFile, QStringLiteral("Test"), {}, nullptr);
        QVERIFY(ps.path().isEmpty());
        QCOMPARE(ps.mode(), PathSelector::OpenFile);
        QVERIFY(ps.lineEdit() != nullptr);
    }

    void testPathSelectorSetPath() {
        PathSelector ps(PathSelector::Directory, QStringLiteral("Dir"), {}, nullptr);
        ps.setPath(QStringLiteral("/tmp/test"));
        QCOMPARE(ps.path(), QStringLiteral("/tmp/test"));
    }

    void testPathSelectorModes() {
        PathSelector ps1(PathSelector::OpenFile, QStringLiteral("F"), {}, nullptr);
        QCOMPARE(ps1.mode(), PathSelector::OpenFile);

        PathSelector ps2(PathSelector::SaveFile, QStringLiteral("S"), {}, nullptr);
        QCOMPARE(ps2.mode(), PathSelector::SaveFile);

        PathSelector ps3(PathSelector::Directory, QStringLiteral("D"), {}, nullptr);
        QCOMPARE(ps3.mode(), PathSelector::Directory);
    }

    void testViewportControllerDefaults() {
        ViewportController vc(nullptr);
        QCOMPARE(vc.startSec(), 0.0);
        QCOMPARE(vc.endSec(), 10.0);
        QCOMPARE(vc.totalDuration(), 0.0);
        // Default resolution snaps to nearest table entry from 40 → 30 or 50
        QVERIFY(vc.resolution() > 0);
    }

    void testViewportControllerSetRange() {
        ViewportController vc(nullptr);
        vc.setTotalDuration(60.0);
        vc.setViewRange(5.0, 15.0);
        QCOMPARE(vc.startSec(), 5.0);
        QCOMPARE(vc.endSec(), 15.0);
    }

    void testViewportControllerZoom() {
        ViewportController vc(nullptr);
        vc.setTotalDuration(60.0);
        vc.setViewRange(0.0, 10.0);
        int oldRes = vc.resolution();
        vc.zoomIn(5.0);
        QVERIFY(vc.resolution() < oldRes);
    }

    void testViewportControllerScroll() {
        ViewportController vc(nullptr);
        vc.setTotalDuration(60.0);
        vc.setViewRange(5.0, 15.0);
        vc.scrollBy(5.0);
        QCOMPARE(vc.startSec(), 10.0);
        QCOMPARE(vc.endSec(), 20.0);
    }

    void testViewportControllerClamp() {
        ViewportController vc(nullptr);
        vc.setTotalDuration(30.0);
        vc.setViewRange(-5.0, 5.0);
        QCOMPARE(vc.startSec(), 0.0);
    }

    void testRunProgressRowConstruction() {
        RunProgressRow row(QStringLiteral("Start"), nullptr);
        // Just verify construction doesn't crash
        QVERIFY(true);
    }

    void testFileProgressTrackerDefaults() {
        FileProgressTracker tracker(FileProgressTracker::LabelOnly, nullptr);
        tracker.setProgress(0, 0);
    }

    void testFileProgressTrackerProgress() {
        FileProgressTracker tracker(FileProgressTracker::LabelOnly, nullptr);
        tracker.setProgress(5, 10);
    }

    void testFileProgressTrackerWithBar() {
        FileProgressTracker tracker(FileProgressTracker::ProgressBarStyle, nullptr);
        tracker.setProgress(50, 100);
    }
};

QTEST_MAIN(TestDsfwWidgets)
#include "TestDsfwWidgets.moc"
