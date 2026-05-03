#include <QtTest/QtTest>

#include <PitchExtractionService.h>
#include <game-infer/Game.h>
#include <dstools/DsTextTypes.h>
#include <dstools/TimePos.h>

class TestPitchExtractionService : public QObject {
    Q_OBJECT

private slots:
    void testApplyPitchToDocumentAddsNewCurve() {
        dstools::DsTextDocument doc;
        QVERIFY(doc.curves.empty());

        std::vector<int32_t> f0 = {440000, 0, 450000};
        dstools::PitchExtractionService::applyPitchToDocument(doc, f0, 0.005f);

        QCOMPARE(doc.curves.size(), 1u);
        QCOMPARE(doc.curves[0].name, QStringLiteral("pitch"));
        QCOMPARE(doc.curves[0].values.size(), 3u);
        QCOMPARE(doc.curves[0].values[0], 440000);
        QCOMPARE(doc.curves[0].values[1], 0);
        QCOMPARE(doc.curves[0].values[2], 450000);
        QCOMPARE(doc.curves[0].timestep, dstools::secToUs(0.005));
    }

    void testApplyPitchToDocumentReplacesExisting() {
        dstools::DsTextDocument doc;
        dstools::CurveLayer existing;
        existing.name = QStringLiteral("pitch");
        existing.values = {100, 200};
        existing.timestep = 0.01f;
        doc.curves.push_back(existing);

        std::vector<int32_t> f0 = {440000, 450000, 460000};
        dstools::PitchExtractionService::applyPitchToDocument(doc, f0, 0.005f);

        QCOMPARE(doc.curves.size(), 1u);
        QCOMPARE(doc.curves[0].values.size(), 3u);
        QCOMPARE(doc.curves[0].values[0], 440000);
        QCOMPARE(doc.curves[0].timestep, dstools::secToUs(0.005));
    }

    void testApplyPitchToDocumentEmptyF0() {
        dstools::DsTextDocument doc;
        std::vector<int32_t> f0;
        dstools::PitchExtractionService::applyPitchToDocument(doc, f0, 0.005f);
        QVERIFY(doc.curves.empty());
    }

    void testApplyMidiToDocumentAddsNewLayer() {
        dstools::DsTextDocument doc;
        QVERIFY(doc.layers.empty());

        std::vector<Game::GameNote> notes;
        Game::GameNote n1;
        n1.onset = 0.5;
        n1.pitch = 60;
        notes.push_back(n1);

        Game::GameNote n2;
        n2.onset = 1.0;
        n2.pitch = 64;
        notes.push_back(n2);

        dstools::PitchExtractionService::applyMidiToDocument(doc, notes);

        bool found = false;
        for (const auto &layer : doc.layers) {
            if (layer.name == QStringLiteral("midi")) {
                found = true;
                QCOMPARE(layer.type, QStringLiteral("note"));
                QCOMPARE(layer.boundaries.size(), 2u);
                QCOMPARE(layer.boundaries[0].text, QStringLiteral("C4"));
                QCOMPARE(layer.boundaries[1].text, QStringLiteral("E4"));
                break;
            }
        }
        QVERIFY(found);
    }

    void testApplyMidiToDocumentEmptyNotes() {
        dstools::DsTextDocument doc;
        std::vector<Game::GameNote> notes;
        dstools::PitchExtractionService::applyMidiToDocument(doc, notes);
        QVERIFY(doc.layers.empty());
    }

    void testExtractPitchNullRmvpe() {
        auto result = dstools::PitchExtractionService::extractPitch(nullptr, QStringLiteral("/tmp/test.wav"));
        QVERIFY(result.f0Mhz.empty());
    }

    void testExtractPitchEmptyPath() {
        auto result = dstools::PitchExtractionService::extractPitch(reinterpret_cast<Rmvpe::Rmvpe *>(1), QString());
        QVERIFY(result.f0Mhz.empty());
    }

    void testTranscribeMidiNullGame() {
        auto result = dstools::PitchExtractionService::transcribeMidi(nullptr, QStringLiteral("/tmp/test.wav"));
        QVERIFY(result.notes.empty());
    }

    void testTranscribeMidiEmptyPath() {
        auto result = dstools::PitchExtractionService::transcribeMidi(reinterpret_cast<Game::Game *>(1), QString());
        QVERIFY(result.notes.empty());
    }
};

QTEST_MAIN(TestPitchExtractionService)
#include "TestPitchExtractionService.moc"
