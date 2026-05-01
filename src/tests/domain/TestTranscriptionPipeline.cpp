#include <QTest>
#include <QTemporaryDir>
#include <dstools/TranscriptionPipeline.h>

using namespace dstools;

class TestTranscriptionPipeline : public QObject {
    Q_OBJECT
private slots:
    void testExtractTextGrids_withMockDeps();
    void testCalculatePhNum_withMockDeps();
    void testGameAlign_callsCallback();
    void testConvertToDs_withMockDeps();
    void testFullPipeline_withAllMocks();
    void testFullPipeline_stepFailure();
    void testProgressCallback();
    void testCalculatePhNum_dictLoadFailure();
    void testGameAlign_nullCallback();
    void testConvertToDs_f0CallbackError();
    void testExtractTextGrids_depFailure();
    void testFullPipeline_checkpointResume();
};

void TestTranscriptionPipeline::testExtractTextGrids_withMockDeps() {
    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [](const QString &, std::vector<TranscriptionRow> &rows, QString &) {
        TranscriptionRow r1;
        r1.name = "test1";
        r1.phSeq = "sh a n";
        r1.phDur = "0.1 0.3 0.2";
        rows.push_back(r1);

        TranscriptionRow r2;
        r2.name = "test2";
        r2.phSeq = "m i";
        r2.phDur = "0.2 0.4";
        rows.push_back(r2);
        return true;
    };

    std::vector<TranscriptionRow> rows;
    QString error;
    QVERIFY(TranscriptionPipeline::extractTextGrids("dummy_dir", rows, error, deps));
    QCOMPARE(rows.size(), size_t(2));
    QCOMPARE(rows[0].name, QString("test1"));
    QCOMPARE(rows[0].phSeq, QString("sh a n"));
    QCOMPARE(rows[1].name, QString("test2"));
    QCOMPARE(rows[1].phSeq, QString("m i"));
}

void TestTranscriptionPipeline::testCalculatePhNum_withMockDeps() {
    TranscriptionPipeline::Deps deps;
    deps.loadDictionary = [](const QString &, QSet<QString> &vowels, QSet<QString> &consonants, QString &) {
        vowels = {"a", "e", "i", "o", "u"};
        consonants = {"sh", "n", "m"};
        return true;
    };

    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "sh a n";
    r.phDur = "0.1 0.3 0.2";
    rows.push_back(r);

    QString error;
    QVERIFY(TranscriptionPipeline::calculatePhNum("dummy_dict.txt", rows, error, deps));
    QVERIFY(!rows[0].phNum.isEmpty());
}

void TestTranscriptionPipeline::testGameAlign_callsCallback() {
    TranscriptionPipeline::GameAlignCallback gameAlignCb =
        [](const QString &, const std::vector<std::string> &,
           const std::vector<float> &, const std::vector<int> &,
           std::vector<std::tuple<std::string, float, int>> &notes, QString &) {
            notes.push_back({"C4", 0.6, 0});
            return true;
        };

    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "sh a n";
    r.phDur = "0.1 0.3 0.2";
    r.phNum = "1 2";
    rows.push_back(r);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.wavsDir = tmp.path();

    QString error;
    QVERIFY(TranscriptionPipeline::gameAlign(opts, gameAlignCb, rows, nullptr, error));
    QVERIFY(!rows[0].noteSeq.isEmpty());
    QVERIFY(!rows[0].noteDur.isEmpty());
    QVERIFY(!rows[0].noteSlur.isEmpty());
}

void TestTranscriptionPipeline::testConvertToDs_withMockDeps() {
    bool convertCalled = false;
    std::vector<TranscriptionRow> capturedRows;

    TranscriptionPipeline::Deps deps;
    deps.convertToDs = [&](const std::vector<TranscriptionRow> &rows,
                           const CsvToDsConverter::Options &,
                           CsvToDsConverter::F0Callback,
                           CsvToDsConverter::ProgressCallback, QString &) {
        convertCalled = true;
        capturedRows = rows;
        return true;
    };

    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "a b";
    r.phDur = "0.1 0.2";
    r.noteSeq = "C4";
    r.noteDur = "0.3";
    r.noteSlur = "0";
    rows.push_back(r);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.wavsDir = tmp.path();
    opts.outputDir = tmp.path();

    QString error;
    QVERIFY(TranscriptionPipeline::convertToDs(opts, nullptr, rows, nullptr, error, deps));
    QVERIFY(convertCalled);
    QCOMPARE(capturedRows.size(), size_t(1));
    QCOMPARE(capturedRows[0].name, QString("test1"));
}

