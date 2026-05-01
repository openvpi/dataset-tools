#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <dstools/TextGridToCsv.h>

using namespace dstools;

class TestTextGridToCsv : public QObject {
    Q_OBJECT
private slots:
    void testExtractFromTextGrid_basic();
    void testExtractFromTextGrid_caseInsensitiveTier();
    void testExtractFromTextGrid_emptyIntervals();
    void testExtractFromTextGrid_nonExistentFile();
    void testExtractDirectory();
    void testExtractDirectory_emptyDir();
};

static bool writeTextGrid(const QString &path, const QString &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(content.toUtf8());
    return true;
}

static QString basicTextGrid() {
    return QStringLiteral(
        "File type = \"ooTextFile\"\n"
        "Object class = \"TextGrid\"\n"
        "\n"
        "xmin = 0\n"
        "xmax = 1.5\n"
        "tiers? <exists>\n"
        "size = 1\n"
        "item []:\n"
        "    item [1]:\n"
        "        class = \"IntervalTier\"\n"
        "        name = \"phones\"\n"
        "        xmin = 0\n"
        "        xmax = 1.5\n"
        "        intervals: size = 3\n"
        "        intervals [1]:\n"
        "            xmin = 0\n"
        "            xmax = 0.3\n"
        "            text = \"sh\"\n"
        "        intervals [2]:\n"
        "            xmin = 0.3\n"
        "            xmax = 0.8\n"
        "            text = \"a\"\n"
        "        intervals [3]:\n"
        "            xmin = 0.8\n"
        "            xmax = 1.5\n"
        "            text = \"n\"\n");
}

void TestTextGridToCsv::testExtractFromTextGrid_basic() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString tgPath = tmp.path() + "/test.TextGrid";
    QVERIFY(writeTextGrid(tgPath, basicTextGrid()));

    TranscriptionRow row;
    QString error;
    QVERIFY(TextGridToCsv::extractFromTextGrid(tgPath, row, error));
    QVERIFY(error.isEmpty());
    QCOMPARE(row.name, QString("test"));
    QCOMPARE(row.phSeq, QString("sh a n"));
    // Durations: 0.3, 0.5, 0.7
    QStringList durs = row.phDur.split(' ');
    QCOMPARE(durs.size(), 3);
    QVERIFY(qAbs(durs[0].toDouble() - 0.3) < 0.001);
    QVERIFY(qAbs(durs[1].toDouble() - 0.5) < 0.001);
    QVERIFY(qAbs(durs[2].toDouble() - 0.7) < 0.001);
}

void TestTextGridToCsv::testExtractFromTextGrid_caseInsensitiveTier() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // Use "Phones" (capitalized) as tier name
    QString content = basicTextGrid();
    content.replace("\"phones\"", "\"Phones\"");
    QString tgPath = tmp.path() + "/capitalized.TextGrid";
    QVERIFY(writeTextGrid(tgPath, content));

    TranscriptionRow row;
    QString error;
    QVERIFY(TextGridToCsv::extractFromTextGrid(tgPath, row, error));
    QCOMPARE(row.phSeq, QString("sh a n"));
}

void TestTextGridToCsv::testExtractFromTextGrid_emptyIntervals() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString content = QStringLiteral(
        "File type = \"ooTextFile\"\n"
        "Object class = \"TextGrid\"\n"
        "\n"
        "xmin = 0\n"
        "xmax = 1.5\n"
        "tiers? <exists>\n"
        "size = 1\n"
        "item []:\n"
        "    item [1]:\n"
        "        class = \"IntervalTier\"\n"
        "        name = \"phones\"\n"
        "        xmin = 0\n"
        "        xmax = 1.5\n"
        "        intervals: size = 4\n"
        "        intervals [1]:\n"
        "            xmin = 0\n"
        "            xmax = 0.2\n"
        "            text = \"\"\n"
        "        intervals [2]:\n"
        "            xmin = 0.2\n"
        "            xmax = 0.5\n"
        "            text = \"b\"\n"
        "        intervals [3]:\n"
        "            xmin = 0.5\n"
        "            xmax = 1.0\n"
        "            text = \"\"\n"
        "        intervals [4]:\n"
        "            xmin = 1.0\n"
        "            xmax = 1.5\n"
        "            text = \"c\"\n");
    QString tgPath = tmp.path() + "/empty_intervals.TextGrid";
    QVERIFY(writeTextGrid(tgPath, content));

    TranscriptionRow row;
    QString error;
    QVERIFY(TextGridToCsv::extractFromTextGrid(tgPath, row, error));
    // Empty intervals should be skipped — only "b" and "c" remain
    QCOMPARE(row.phSeq, QString("b c"));
}

void TestTextGridToCsv::testExtractFromTextGrid_nonExistentFile() {
    TranscriptionRow row;
    QString error;
    QVERIFY(!TextGridToCsv::extractFromTextGrid("/nonexistent/file.TextGrid", row, error));
    QVERIFY(!error.isEmpty());
}

void TestTextGridToCsv::testExtractDirectory() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // Create two TextGrid files with names that sort alphabetically
    QVERIFY(writeTextGrid(tmp.path() + "/alpha.TextGrid", basicTextGrid()));

    QString content2 = basicTextGrid();
    content2.replace("\"sh\"", "\"k\"");
    content2.replace("\"a\"", "\"o\"");
    content2.replace("\"n\"", "\"t\"");
    QVERIFY(writeTextGrid(tmp.path() + "/beta.TextGrid", content2));

    std::vector<TranscriptionRow> rows;
    QString error;
    QVERIFY(TextGridToCsv::extractDirectory(tmp.path(), rows, error));
    QCOMPARE(rows.size(), size_t(2));
    // Sorted by name
    QCOMPARE(rows[0].name, QString("alpha"));
    QCOMPARE(rows[1].name, QString("beta"));
    QCOMPARE(rows[0].phSeq, QString("sh a n"));
    QCOMPARE(rows[1].phSeq, QString("k o t"));
}

void TestTextGridToCsv::testExtractDirectory_emptyDir() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    std::vector<TranscriptionRow> rows;
    QString error;
    QVERIFY(TextGridToCsv::extractDirectory(tmp.path(), rows, error));
    QVERIFY(rows.empty());
}

QTEST_GUILESS_MAIN(TestTextGridToCsv)
#include "TestTextGridToCsv.moc"
