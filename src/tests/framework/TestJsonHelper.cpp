#include <QTest>
#include <dsfw/JsonHelper.h>

using namespace dstools;
using json = nlohmann::json;

class TestJsonHelper : public QObject {
    Q_OBJECT
private slots:
    void testResolveNestedPath();
    void testResolveNullOnMissing();
    void testGetWithDefault();
    void testGetMissingKey();
    void testGetObject();
    void testGetObjectMissing();
    void testLoadFileMissing();
};

void TestJsonHelper::testResolveNestedPath() {
    json root = {{"a", {{"b", {{"c", 42}}}}}};
    const auto *node = JsonHelper::resolve(root, "a/b/c");
    QVERIFY(node != nullptr);
    QCOMPARE(node->get<int>(), 42);
}

void TestJsonHelper::testResolveNullOnMissing() {
    json root = {{"a", 1}};
    QVERIFY(JsonHelper::resolve(root, "x/y/z") == nullptr);
}

void TestJsonHelper::testGetWithDefault() {
    json root = {{"count", 5}};
    QCOMPARE(JsonHelper::get<int>(root, "count", 0), 5);
}

void TestJsonHelper::testGetMissingKey() {
    json root = {{"a", 1}};
    QCOMPARE(JsonHelper::get<int>(root, "missing", 99), 99);
}

void TestJsonHelper::testGetObject() {
    json root = {{"obj", {{"k", "v"}}}};
    auto obj = JsonHelper::getObject(root, "obj");
    QVERIFY(obj.is_object());
    QCOMPARE(obj["k"].get<std::string>(), std::string("v"));
}

void TestJsonHelper::testGetObjectMissing() {
    json root = {{"a", 1}};
    auto obj = JsonHelper::getObject(root, "missing");
    QVERIFY(obj.is_object());
    QVERIFY(obj.empty());
}

void TestJsonHelper::testLoadFileMissing() {
    auto result = JsonHelper::loadFile("nonexistent_file.json");
    QVERIFY(!result);
}

QTEST_GUILESS_MAIN(TestJsonHelper)
#include "TestJsonHelper.moc"
