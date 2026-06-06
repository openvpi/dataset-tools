#include <QTest>
#include <QTemporaryDir>
#include <dsfw/PathUtils.h>
#include <dstools/DsKeys.h>
#include <dstools/DsTextTypes.h>
#include <dsfw/TimePos.h>
#include "DsTextDocBridge.h"

using namespace dstools;

class TestDsTextDocBridgeRoundtrip : public QObject {
    Q_OBJECT
private slots:
    void layer_roundtrip();
    void pitch_doc_roundtrip();
};

void TestDsTextDocBridgeRoundtrip::layer_roundtrip() {
    DsTextDocument original;
    original.audio.path = "test.wav";

    IntervalLayer grapheme;
    grapheme.name = QString::fromUtf8(dstools::keys::layers::grapheme);
    grapheme.type = QStringLiteral("grapheme");
    grapheme.boundaries = {{1, 0, "hello"}, {2, 500000, "world"}, {3, 1000000, ""}};
    original.layers.push_back(grapheme);

    IntervalLayer phoneme;
    phoneme.name = QString::fromUtf8(dstools::keys::layers::phoneme);
    phoneme.type = QStringLiteral("phoneme");
    phoneme.boundaries = {{1, 0, "hh"},     {2, 200000, "ah"}, {3, 400000, "l"}, {4, 500000, "ow"},
                          {5, 800000, "w"}, {6, 900000, "er"}, {7, 1000000, ""}};
    original.layers.push_back(phoneme);

    // Save → reload to simulate roundtrip
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString path = tmpDir.filePath("roundtrip.dstext");
    auto saveRes = original.save(path);
    QVERIFY(saveRes.ok());

    auto loadRes = DsTextDocument::load(path);
    QVERIFY(loadRes.ok());
    const auto& restored = loadRes.value();

    auto verifyRes = DsTextDocBridge::verifyLayerRoundtrip(original, restored);
    QVERIFY(verifyRes.ok());

    // Extract layers and verify count
    auto extracted = DsTextDocBridge::extractIntervalLayers(restored);
    QCOMPARE(extracted.size(), original.layers.size());

    // Roundtrip verification with modified document should fail
    DsTextDocument modified = restored;
    modified.layers[0].boundaries[0].pos = 999;
    auto verifyFail = DsTextDocBridge::verifyLayerRoundtrip(original, modified);
    QVERIFY(!verifyFail.ok());
}

void TestDsTextDocBridgeRoundtrip::pitch_doc_roundtrip() {
    DsTextDocument original;
    original.audio.path = "test.wav";

    IntervalLayer midi;
    midi.name = QString::fromUtf8(dstools::keys::layers::midi);
    midi.type = QStringLiteral("midi");
    midi.boundaries = {{1, 0, "C4"}, {2, 500000, ""}, {3, 500000, "D4"}, {4, 1000000, ""}};
    original.layers.push_back(midi);

    CurveLayer pitch;
    pitch.name = QString::fromUtf8(dstools::keys::layers::pitch);
    pitch.timestep = 5000;
    pitch.values = {261.6, 262.0, 0.0, 293.7, 295.0, 0.0};
    original.curves.push_back(pitch);

    // Extract IntervalLayers from DsTextDocument
    auto extracted = DsTextDocBridge::extractIntervalLayers(original);

    // Merge back into a clean document
    DsTextDocument restored;
    DsTextDocBridge::mergeIntervalLayers(restored, extracted);

    // Convert to DsPitchDocument and back
    auto pitchDoc = DsTextDocBridge::toPitchDoc(original, TimePos(2000000));
    QVERIFY(pitchDoc != nullptr);
    QCOMPARE(pitchDoc->f0.values.size(), original.curves[0].values.size());
    QCOMPARE(pitchDoc->notes.size(), size_t(2));

    DsTextDocument roundtripped;
    roundtripped.curves.push_back(pitch);
    auto mergeResult = DsTextDocBridge::fromPitchDoc(roundtripped, *pitchDoc);
    QVERIFY(mergeResult.ok());

    // Note: DsPitchDocument doesn't carry timestep back, so only checks notes
    for (auto& layer : roundtripped.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
            QCOMPARE(layer.boundaries.size(), size_t(4));
            break;
        }
    }

    // Roundtrip verify should succeed for the PitchDoc level
    auto verifyRes = DsTextDocBridge::verifyPitchDocRoundtrip(original, *pitchDoc);
    QVERIFY(verifyRes.ok());
}

QTEST_MAIN(TestDsTextDocBridgeRoundtrip)
#include "TestDsTextDocBridgeRoundtrip.moc"