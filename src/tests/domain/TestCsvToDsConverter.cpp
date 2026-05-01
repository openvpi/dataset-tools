#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <dstools/CsvToDsConverter.h>
#include <dstools/DsDocument.h>
#include <dstools/TranscriptionCsv.h>

using namespace dstools;

class TestCsvToDsConverter : public QObject {
    Q_OBJECT
private slots:
    void testDsToCsv_basic();
    void testDsToCsv_nonExistentDir();
    void testConvertFromMemory_withMockF0();
    void testConvert_nonExistentCsv();
    void testConvertFromMemory_emptyRows();
};

void TestCsvToDsConverter::testDsToCsv_basic() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Create a .ds file with known data
    DsDocument doc;
    nlohmann::json sentence;
    sentence["offset"] = 0.0;
    sentence["text"] = "hello";
    sentence["ph_seq"] = "h eh l ow";
    sentence["ph_dur"] = "0.1 0.2 0.15 0.25";
    sentence["ph_num"] = "2 2";
    sentence["note_seq"] = "C4 D4";
    sentence["note_dur"] = "0.3 0.4";
    doc.sentences().push_back(sentence);

    QString dsPath = tmp.path() + "/test.ds";
    auto saveResult = doc.saveFile(dsPath);
    QVERIFY(saveResult.ok());

    // Create a dummy .wav file alongside (empty file)
    QFile wavFile(tmp.path() + "/test.wav");
    QVERIFY(wavFile.open(QIODevice::WriteOnly));
    wavFile.close();

    // Convert ds -> csv
    QString csvPath = tmp.path() + "/output.csv";
    QString error;
    QVERIFY(CsvToDsConverter::dsToCsv(tmp.path(), csvPath, error));

    // Read back and verify
    std::vector<TranscriptionRow> rows;
    QVERIFY(TranscriptionCsv::read(csvPath, rows, error));
    QVERIFY(!rows.empty());
    QCOMPARE(rows[0].name, QString("test"));
    QCOMPARE(rows[0].phSeq, QString("h eh l ow"));
}

void TestCsvToDsConverter::testDsToCsv_nonExistentDir() {
    QString error;
    QVERIFY(!CsvToDsConverter::dsToCsv("/nonexistent/dir", "/tmp/out.csv", error));
    QVERIFY(!error.isEmpty());
}

void TestCsvToDsConverter::testConvertFromMemory_withMockF0() {
    QTemporaryDir tmpWav;
    QVERIFY(tmpWav.isValid());
    QTemporaryDir tmpOut;
    QVERIFY(tmpOut.isValid());

    // Create dummy wav files
    QFile wav1(tmpWav.path() + "/song1.wav");
    QVERIFY(wav1.open(QIODevice::WriteOnly));
    wav1.close();

    // Prepare rows
    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "song1";
    row.phSeq = "a b c";
    row.phDur = "0.1 0.2 0.3";
    rows.push_back(row);

    // Mock F0 callback
    auto mockF0 = [](const QString &, std::vector<float> &f0, QString &) {
        f0.resize(100, 440.0f);
        return true;
    };

    CsvToDsConverter::Options opts;
    opts.wavsDir = tmpWav.path();
    opts.outputDir = tmpOut.path();
    opts.sampleRate = 44100;
    opts.hopSize = 512;

    QString error;
    bool ok = CsvToDsConverter::convertFromMemory(rows, opts, mockF0, nullptr, error);
    QVERIFY(ok);

    // Verify .ds file was created
    QDir outDir(tmpOut.path());
    QStringList dsFiles = outDir.entryList(QStringList() << "*.ds", QDir::Files);
    QVERIFY(!dsFiles.isEmpty());
}

void TestCsvToDsConverter::testConvert_nonExistentCsv() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    CsvToDsConverter::Options opts;
    opts.csvPath = "/nonexistent/file.csv";
    opts.wavsDir = tmp.path();
    opts.outputDir = tmp.path();

    QString error;
    QVERIFY(!CsvToDsConverter::convert(opts, nullptr, nullptr, error));
    QVERIFY(!error.isEmpty());
}

void TestCsvToDsConverter::testConvertFromMemory_emptyRows() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    std::vector<TranscriptionRow> rows;

    CsvToDsConverter::Options opts;
    opts.wavsDir = tmp.path();
    opts.outputDir = tmp.path();

    auto mockF0 = [](const QString &, std::vector<float> &f0, QString &) {
        f0.resize(100, 440.0f);
        return true;
    };

    QString error;
    QVERIFY(CsvToDsConverter::convertFromMemory(rows, opts, mockF0, nullptr, error));

    // No output files
    QDir outDir(tmp.path());
    QStringList dsFiles = outDir.entryList(QStringList() << "*.ds", QDir::Files);
    QVERIFY(dsFiles.isEmpty());
}

QTEST_GUILESS_MAIN(TestCsvToDsConverter)
#include "TestCsvToDsConverter.moc"
