#include <QTest>
#include "CompositeInferenceService.h"

using namespace dstools;

class TestCompositeInferenceService : public QObject {
    Q_OBJECT

private slots:
    void testNullEnginesHasNoCapabilities() {
        CompositeInferenceService service(nullptr, nullptr, nullptr);
        QVERIFY(!service.hasForcedAlignment());
        QVERIFY(!service.hasPitchExtraction());
        QVERIFY(!service.hasMidiTranscription());
    }

    void testNullEnginesRunForcedAlignmentFails() {
        CompositeInferenceService service(nullptr, nullptr, nullptr);
        std::vector<dsfw::infer::AlignedWord> words;
        bool ok = service.runForcedAlignment(std::filesystem::path(), "zh", {}, "", words);
        QVERIFY(!ok);
    }

    void testNullEnginesRunPitchExtractionFails() {
        CompositeInferenceService service(nullptr, nullptr, nullptr);
        dsfw::infer::PitchResult result;
        bool ok = service.runPitchExtraction(std::filesystem::path(), 0.01f, result);
        QVERIFY(!ok);
    }

    void testNullEnginesRunMidiTranscriptionFails() {
        CompositeInferenceService service(nullptr, nullptr, nullptr);
        std::vector<dsfw::infer::MidiNote> notes;
        bool ok = service.runMidiTranscription(std::filesystem::path(), notes);
        QVERIFY(!ok);
    }
};

QTEST_MAIN(TestCompositeInferenceService)
#include "TestCompositeInferenceService.moc"