void TestTranscriptionPipeline::testFullPipeline_withAllMocks() {
    int callOrder = 0;
    int extractOrder = 0, loadDictOrder = 0, writeCsvOrder = 0, convertOrder = 0;

    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [&](const QString &, std::vector<TranscriptionRow> &rows, QString &) {
        extractOrder = ++callOrder;
        TranscriptionRow r;
        r.name = "test1";
        r.phSeq = "sh a n";
        r.phDur = "0.1 0.3 0.2";
        rows.push_back(r);
        return true;
    };

    deps.loadDictionary = [&](const QString &, QSet<QString> &vowels, QSet<QString> &consonants, QString &) {
        loadDictOrder = ++callOrder;
        vowels = {"a", "e", "i", "o", "u"};
        consonants = {"sh", "n", "m"};
        return true;
    };

    deps.writeCsv = [&](const QString &, const std::vector<TranscriptionRow> &, QString &) {
        writeCsvOrder = ++callOrder;
        return true;
    };

    deps.readCsv = [](const QString &, std::vector<TranscriptionRow> &, QString &) {
        return true;
    };

    deps.convertToDs = [&](const std::vector<TranscriptionRow> &,
                           const CsvToDsConverter::Options &,
                           CsvToDsConverter::F0Callback,
                           CsvToDsConverter::ProgressCallback, QString &) {
        convertOrder = ++callOrder;
        return true;
    };

    TranscriptionPipeline::GameAlignCallback gameAlignCb =
        [](const QString &, const std::vector<std::string> &,
           const std::vector<float> &, const std::vector<int> &,
           std::vector<std::tuple<std::string, float, int>> &notes, QString &) {
            notes.push_back({"C4", 0.6, 0});
            return true;
        };

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.textGridDir = tmp.path();
    opts.wavsDir = tmp.path();
    opts.dictPath = tmp.path() + "/dict.txt";
    opts.outputDir = tmp.path();
    opts.writeCsv = true;
    opts.csvPath = tmp.path() + "/transcriptions.csv";

    QString error;
    QVERIFY(TranscriptionPipeline::run(opts, gameAlignCb, nullptr, nullptr, deps, error));

    // Verify execution order
    QVERIFY(extractOrder > 0);
    QVERIFY(loadDictOrder > extractOrder);
    QVERIFY(convertOrder > loadDictOrder);
}

void TestTranscriptionPipeline::testFullPipeline_stepFailure() {
    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [](const QString &, std::vector<TranscriptionRow> &, QString &error) {
        error = "TextGrid extraction failed";
        return false;
    };

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.textGridDir = tmp.path();
    opts.wavsDir = tmp.path();
    opts.dictPath = tmp.path() + "/dict.txt";
    opts.outputDir = tmp.path();

    QString error;
    QVERIFY(!TranscriptionPipeline::run(opts, nullptr, nullptr, nullptr, deps, error));
    QVERIFY(error.contains("TextGrid extraction failed"));
}

void TestTranscriptionPipeline::testProgressCallback() {
    std::vector<TranscriptionPipeline::Step> stepsReported;

    TranscriptionPipeline::ProgressCallback progress =
        [&](TranscriptionPipeline::Step step, int, int, const QString &) {
            stepsReported.push_back(step);
        };

    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [](const QString &, std::vector<TranscriptionRow> &rows, QString &) {
        TranscriptionRow r;
        r.name = "test1";
        r.phSeq = "sh a n";
        r.phDur = "0.1 0.3 0.2";
        rows.push_back(r);
        return true;
    };

    deps.loadDictionary = [](const QString &, QSet<QString> &vowels, QSet<QString> &consonants, QString &) {
        vowels = {"a", "e", "i", "o", "u"};
        consonants = {"sh", "n", "m"};
        return true;
    };

    deps.writeCsv = [](const QString &, const std::vector<TranscriptionRow> &, QString &) {
        return true;
    };

    deps.readCsv = [](const QString &, std::vector<TranscriptionRow> &, QString &) {
        return true;
    };

    deps.convertToDs = [](const std::vector<TranscriptionRow> &,
                          const CsvToDsConverter::Options &,
                          CsvToDsConverter::F0Callback,
                          CsvToDsConverter::ProgressCallback, QString &) {
        return true;
    };

    TranscriptionPipeline::GameAlignCallback gameAlignCb =
        [](const QString &, const std::vector<std::string> &,
           const std::vector<float> &, const std::vector<int> &,
           std::vector<std::tuple<std::string, float, int>> &notes, QString &) {
            notes.push_back({"C4", 0.6, 0});
            return true;
        };

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.textGridDir = tmp.path();
    opts.wavsDir = tmp.path();
    opts.dictPath = tmp.path() + "/dict.txt";
    opts.outputDir = tmp.path();
    opts.writeCsv = true;
    opts.csvPath = tmp.path() + "/transcriptions.csv";

    QString error;
    QVERIFY(TranscriptionPipeline::run(opts, gameAlignCb, nullptr, progress, deps, error));

    QVERIFY(!stepsReported.empty());

    // Check that ExtractTextGrid step was reported
    bool hasExtract = false;
    for (auto s : stepsReported) {
        if (s == TranscriptionPipeline::Step::ExtractTextGrid)
            hasExtract = true;
    }
    QVERIFY(hasExtract);
}

