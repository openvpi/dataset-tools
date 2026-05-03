#include <QTest>
#include <QTemporaryDir>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <dstools/DsDocument.h>

#include <cmath>

using namespace dstools;

class TestDsDocument : public QObject {
    Q_OBJECT
private slots:
    void testEmptyDocument();
    void testLoadInvalidPath();
    void testSaveAndLoadRoundTrip();
    void testSaveFileResultApi();
    void testLoadFileResultApi();
    void testSentenceAccess();
    void testNumberOrString_number();
    void testNumberOrString_string();
    void testNumberOrString_missing();
    void testNumberOrString_invalidString();
    void testSetFilePath();
    void testSaveWithoutPath();
    void testLoadMalformedJson();
    void testLoadEmptyArray();
    void testLoadNonArrayJson();
};

void TestDsDocument::testEmptyDocument() {
    DsDocument doc;
    QVERIFY(doc.isEmpty());
    QCOMPARE(doc.sentenceCount(), 0);
    QVERIFY(doc.filePath().isEmpty());
}

void TestDsDocument::testLoadInvalidPath() {
    QString error;
    auto doc = DsDocument::load("/nonexistent/path/test.ds", error);
    QVERIFY(!error.isEmpty());
    QVERIFY(doc.isEmpty());
}

void TestDsDocument::testSaveAndLoadRoundTrip() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/test.ds";

    DsDocument doc;
    nlohmann::json s1;
    s1["offset"] = 0.0;
    s1["text"] = "hello";
    s1["ph_seq"] = "h @ l oU";
    s1["ph_dur"] = "0.1 0.1 0.1 0.2";
    doc.sentences().push_back(s1);

    nlohmann::json s2;
    s2["offset"] = "0.5";
    s2["text"] = "world";
    doc.sentences().push_back(s2);

    QString saveError;
    QVERIFY(doc.save(path, saveError));
    QVERIFY(saveError.isEmpty());

    QString loadError;
    auto loaded = DsDocument::load(path, loadError);
    QVERIFY(loadError.isEmpty());
    QCOMPARE(loaded.sentenceCount(), 2);
    QCOMPARE(loaded.sentence(0)["text"].get<std::string>(), "hello");
    QCOMPARE(loaded.sentence(1)["text"].get<std::string>(), "world");
}

void TestDsDocument::testSaveFileResultApi() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/result_test.ds";

    DsDocument doc;
    doc.sentences().push_back(nlohmann::json::object({{"text", "test"}}));

    auto result = doc.saveFile(path);
    QVERIFY(result.ok());
}

void TestDsDocument::testLoadFileResultApi() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/load_result_test.ds";

    DsDocument doc;
    doc.sentences().push_back(nlohmann::json::object({{"text", "data"}}));
    doc.saveFile(path);

    auto result = DsDocument::loadFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result->sentenceCount(), 1);
}

void TestDsDocument::testSentenceAccess() {
    DsDocument doc;
    doc.sentences().push_back(nlohmann::json::object({{"text", "first"}}));
    doc.sentences().push_back(nlohmann::json::object({{"text", "second"}}));

    QCOMPARE(doc.sentenceCount(), 2);
    QCOMPARE(doc.sentence(0)["text"].get<std::string>(), "first");
    QCOMPARE(doc.sentence(1)["text"].get<std::string>(), "second");
}

void TestDsDocument::testNumberOrString_number() {
    nlohmann::json obj = {{"offset", 1.5}};
    QCOMPARE(DsDocument::numberOrString(obj, "offset", 0.0), 1.5);
}

void TestDsDocument::testNumberOrString_string() {
    nlohmann::json obj = {{"offset", "2.5"}};
    QCOMPARE(DsDocument::numberOrString(obj, "offset", 0.0), 2.5);
}

void TestDsDocument::testNumberOrString_missing() {
    nlohmann::json obj = {{"other", 1.0}};
    QCOMPARE(DsDocument::numberOrString(obj, "offset", 9.9), 9.9);
}

void TestDsDocument::testNumberOrString_invalidString() {
    nlohmann::json obj = {{"offset", "not_a_number"}};
    QCOMPARE(DsDocument::numberOrString(obj, "offset", 3.3), 3.3);
}

void TestDsDocument::testSetFilePath() {
    DsDocument doc;
    doc.setFilePath("/some/path.ds");
    QCOMPARE(doc.filePath(), QString("/some/path.ds"));
}

void TestDsDocument::testSaveWithoutPath() {
    DsDocument doc;
    doc.sentences().push_back(nlohmann::json::object({{"text", "test"}}));
    QString error;
    QVERIFY(!doc.save(error));
    QVERIFY(!error.isEmpty());
}

void TestDsDocument::testLoadMalformedJson() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/malformed.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{invalid json content");
    f.close();

    QString error;
    auto doc = DsDocument::load(path, error);
    QVERIFY(!error.isEmpty());
    QVERIFY(doc.isEmpty());
}

void TestDsDocument::testLoadEmptyArray() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/empty.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("[]");
    f.close();

    QString error;
    auto doc = DsDocument::load(path, error);
    QVERIFY(!error.isEmpty());
}

void TestDsDocument::testLoadNonArrayJson() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/object.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{\"key\": \"value\"}");
    f.close();

    QString error;
    auto doc = DsDocument::load(path, error);
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestDsDocument)
#include "TestDsDocument.moc"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
