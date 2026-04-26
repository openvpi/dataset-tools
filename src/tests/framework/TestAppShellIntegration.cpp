#include <QTest>
#include <QSignalSpy>
#include <QLabel>

#include <dsfw/AppShell.h>

class TestAppShellIntegration : public QObject {
    Q_OBJECT
private slots:
    void testCreateShell();
    void testAddPage();
    void testPageSwitch();
    void testWorkingDirectory();
};

void TestAppShellIntegration::testCreateShell() {
    dsfw::AppShell shell;
    QCOMPARE(shell.pageCount(), 0);
    QCOMPARE(shell.currentPageIndex(), -1);
    QVERIFY(shell.currentPage() == nullptr);
}

void TestAppShellIntegration::testAddPage() {
    dsfw::AppShell shell;
    auto *page = new QLabel("Page 1");
    int idx = shell.addPage(page, "p1", {}, "Page 1");
    QCOMPARE(idx, 0);
    QCOMPARE(shell.pageCount(), 1);
    QCOMPARE(shell.pageAt(0), page);
}

void TestAppShellIntegration::testPageSwitch() {
    dsfw::AppShell shell;
    shell.addPage(new QLabel("A"), "a", {}, "A");
    shell.addPage(new QLabel("B"), "b", {}, "B");

    QSignalSpy spy(&shell, &dsfw::AppShell::currentPageChanged);
    shell.setCurrentPage(1);
    QCOMPARE(shell.currentPageIndex(), 1);
    QCOMPARE(spy.count(), 1);
}

void TestAppShellIntegration::testWorkingDirectory() {
    dsfw::AppShell shell;
    QSignalSpy spy(&shell, &dsfw::AppShell::workingDirectoryChanged);
    shell.setWorkingDirectory("/tmp/test");
    QCOMPARE(shell.workingDirectory(), QString("/tmp/test"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestAppShellIntegration)
#include "TestAppShellIntegration.moc"
