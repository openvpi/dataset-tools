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
    void testConvertFromMemory_optionalFields();
    void testConvertFromMemory_dsContentVerification();
    void testConvertFromMemory_f0CallbackError();
    void testDsToCsv_multiSentence();
    void testDsToCsv_noteGlideFill();
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

void TestCsvToDsConverter::testConvertFromMemory_optionalFields() {
    QTemporaryDir tmpWav;
    QVERIFY(tmpWav.isValid());
    QTemporaryDir tmpOut;
    QVERIFY(tmpOut.isValid());

    QFile wav1(tmpWav.path() + "/song1.wav");
    QVERIFY(wav1.open(QIODevice::WriteOnly));
    wav1.close();

    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "song1";
    row.phSeq = "a b";
    row.phDur = "0.1 0.2";
    row.phNum = "2";
    row.noteSeq = "C4 D4";
    row.noteDur = "0.1 0.2";
    row.noteSlur = "0 0";
    row.noteGlide = "none none";
    rows.push_back(row);

    TranscriptionRow row2;
    row2.name = "song2";
    row2.phSeq = "c d";
    row2.phDur = "0.3 0.4";
    rows.push_back(row2);

    auto mockF0 = [](const QString &, std::vector<float> &f0, QString &) {
        f0.resize(50, 440.0f);
        return true;
    };

    CsvToDsConverter::Options opts;
    opts.wavsDir = tmpWav.path();
    opts.outputDir = tmpOut.path();

    QString error;
    QVERIFY(CsvToDsConverter::convertFromMemory(rows, opts, mockF0, nullptr, error));

    QDir outDir(tmpOut.path());
    QStringList dsFiles = outDir.entryList(QStringList() << "*.ds", QDir::Files);
    QVERIFY(dsFiles.size() >= 1);
}

void TestCsvToDsConverter::testConvertFromMemory_dsContentVerification() {
    QTemporaryDir tmpWav;
    QVERIFY(tmpWav.isValid());
    QTemporaryDir tmpOut;
    QVERIFY(tmpOut.isValid());

    QFile wav1(tmpWav.path() + "/song1.wav");
    QVERIFY(wav1.open(QIODevice::WriteOnly));
    wav1.close();

    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "song1";
    row.phSeq = "a b";
    row.phDur = "0.1 0.2";
    rows.push_back(row);

    auto mockF0 = [](const QString &, std::vector<float> &f0, QString &) {
        f0.resize(50, 440.0f);
        return true;
    };

    CsvToDsConverter::Options opts;
    opts.wavsDir = tmpWav.path();
    opts.outputDir = tmpOut.path();
    opts.sampleRate = 44100;
    opts.hopSize = 512;

    QString error;
    QVERIFY(CsvToDsConverter::convertFromMemory(rows, opts, mockF0, nullptr, error));

    QString dsPath = tmpOut.path() + "/song1.ds";
    auto loadResult = DsDocument::loadFile(dsPath);
    QVERIFY(loadResult.ok());
    DsDocument doc = std::move(loadResult.value());
    QVERIFY(!doc.sentences().empty());

    auto &s = doc.sentences()[0];
    QVERIFY(s.contains("ph_seq"));
    QCOMPARE(s["ph_seq"].get<std::string>(), std::string("a b"));
    QVERIFY(s.contains("ph_dur"));
    QVERIFY(s.contains("f0_seq"));
}

void TestCsvToDsConverter::testConvertFromMemory_f0CallbackError() {
    QTemporaryDir tmpWav;
    QVERIFY(tmpWav.isValid());
    QTemporaryDir tmpOut;
    QVERIFY(tmpOut.isValid());

    QFile wav1(tmpWav.path() + "/song1.wav");
    QVERIFY(wav1.open(QIODevice::WriteOnly));
    wav1.close();

    std::vector<TranscriptionRow> rows;
    TranscriptionRow row;
    row.name = "song1";
    row.phSeq = "a";
    row.phDur = "0.1";
    rows.push_back(row);

    auto failingF0 = [](const QString &, std::vector<float> &, QString &err) {
        err = "F0 extraction failed";
        return false;
    };

    CsvToDsConverter::Options opts;
    opts.wavsDir = tmpWav.path();
    opts.outputDir = tmpOut.path();

    QString error;
    QVERIFY(!CsvToDsConverter::convertFromMemory(rows, opts, failingF0, nullptr, error));
    QVERIFY(!error.isEmpty());
}

void TestCsvToDsConverter::testDsToCsv_multiSentence() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    DsDocument doc;
    nlohmann::json s1;
    s1["offset"] = 0.0;
    s1["text"] = "hello";
    s1["ph_seq"] = "h eh l ow";
    s1["ph_dur"] = "0.1 0.2 0.15 0.25";
    s1["ph_num"] = "2 2";
    s1["note_seq"] = "C4 D4";
    s1["note_dur"] = "0.3 0.4";
    doc.sentences().push_back(s1);

    nlohmann::json s2;
    s2["offset"] = 1.0;
    s2["text"] = "world";
    s2["ph_seq"] = "w er l d";
    s2["ph_dur"] = "0.1 0.2 0.15 0.25";
    s2["ph_num"] = "2 2";
    doc.sentences().push_back(s2);

    QString dsPath = tmp.path() + "/multi.ds";
    auto saveResult = doc.saveFile(dsPath);
    QVERIFY(saveResult.ok());

    QFile wavFile(tmp.path() + "/multi.wav");
    QVERIFY(wavFile.open(QIODevice::WriteOnly));
    wavFile.close();

    QString csvPath = tmp.path() + "/output.csv";
    QString error;
    QVERIFY(CsvToDsConverter::dsToCsv(tmp.path(), csvPath, error));

    std::vector<TranscriptionRow> rows;
    QVERIFY(TranscriptionCsv::read(csvPath, rows, error));
    QVERIFY(rows.size() >= 1);
}

void TestCsvToDsConverter::testDsToCsv_noteGlideFill() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    DsDocument doc;
    nlohmann::json s1;
    s1["offset"] = 0.0;
    s1["text"] = "test";
    s1["ph_seq"] = "t eh s t";
    s1["ph_dur"] = "0.1 0.2 0.1 0.2";
    s1["ph_num"] = "2 2";
    s1["note_seq"] = "C4 D4";
    s1["note_dur"] = "0.3 0.3";
    s1["note_glide"] = "none none";
    doc.sentences().push_back(s1);

    QString dsPath = tmp.path() + "/glide.ds";
    auto saveResult = doc.saveFile(dsPath);
    QVERIFY(saveResult.ok());

    QFile wavFile(tmp.path() + "/glide.wav");
    QVERIFY(wavFile.open(QIODevice::WriteOnly));
    wavFile.close();

    QString csvPath = tmp.path() + "/output.csv";
    QString error;
    QVERIFY(CsvToDsConverter::dsToCsv(tmp.path(), csvPath, error));

    std::vector<TranscriptionRow> rows;
    QVERIFY(TranscriptionCsv::read(csvPath, rows, error));
    QVERIFY(!rows.empty());
    QVERIFY(!rows[0].noteGlide.isEmpty());
}

QTEST_GUILESS_MAIN(TestCsvToDsConverter)
#include "TestCsvToDsConverter.moc"
