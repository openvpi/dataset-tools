#include <QTest>
#include <QTemporaryDir>

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
    void testSentenceViewAllFieldsRoundTrip();
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
    auto result = DsDocument::loadFile("/nonexistent/path/test.ds");
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
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
    doc.addRawSentence(s1.dump());

    nlohmann::json s2;
    s2["offset"] = "0.5";
    s2["text"] = "world";
    doc.addRawSentence(s2.dump());

    auto saveResult = doc.saveFile(path);
    QVERIFY(saveResult.ok());

    auto loadResult = DsDocument::loadFile(path);
    QVERIFY(loadResult.ok());
    auto &loaded = loadResult.value();
    QCOMPARE(loaded.sentenceCount(), 2);
    QCOMPARE(loaded.sentenceView(0).text, QString("hello"));
    QCOMPARE(loaded.sentenceView(1).text, QString("world"));
}

void TestDsDocument::testSaveFileResultApi() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/result_test.ds";

    DsDocument doc;
    doc.addRawSentence(nlohmann::json::object({{"text", "test"}}).dump());

    auto result = doc.saveFile(path);
    QVERIFY(result.ok());
}

void TestDsDocument::testLoadFileResultApi() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/load_result_test.ds";

    DsDocument doc;
    doc.addRawSentence(nlohmann::json::object({{"text", "data"}}).dump());
    doc.saveFile(path);

    auto result = DsDocument::loadFile(path);
    QVERIFY(result.ok());
    QCOMPARE(result->sentenceCount(), 1);
}

void TestDsDocument::testSentenceAccess() {
    DsDocument doc;
    doc.addRawSentence(nlohmann::json::object({{"text", "first"}}).dump());
    doc.addRawSentence(nlohmann::json::object({{"text", "second"}}).dump());

    QCOMPARE(doc.sentenceCount(), 2);
    QCOMPARE(doc.sentenceView(0).text, QString("first"));
    QCOMPARE(doc.sentenceView(1).text, QString("second"));
}

void TestDsDocument::testSentenceViewAllFieldsRoundTrip() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/all_fields.ds";

    DsDocument doc;

    nlohmann::json s;
    s["text"] = "测试句子";
    s["ph_seq"] = "c e4 sh i4";
    s["ph_dur"] = "0.1 0.15 0.08 0.2";
    s["ph_num"] = "1 2 3 4";
    s["note_seq"] = "C4 E4 G4 C5";
    s["note_dur"] = "0.2 0.2 0.2 0.4";
    s["note_slur"] = "0 0 1 0";
    s["note_glide"] = "0 0 0 1";
    s["f0_seq"] = "260.0 330.0 392.0 523.0";
    s["word_span"] = "1 3 4 4";
    s["offset"] = 1.5;
    s["f0_timestep"] = 0.01;
    doc.addRawSentence(s.dump());

    const auto sv = doc.sentenceView(0);
    QCOMPARE(sv.text, QString("测试句子"));
    QCOMPARE(sv.phSeq, QString("c e4 sh i4"));
    QCOMPARE(sv.phDur, QString("0.1 0.15 0.08 0.2"));
    QCOMPARE(sv.phNum, QString("1 2 3 4"));
    QCOMPARE(sv.noteSeq, QString("C4 E4 G4 C5"));
    QCOMPARE(sv.noteDur, QString("0.2 0.2 0.2 0.4"));
    QCOMPARE(sv.noteSlur, QString("0 0 1 0"));
    QCOMPARE(sv.noteGlide, QString("0 0 0 1"));
    QCOMPARE(sv.f0Seq, QString("260.0 330.0 392.0 523.0"));
    QCOMPARE(sv.wordSpan, QString("1 3 4 4"));
    QCOMPARE(sv.offset, 1.5);
    QCOMPARE(sv.f0Timestep, 0.01);

    auto saveResult = doc.saveFile(path);
    QVERIFY(saveResult.ok());

    auto loadResult = DsDocument::loadFile(path);
    QVERIFY(loadResult.ok());
    const auto &loaded = loadResult.value();
    QCOMPARE(loaded.sentenceCount(), 1);

    const auto lsv = loaded.sentenceView(0);
    QCOMPARE(lsv.text, QString("测试句子"));
    QCOMPARE(lsv.phSeq, QString("c e4 sh i4"));
    QCOMPARE(lsv.phDur, QString("0.1 0.15 0.08 0.2"));
    QCOMPARE(lsv.phNum, QString("1 2 3 4"));
    QCOMPARE(lsv.noteSeq, QString("C4 E4 G4 C5"));
    QCOMPARE(lsv.noteDur, QString("0.2 0.2 0.2 0.4"));
    QCOMPARE(lsv.noteSlur, QString("0 0 1 0"));
    QCOMPARE(lsv.noteGlide, QString("0 0 0 1"));
    QCOMPARE(lsv.f0Seq, QString("260.0 330.0 392.0 523.0"));
    QCOMPARE(lsv.wordSpan, QString("1 3 4 4"));
    QCOMPARE(lsv.offset, 1.5);
    QCOMPARE(lsv.f0Timestep, 0.01);
}

void TestDsDocument::testSetFilePath() {
    DsDocument doc;
    doc.setFilePath("/some/path.ds");
    QCOMPARE(doc.filePath(), QString("/some/path.ds"));
}

void TestDsDocument::testSaveWithoutPath() {
    DsDocument doc;
    doc.addRawSentence(nlohmann::json::object({{"text", "test"}}).dump());
    auto result = doc.saveFile();
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
}

void TestDsDocument::testLoadMalformedJson() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/malformed.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{invalid json content");
    f.close();

    auto result = DsDocument::loadFile(path);
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
}

void TestDsDocument::testLoadEmptyArray() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/empty.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("[]");
    f.close();

    auto result = DsDocument::loadFile(path);
    QVERIFY(!result.ok());
}

void TestDsDocument::testLoadNonArrayJson() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/object.ds";

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{\"key\": \"value\"}");
    f.close();

    auto result = DsDocument::loadFile(path);
    QVERIFY(!result.ok());
}

QTEST_GUILESS_MAIN(TestDsDocument)
#include "TestDsDocument.moc"