#include <QTest>
#include <QTemporaryDir>
#include <dstools/PhNumCalculator.h>
#include <dstools/TranscriptionCsv.h>

using namespace dstools;

class TestPhNumCalculator : public QObject {
    Q_OBJECT
private slots:
    void testCalculate_simpleVowels();
    void testCalculate_consonantFollowsVowel();
    void testCalculate_empty();
    void testCalculate_singlePhoneme();
    void testCalculate_mixed();
    void testSetVowelsAndConsonants();
    void testLoadDictionaryInvalidPath();
    void testLoadDictionaryValidFile();
    void testCalculateBatch();
};

void TestPhNumCalculator::testCalculate_simpleVowels() {
    PhNumCalculator calc;
    calc.setVowels({"a", "i", "u", "SP", "AP"});
    calc.setConsonants({"k", "s"});

    auto result = calc.calculate("a i u");
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QString("1 1 1"));
}

void TestPhNumCalculator::testCalculate_consonantFollowsVowel() {
    PhNumCalculator calc;
    calc.setVowels({"a", "i"});
    calc.setConsonants({"k", "s"});

    auto result = calc.calculate("k a s i");
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QString("1 2 1"));
}

void TestPhNumCalculator::testCalculate_empty() {
    PhNumCalculator calc;
    calc.setVowels({"a"});
    calc.setConsonants({"k"});

    auto result = calc.calculate("");
    QVERIFY(result.ok());
    QVERIFY(result.value().isEmpty());
}

void TestPhNumCalculator::testCalculate_singlePhoneme() {
    PhNumCalculator calc;
    calc.setVowels({"a"});
    calc.setConsonants({"k"});

    auto result = calc.calculate("a");
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QString("1"));
}

void TestPhNumCalculator::testCalculate_mixed() {
    PhNumCalculator calc;
    calc.setVowels({"a", "i", "SP"});
    calc.setConsonants({"k", "s"});

    auto result = calc.calculate("SP k a s i SP");
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QString("2 2 1 1"));
}

void TestPhNumCalculator::testSetVowelsAndConsonants() {
    PhNumCalculator calc;
    calc.setVowels({"a", "e"});
    calc.setConsonants({"b", "c"});

    auto result = calc.calculate("b a c e");
    QVERIFY(result.ok());
    QCOMPARE(result.value(), QString("1 2 1"));
}

void TestPhNumCalculator::testLoadDictionaryInvalidPath() {
    PhNumCalculator calc;
    auto result = calc.loadDictionary("/nonexistent/dict.txt");
    QVERIFY(!result.ok());
    QVERIFY(!result.error().empty());
}

void TestPhNumCalculator::testLoadDictionaryValidFile() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString dictPath = tmp.path() + "/dict.txt";

    QFile f(dictPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "ka\tk a\n";
    out << "shi\ts i\n";
    out << "a\ta\n";
    f.close();

    PhNumCalculator calc;
    auto result = calc.loadDictionary(dictPath);
    QVERIFY(result.ok());
    QVERIFY(calc.isLoaded());

    auto calcResult = calc.calculate("k a s i a");
    QVERIFY(calcResult.ok());
    QCOMPARE(calcResult.value(), QString("1 2 1 1"));
}

void TestPhNumCalculator::testCalculateBatch() {
    PhNumCalculator calc;
    calc.setVowels({"a", "i"});
    calc.setConsonants({"k", "s"});

    std::vector<TranscriptionRow> rows(2);
    rows[0].phSeq = "k a";
    rows[1].phSeq = "s i";

    auto result = calc.calculateBatch(rows);
    QVERIFY(result.ok());
    QCOMPARE(rows[0].phNum, QString("1 1"));
    QCOMPARE(rows[1].phNum, QString("1 1"));
}

QTEST_GUILESS_MAIN(TestPhNumCalculator)
#include "TestPhNumCalculator.moc"