#include "../../domain/src/adapters/DsFileAdapter.h"
#include "../../domain/src/adapters/LabAdapter.h"
#include "dstools/CsvAdapter.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <nlohmann/json.hpp>

using namespace dstools;

class TestFormatAdapters : public QObject {
    Q_OBJECT

private slots:
    void lab_roundtrip();
    void csv_import();
    void ds_import();
    void lab_empty_file();
    void csv_empty_file();
    void csv_missing_columns();
    void csv_invalid_duration();
    void lab_nonexistent_file();
    void csv_nonexistent_file();
    void lab_single_word();
    void csv_single_phoneme();
};

void TestFormatAdapters::lab_roundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString labPath = tmpDir.filePath("test.lab");

    {
        QFile f(labPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "gan shou ting zai";
    }

    LabAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("lab"));
    QVERIFY(adapter.canImport());
    QVERIFY(adapter.canExport());

    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(labPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("grapheme")) == 1);
    const auto graphemeJson = layers["grapheme"].toJson();
    const auto& boundaries = graphemeJson["boundaries"];
    QVERIFY(boundaries.size() == 5);
    QVERIFY(boundaries[0]["text"] == "gan");
    QVERIFY(boundaries[1]["text"] == "shou");
    QVERIFY(boundaries[2]["text"] == "ting");
    QVERIFY(boundaries[3]["text"] == "zai");
    QVERIFY(boundaries[4]["text"] == "");

    const QString outPath = tmpDir.filePath("out.lab");
    auto exportResult = adapter.exportFromLayers(layers, outPath, config);
    QVERIFY(exportResult.ok());

    QFile outFile(outPath);
    QVERIFY(outFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QTextStream(&outFile).readAll().trimmed();
    QCOMPARE(content, QStringLiteral("gan shou ting zai"));
}

void TestFormatAdapters::csv_import() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("transcriptions.csv");

    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,ph_seq,ph_dur\n";
        out << "item1,a b c,0.1 0.2 0.3\n";
    }

    dsfw::CsvAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("csv"));

    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("phoneme")) == 1);
    const auto phonemeJson = layers["phoneme"].toJson();
    const auto& boundaries = phonemeJson["boundaries"];
    QVERIFY(boundaries.size() == 4);

    QVERIFY(boundaries[0]["text"] == "a");
    QVERIFY(boundaries[0]["pos"] == 0);
    QVERIFY(boundaries[1]["text"] == "b");
    QVERIFY(boundaries[1]["pos"] == 100000);
    QVERIFY(boundaries[2]["text"] == "c");
    QVERIFY(boundaries[2]["pos"] == 300000);
    QVERIFY(boundaries[3]["pos"] == 600000);
    QVERIFY(boundaries[3]["text"] == "");
}

void TestFormatAdapters::ds_import() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString dsPath = tmpDir.filePath("test.ds");

    {
        nlohmann::json doc = nlohmann::json::array();
        nlohmann::json sentence;
        sentence["offset"] = 0.0;
        sentence["ph_seq"] = "x a y b";
        sentence["ph_dur"] = "0.1 0.2 0.15 0.25";
        doc.push_back(sentence);

        QFile f(dsPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << QString::fromStdString(doc.dump());
    }

    dsfw::DsFileAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("ds"));

    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(dsPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("phoneme")) == 1);
    const auto phonemeJson = layers["phoneme"].toJson();
    const auto& boundaries = phonemeJson["boundaries"];
    QVERIFY(boundaries.size() == 5);

    QVERIFY(boundaries[0]["text"] == "x");
    QVERIFY(boundaries[0]["pos"] == 0);
    QVERIFY(boundaries[1]["text"] == "a");
    QVERIFY(boundaries[1]["pos"] == 100000);
    QVERIFY(boundaries[2]["text"] == "y");
    QVERIFY(boundaries[2]["pos"] == 300000);
    QVERIFY(boundaries[3]["text"] == "b");
    QVERIFY(boundaries[3]["pos"] == 450000);
    QVERIFY(boundaries[4]["text"] == "");
    QVERIFY(boundaries[4]["pos"] == 700000);
}

QTEST_MAIN(TestFormatAdapters)

void TestFormatAdapters::lab_empty_file() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString labPath = tmpDir.filePath("empty.lab");
    {
        QFile f(labPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    }

    LabAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(labPath, layers, config);
    QVERIFY(result.ok());
    QVERIFY(layers.count(QStringLiteral("grapheme")) == 1);
    const auto graphemeJson = layers["grapheme"].toJson();
    const auto& boundaries = graphemeJson["boundaries"];
    QVERIFY(boundaries.size() == 1);
    QVERIFY(boundaries[0]["text"] == "");
}

void TestFormatAdapters::csv_empty_file() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("empty.csv");
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,ph_seq,ph_dur\n";
    }

    dsfw::CsvAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(result.ok());
}

void TestFormatAdapters::csv_missing_columns() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("missing_cols.csv");
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,extra\n";
        out << "item1,value\n";
    }

    dsfw::CsvAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(!result.ok());
}

void TestFormatAdapters::csv_invalid_duration() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("invalid_dur.csv");
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,ph_seq,ph_dur\n";
        out << "item1,a b,0.1 invalid\n";
    }

    dsfw::CsvAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(!result.ok());
}

void TestFormatAdapters::lab_nonexistent_file() {
    LabAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(QStringLiteral("/nonexistent/file.lab"), layers, config);
    QVERIFY(!result.ok());
}

void TestFormatAdapters::csv_nonexistent_file() {
    dsfw::CsvAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(QStringLiteral("/nonexistent/file.csv"), layers, config);
    QVERIFY(!result.ok());
}

void TestFormatAdapters::lab_single_word() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString labPath = tmpDir.filePath("single.lab");
    {
        QFile f(labPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "hello";
    }

    LabAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(labPath, layers, config);
    QVERIFY(result.ok());
    QVERIFY(layers.count(QStringLiteral("grapheme")) == 1);
    const auto graphemeJson = layers["grapheme"].toJson();
    const auto& boundaries = graphemeJson["boundaries"];
    QVERIFY(boundaries.size() == 2);
    QVERIFY(boundaries[0]["text"] == "hello");
    QVERIFY(boundaries[1]["text"] == "");
}

void TestFormatAdapters::csv_single_phoneme() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("single.csv");
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,ph_seq,ph_dur\n";
        out << "item1,a,0.5\n";
    }

    dsfw::CsvAdapter adapter;
    std::map<QString, LayerData> layers;
    dsfw::ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(result.ok());
    QVERIFY(layers.count(QStringLiteral("phoneme")) == 1);
    const auto phonemeJson = layers["phoneme"].toJson();
    const auto& boundaries = phonemeJson["boundaries"];
    QVERIFY(boundaries.size() == 2);
    QVERIFY(boundaries[0]["text"] == "a");
    QVERIFY(boundaries[0]["pos"] == 0);
    QVERIFY(boundaries[1]["text"] == "");
    QVERIFY(boundaries[1]["pos"] == 500000);
}

#include "TestFormatAdapters.moc"
