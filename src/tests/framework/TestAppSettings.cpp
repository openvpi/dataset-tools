#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDir>
#include <dsfw/AppSettings.h>
#include <dsfw/AppPaths.h>

using namespace dstools;

class TestAppSettings : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void testGetDefault();
    void testSetAndGet();
    void testSetBool();
    void testNestedPath();
    void testContains();
    void testRemove();
    void testObserveNotify();
    void testFlushReload();

private:
    QString m_configDir;
};

void TestAppSettings::initTestCase() {
    m_configDir = dsfw::AppPaths::configDir();
}

void TestAppSettings::cleanup() {
    static const QStringList appNames = {
        "TestApp_default", "TestApp_setget", "TestApp_bool",
        "TestApp_nested",  "TestApp_contains", "TestApp_remove",
        "TestApp_observe", "TestApp_flush"};
    for (const auto &name : appNames) {
        QString path = m_configDir + QStringLiteral("/") + name + QStringLiteral(".json");
        QFile::remove(path);
    }
}

void TestAppSettings::testGetDefault() {
    AppSettings settings("TestApp_default");
    SettingsKey<int> key("test/value", 42);
    QCOMPARE(settings.get(key), 42);
}

void TestAppSettings::testSetAndGet() {
    AppSettings settings("TestApp_setget");
    SettingsKey<int> key("test/count", 0);
    settings.set(key, 100);
    QCOMPARE(settings.get(key), 100);
}

void TestAppSettings::testSetBool() {
    AppSettings settings("TestApp_bool");
    SettingsKey<bool> key("feature/enabled", false);
    settings.set(key, true);
    QVERIFY(settings.get(key));
}

void TestAppSettings::testNestedPath() {
    AppSettings settings("TestApp_nested");
    SettingsKey<QString> key("ui/theme/name", QString("light"));
    settings.set(key, QString("dark"));
    QCOMPARE(settings.get(key), QString("dark"));
}

void TestAppSettings::testContains() {
    AppSettings settings("TestApp_contains");
    SettingsKey<int> key("some/key", 0);
    QVERIFY(!settings.contains(key));
    settings.set(key, 5);
    QVERIFY(settings.contains(key));
}

void TestAppSettings::testRemove() {
    AppSettings settings("TestApp_remove");
    SettingsKey<int> key("del/me", 0);
    settings.set(key, 10);
    QVERIFY(settings.contains(key));
    settings.remove(key);
    QVERIFY(!settings.contains(key));
}

void TestAppSettings::testObserveNotify() {
    AppSettings settings("TestApp_observe");
    SettingsKey<int> key("obs/val", 0);

    int received = -1;
    settings.observe<int>(key, [&](const int &v) { received = v; });
    settings.set(key, 77);
    QCOMPARE(received, 77);
}

void TestAppSettings::testFlushReload() {
    const QString appName = "TestApp_flush";
    SettingsKey<int> key("persist/val", 0);

    {
        AppSettings s1(appName);
        s1.set(key, 42);
        s1.flush();
    }
    {
        AppSettings s2(appName);
        QCOMPARE(s2.get(key), 42);
    }
}

QTEST_GUILESS_MAIN(TestAppSettings)
#include "TestAppSettings.moc"
