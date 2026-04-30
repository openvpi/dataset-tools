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
    void testGetQStringWithDefault();
    void testGetVec();
    void testGetVecMissing();
    void testGetMap();
    void testGetRequiredSuccess();
    void testGetRequiredFailure();
    void testContains();
    void testContainsMissing();
    void testGetObject();
    void testGetArray();
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
    QCOMPARE(JsonHelper::get<int>(root, "missing", 99), 99);
}

void TestJsonHelper::testGetQStringWithDefault() {
    json root = {{"name", "hello"}};
    QCOMPARE(JsonHelper::get<QString>(root, "name", QString("def")), QString("hello"));
    QCOMPARE(JsonHelper::get<QString>(root, "nope", QString("def")), QString("def"));
}

void TestJsonHelper::testGetVec() {
    json root = {{"nums", {1, 2, 3}}};
    auto v = JsonHelper::getVec<int>(root, "nums");
    QCOMPARE(v.size(), 3u);
    QCOMPARE(v[0], 1);
    QCOMPARE(v[2], 3);
}

void TestJsonHelper::testGetVecMissing() {
    json root = {{"a", 1}};
    auto v = JsonHelper::getVec<int>(root, "missing");
    QVERIFY(v.empty());
}

void TestJsonHelper::testGetMap() {
    json root = {{"kv", {{"x", 1}, {"y", 2}}}};
    auto m = JsonHelper::getMap<std::string, int>(root, "kv");
    QCOMPARE(m.size(), 2u);
    QCOMPARE(m["x"], 1);
    QCOMPARE(m["y"], 2);
}

void TestJsonHelper::testGetRequiredSuccess() {
    json root = {{"val", 10}};
    int out = 0;
    std::string error;
    QVERIFY(JsonHelper::getRequired(root, "val", out, error));
    QCOMPARE(out, 10);
}

void TestJsonHelper::testGetRequiredFailure() {
    json root = {{"a", 1}};
    int out = 0;
    std::string error;
    QVERIFY(!JsonHelper::getRequired(root, "missing", out, error));
    QVERIFY(!error.empty());
}

void TestJsonHelper::testContains() {
    json root = {{"a", {{"b", 1}}}};
    QVERIFY(JsonHelper::contains(root, "a/b"));
}

void TestJsonHelper::testContainsMissing() {
    json root = {{"a", 1}};
    QVERIFY(!JsonHelper::contains(root, "x/y"));
}

void TestJsonHelper::testGetObject() {
    json root = {{"obj", {{"k", "v"}}}};
    auto obj = JsonHelper::getObject(root, "obj");
    QVERIFY(obj.is_object());
    QCOMPARE(obj["k"].get<std::string>(), std::string("v"));
}

void TestJsonHelper::testGetArray() {
    json root = {{"arr", {1, 2, 3}}};
    auto arr = JsonHelper::getArray(root, "arr");
    QVERIFY(arr.is_array());
    QCOMPARE(arr.size(), 3u);
}

QTEST_GUILESS_MAIN(TestJsonHelper)
#include "TestJsonHelper.moc"
