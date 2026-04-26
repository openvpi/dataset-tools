#include <QTest>
#include <QTemporaryDir>
#include <dstools/TranscriptionCsv.h>

using namespace dstools;

class TestTranscriptionCsv : public QObject {
    Q_OBJECT
private slots:
    void testParseBasic();
    void testParseWithOptionalColumns();
    void testParseMissingRequiredColumn();
    void testParseEmptyContent();
    void testParseQuotedFields();
    void testParseBOM();
    void testWriteAndReadRoundTrip();
    void testWriteOnlyNonEmptyColumns();
    void testReadNonExistentFile();
    void testWriteToInvalidPath();
    void testParseTrailingNewline();
};

void TestTranscriptionCsv::testParseBasic() {
    QString content = "name,ph_seq,ph_dur\ntest,a b c,0.1 0.2 0.3\n";
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(result.ok());
    QCOMPARE(result->size(), size_t(1));
    QCOMPARE((*result)[0].name, QString("test"));
    QCOMPARE((*result)[0].phSeq, QString("a b c"));
    QCOMPARE((*result)[0].phDur, QString("0.1 0.2 0.3"));
}

void TestTranscriptionCsv::testParseWithOptionalColumns() {
    QString content = "name,ph_seq,ph_dur,ph_num,note_seq\ntest,a b,0.1 0.2,2,C4 D4\n";
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(result.ok());
    QCOMPARE(result->size(), size_t(1));
    QCOMPARE((*result)[0].phNum, QString("2"));
    QCOMPARE((*result)[0].noteSeq, QString("C4 D4"));
}

void TestTranscriptionCsv::testParseMissingRequiredColumn() {
    QString content = "name,ph_seq\ntest,a b c\n";
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(!result.ok());
    QVERIFY(result.error().find("ph_dur") != std::string::npos);
}

void TestTranscriptionCsv::testParseEmptyContent() {
    auto result = TranscriptionCsv::parse("");
    QVERIFY(!result.ok());
}

void TestTranscriptionCsv::testParseQuotedFields() {
    QString content = "name,ph_seq,ph_dur\n\"test file\",\"a b c\",\"0.1 0.2 0.3\"\n";
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(result.ok());
    QCOMPARE((*result)[0].name, QString("test file"));
}

void TestTranscriptionCsv::testParseBOM() {
    QString content = QChar(0xFEFF) + QString("name,ph_seq,ph_dur\ntest,a b,0.1 0.2\n");
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(result.ok());
    QCOMPARE(result->size(), size_t(1));
}

void TestTranscriptionCsv::testWriteAndReadRoundTrip() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/test.csv";

    std::vector<TranscriptionRow> original;
    TranscriptionRow row;
    row.name = "song1";
    row.phSeq = "a b c";
    row.phDur = "0.1 0.2 0.3";
    row.phNum = "1 1 1";
    original.push_back(row);

    auto writeResult = TranscriptionCsv::write(path, original);
    QVERIFY(writeResult.ok());

    auto readResult = TranscriptionCsv::read(path);
    QVERIFY(readResult.ok());
    const auto &loaded = readResult.value();
    QCOMPARE(loaded.size(), size_t(1));
    QCOMPARE(loaded[0].name, QString("song1"));
    QCOMPARE(loaded[0].phSeq, QString("a b c"));
    QCOMPARE(loaded[0].phDur, QString("0.1 0.2 0.3"));
}

void TestTranscriptionCsv::testWriteOnlyNonEmptyColumns() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString path = tmp.path() + "/minimal.csv";

    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "test";
    row.phSeq = "a b";
    row.phDur = "0.1 0.2";
    rows.push_back(row);

    auto writeResult = TranscriptionCsv::write(path, rows);
    QVERIFY(writeResult.ok());

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(f.readAll());
    f.close();

    QVERIFY(content.contains("name"));
    QVERIFY(content.contains("ph_seq"));
    QVERIFY(content.contains("ph_dur"));
}

void TestTranscriptionCsv::testReadNonExistentFile() {
    auto result = TranscriptionCsv::read("/nonexistent/file.csv");
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
}

void TestTranscriptionCsv::testWriteToInvalidPath() {
    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "test";
    row.phSeq = "a";
    row.phDur = "0.1";
    rows.push_back(row);

    auto result = TranscriptionCsv::write("/nonexistent/dir/file.csv", rows);
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
}

void TestTranscriptionCsv::testParseTrailingNewline() {
    QString content = "name,ph_seq,ph_dur\ntest1,a b,0.1 0.2\ntest2,c d,0.3 0.4\n\n";
    auto result = TranscriptionCsv::parse(content);
    QVERIFY(result.ok());
    QCOMPARE(result->size(), size_t(2));
}

QTEST_GUILESS_MAIN(TestTranscriptionCsv)
#include "TestTranscriptionCsv.moc"