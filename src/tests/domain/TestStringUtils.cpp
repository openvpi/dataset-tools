#include <QTest>
#include <dstools/StringUtils.h>
#include <dstools/TranscriptionCsv.h>

using namespace dstools;

class TestStringUtils : public QObject {
    Q_OBJECT

private slots:
    void testFormatDur6_integer();
    void testFormatDur6_decimal();
    void testFormatDur6_trailingZeros();
    void testFormatDur6_allZeros();
    void testFormatDur6_zero();

    void testParsePhSeq_normal();
    void testParsePhSeq_empty();
    void testParsePhSeq_single();

    void testParsePhDur_normal();
    void testParsePhDur_empty();

    void testParsePhNum_normal();
    void testParsePhNum_empty();

    void testWriteNotesToRow_normal();
    void testWriteNotesToRow_empty();
};

void TestStringUtils::testFormatDur6_integer() {
    QCOMPARE(formatDur6(1.0), QString("1"));
}

void TestStringUtils::testFormatDur6_decimal() {
    QCOMPARE(formatDur6(0.123456), QString("0.123456"));
}

void TestStringUtils::testFormatDur6_trailingZeros() {
    QCOMPARE(formatDur6(0.5), QString("0.5"));
    QCOMPARE(formatDur6(0.25), QString("0.25"));
    QCOMPARE(formatDur6(0.125), QString("0.125"));
}

void TestStringUtils::testFormatDur6_allZeros() {
    QCOMPARE(formatDur6(2.0), QString("2"));
}

void TestStringUtils::testFormatDur6_zero() {
    QCOMPARE(formatDur6(0.0), QString("0"));
}

void TestStringUtils::testParsePhSeq_normal() {
    auto result = parsePhSeq("a b c d");
    QCOMPARE(result.size(), static_cast<size_t>(4));
    QCOMPARE(result[0], std::string("a"));
    QCOMPARE(result[3], std::string("d"));
}

void TestStringUtils::testParsePhSeq_empty() {
    auto result = parsePhSeq("");
    QVERIFY(result.empty());
}

void TestStringUtils::testParsePhSeq_single() {
    auto result = parsePhSeq("hello");
    QCOMPARE(result.size(), static_cast<size_t>(1));
    QCOMPARE(result[0], std::string("hello"));
}

void TestStringUtils::testParsePhDur_normal() {
    auto result = parsePhDur("0.1 0.2 0.3");
    QCOMPARE(result.size(), static_cast<size_t>(3));
    QCOMPARE(result[0], 0.1f);
    QCOMPARE(result[2], 0.3f);
}

void TestStringUtils::testParsePhDur_empty() {
    auto result = parsePhDur("");
    QVERIFY(result.empty());
}

void TestStringUtils::testParsePhNum_normal() {
    auto result = parsePhNum("3 2 4");
    QCOMPARE(result.size(), static_cast<size_t>(3));
    QCOMPARE(result[0], 3);
    QCOMPARE(result[2], 4);
}

void TestStringUtils::testParsePhNum_empty() {
    auto result = parsePhNum("");
    QVERIFY(result.empty());
}

void TestStringUtils::testWriteNotesToRow_normal() {
    TranscriptionRow row;
    std::vector<std::tuple<std::string, float, int>> notes = {
        {"C4", 0.5f, 0},
        {"D4", 0.25f, 1},
    };
    writeNotesToRow(row, notes);
    QCOMPARE(row.noteSeq, QString("C4 D4"));
    QCOMPARE(row.noteDur, QString("0.5 0.25"));
    QCOMPARE(row.noteSlur, QString("0 1"));
}

void TestStringUtils::testWriteNotesToRow_empty() {
    TranscriptionRow row;
    std::vector<std::tuple<std::string, float, int>> notes;
    writeNotesToRow(row, notes);
    QCOMPARE(row.noteSeq, QString(""));
    QCOMPARE(row.noteDur, QString(""));
    QCOMPARE(row.noteSlur, QString(""));
}

QTEST_GUILESS_MAIN(TestStringUtils)
#include "TestStringUtils.moc"