void TestTranscriptionPipeline::testCalculatePhNum_dictLoadFailure() {
    TranscriptionPipeline::Deps deps;
    deps.loadDictionary = [](const QString &, QSet<QString> &, QSet<QString> &, QString &error) {
        error = "Dictionary file not found";
        return false;
    };

    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "a b";
    r.phDur = "0.1 0.2";
    rows.push_back(r);

    QString error;
    QVERIFY(!TranscriptionPipeline::calculatePhNum("dummy_dict.txt", rows, error, deps));
    QVERIFY(error.contains("Dictionary file not found"));
}

void TestTranscriptionPipeline::testGameAlign_nullCallback() {
    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "a b";
    r.phDur = "0.1 0.2";
    r.phNum = "2";
    rows.push_back(r);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.wavsDir = tmp.path();

    QString error;
    QVERIFY(TranscriptionPipeline::gameAlign(opts, nullptr, rows, nullptr, error));
    QVERIFY(rows[0].noteSeq.isEmpty());
}

void TestTranscriptionPipeline::testConvertToDs_f0CallbackError() {
    TranscriptionPipeline::Deps deps;
    deps.convertToDs = [](const std::vector<TranscriptionRow> &,
                          const CsvToDsConverter::Options &,
                          CsvToDsConverter::F0Callback f0Cb,
                          CsvToDsConverter::ProgressCallback, QString &error) {
        if (f0Cb) {
            std::vector<float> f0;
            QString f0Error;
            if (!f0Cb("test.wav", f0, f0Error)) {
                error = f0Error;
                return false;
            }
        }
        return true;
    };

    std::vector<TranscriptionRow> rows;
    TranscriptionRow r;
    r.name = "test1";
    r.phSeq = "a";
    r.phDur = "0.1";
    rows.push_back(r);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.wavsDir = tmp.path();
    opts.outputDir = tmp.path();

    auto failingF0 = [](const QString &, std::vector<float> &, QString &err) {
        err = "F0 extraction failed";
        return false;
    };

    QString error;
    QVERIFY(!TranscriptionPipeline::convertToDs(opts, failingF0, rows, nullptr, error, deps));
}

void TestTranscriptionPipeline::testExtractTextGrids_depFailure() {
    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [](const QString &, std::vector<TranscriptionRow> &, QString &error) {
        error = "TextGrid extraction dependency failed";
        return false;
    };

    std::vector<TranscriptionRow> rows;
    QString error;
    QVERIFY(!TranscriptionPipeline::extractTextGrids("dummy_dir", rows, error, deps));
    QVERIFY(error.contains("TextGrid extraction dependency failed"));
}

void TestTranscriptionPipeline::testFullPipeline_checkpointResume() {
    TranscriptionPipeline::Deps deps;
    deps.extractTextGrids = [](const QString &, std::vector<TranscriptionRow> &, QString &) {
        return true;
    };

    deps.loadDictionary = [](const QString &, QSet<QString> &vowels, QSet<QString> &consonants, QString &) {
        vowels = {"a", "e", "i"};
        consonants = {"sh", "n"};
        return true;
    };

    deps.readCsv = [](const QString &, std::vector<TranscriptionRow> &rows, QString &) {
        TranscriptionRow r;
        r.name = "checkpoint";
        r.phSeq = "a n";
        r.phDur = "0.1 0.2";
        r.phNum = "1 1";
        r.noteSeq = "C4 D4";
        r.noteDur = "0.1 0.2";
        r.noteSlur = "0 0";
        rows.push_back(r);
        return true;
    };

    deps.writeCsv = [](const QString &, const std::vector<TranscriptionRow> &, QString &) {
        return true;
    };

    deps.convertToDs = [](const std::vector<TranscriptionRow> &,
                          const CsvToDsConverter::Options &,
                          CsvToDsConverter::F0Callback,
                          CsvToDsConverter::ProgressCallback, QString &) {
        return true;
    };

    TranscriptionPipeline::GameAlignCallback gameAlignCb =
        [](const QString &, const std::vector<std::string> &,
           const std::vector<float> &, const std::vector<int> &,
           std::vector<std::tuple<std::string, float, int>> &notes, QString &) {
            notes.push_back({"C4", 0.3, 0});
            return true;
        };

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    TranscriptionPipeline::Options opts;
    opts.textGridDir = tmp.path();
    opts.wavsDir = tmp.path();
    opts.dictPath = tmp.path() + "/dict.txt";
    opts.outputDir = tmp.path();
    opts.writeCsv = true;
    opts.csvPath = tmp.path() + "/transcriptions.csv";

    QString error;
    QVERIFY(TranscriptionPipeline::run(opts, gameAlignCb, nullptr, nullptr, deps, error));
}

QTEST_GUILESS_MAIN(TestTranscriptionPipeline)
#include "TestTranscriptionPipeline.moc"
