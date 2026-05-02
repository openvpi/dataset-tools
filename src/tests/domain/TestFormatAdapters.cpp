#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "../../domain/src/adapters/LabAdapter.h"
#include "../../domain/src/adapters/CsvAdapter.h"
#include "../../domain/src/adapters/DsFileAdapter.h"

#include <nlohmann/json.hpp>

using namespace dstools;

class TestFormatAdapters : public QObject {
    Q_OBJECT

private slots:
    void lab_roundtrip();
    void csv_import();
    void ds_import();
};

void TestFormatAdapters::lab_roundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString labPath = tmpDir.filePath("test.lab");

    // Write a lab file
    {
        QFile f(labPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "gan shou ting zai";
    }

    // Import
    LabAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("lab"));
    QVERIFY(adapter.canImport());
    QVERIFY(adapter.canExport());

    std::map<QString, nlohmann::json> layers;
    ProcessorConfig config;
    auto result = adapter.importToLayers(labPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("grapheme")) == 1);
    const auto &boundaries = layers["grapheme"]["boundaries"];
    QVERIFY(boundaries.size() == 5); // 4 syllables + 1 final
    QVERIFY(boundaries[0]["text"] == "gan");
    QVERIFY(boundaries[1]["text"] == "shou");
    QVERIFY(boundaries[2]["text"] == "ting");
    QVERIFY(boundaries[3]["text"] == "zai");
    QVERIFY(boundaries[4]["text"] == "");

    // Export
    const QString outPath = tmpDir.filePath("out.lab");
    auto exportResult = adapter.exportFromLayers(layers, outPath, config);
    QVERIFY(exportResult.ok());

    // Verify exported content
    QFile outFile(outPath);
    QVERIFY(outFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QTextStream(&outFile).readAll().trimmed();
    QCOMPARE(content, QStringLiteral("gan shou ting zai"));
}

void TestFormatAdapters::csv_import() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString csvPath = tmpDir.filePath("transcriptions.csv");

    // Write a minimal CSV
    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "name,ph_seq,ph_dur\n";
        out << "item1,a b c,0.1 0.2 0.3\n";
    }

    CsvAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("csv"));

    std::map<QString, nlohmann::json> layers;
    ProcessorConfig config;
    auto result = adapter.importToLayers(csvPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("phoneme")) == 1);
    const auto &boundaries = layers["phoneme"]["boundaries"];
    QVERIFY(boundaries.size() == 4); // 3 phones + 1 final

    QVERIFY(boundaries[0]["text"] == "a");
    QVERIFY(boundaries[0]["pos"] == 0);
    QVERIFY(boundaries[1]["text"] == "b");
    // pos of b = secToUs(0.1) = 100000
    QVERIFY(boundaries[1]["pos"] == 100000);
    QVERIFY(boundaries[2]["text"] == "c");
    // pos of c = secToUs(0.1 + 0.2) = 300000
    QVERIFY(boundaries[2]["pos"] == 300000);
    // final pos = secToUs(0.1 + 0.2 + 0.3) = 600000
    QVERIFY(boundaries[3]["pos"] == 600000);
    QVERIFY(boundaries[3]["text"] == "");
}

void TestFormatAdapters::ds_import() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString dsPath = tmpDir.filePath("test.ds");

    // Write a minimal DS JSON file
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

    DsFileAdapter adapter;
    QCOMPARE(adapter.formatId(), QStringLiteral("ds"));

    std::map<QString, nlohmann::json> layers;
    ProcessorConfig config;
    auto result = adapter.importToLayers(dsPath, layers, config);
    QVERIFY(result.ok());

    QVERIFY(layers.count(QStringLiteral("phoneme")) == 1);
    const auto &boundaries = layers["phoneme"]["boundaries"];
    QVERIFY(boundaries.size() == 5); // 4 phones + 1 final

    QVERIFY(boundaries[0]["text"] == "x");
    QVERIFY(boundaries[0]["pos"] == 0);
    QVERIFY(boundaries[1]["text"] == "a");
    QVERIFY(boundaries[1]["pos"] == 100000); // secToUs(0.1)
    QVERIFY(boundaries[2]["text"] == "y");
    QVERIFY(boundaries[2]["pos"] == 300000); // secToUs(0.1 + 0.2)
    QVERIFY(boundaries[3]["text"] == "b");
    QVERIFY(boundaries[3]["pos"] == 450000); // secToUs(0.1 + 0.2 + 0.15)
    QVERIFY(boundaries[4]["text"] == "");
    QVERIFY(boundaries[4]["pos"] == 700000); // secToUs(0.1 + 0.2 + 0.15 + 0.25)
}

QTEST_MAIN(TestFormatAdapters)
#include "TestFormatAdapters.moc"
