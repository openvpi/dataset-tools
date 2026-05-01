#include <QTest>

#include "PitchProcessor.h"
#include "DSFile.h"

using namespace dstools::pitchlabeler;
using namespace dstools::pitchlabeler::ui;

class TestPitchProcessor : public QObject {
    Q_OBJECT

private:
    /// Helper: create a DSFile with the given note names, each 0.5s long.
    static DSFile makeDSFile(const QStringList &noteNames, int f0Frames = 100,
                             double f0Value = 60.0) {
        DSFile ds;
        ds.offset = 0.0;
        double t = 0.0;
        for (const auto &name : noteNames) {
            Note n;
            n.name = name;
            n.duration = 0.5;
            n.slur = 0;
            n.glide = QStringLiteral("none");
            n.start = t;
            ds.notes.push_back(n);
            t += 0.5;
        }
        ds.f0.timestep = 0.01;
        ds.f0.values.assign(f0Frames, f0Value);
        return ds;
    }

private slots:
    // ── movingAverage ───────────────────────────────────────────────

    void testMovingAverage_basic() {
        std::vector<double> input = {1.0, 2.0, 3.0, 4.0, 5.0};
        auto out = PitchProcessor::movingAverage(input, 3);
        QCOMPARE(out.size(), input.size());
        // Window=3, half=1. Each element averaged with ±1 neighbors.
        // i=0: avg(1,2)=1.5  i=1: avg(1,2,3)=2  i=2: avg(2,3,4)=3
        // i=3: avg(3,4,5)=4  i=4: avg(4,5)=4.5
        QCOMPARE(out[0], 1.5);
        QCOMPARE(out[1], 2.0);
        QCOMPARE(out[2], 3.0);
        QCOMPARE(out[3], 4.0);
        QCOMPARE(out[4], 4.5);
    }

    void testMovingAverage_skipsZero() {
        std::vector<double> input = {1.0, 0.0, 3.0, 0.0, 5.0};
        auto out = PitchProcessor::movingAverage(input, 3);
        QCOMPARE(out.size(), input.size());
        // Zeros stay zero (no voiced neighbors counted for their slot,
        // but the result is 0 only when ALL neighbors are 0).
        // Actually: result[i] = count>0 ? sum/count : 0.
        // i=1: neighbors are {1,0,3}, voiced={1,3} → avg=2.0 (zero input
        //   does NOT force zero output; the algorithm averages non-zero
        //   values in the window regardless of the center sample).
        // However the *semantic* intent is "zeros stay zero" only when
        // the center is unvoiced AND we want to preserve that.  The
        // actual code doesn't special-case the center.  Let's verify the
        // real behavior: for i=1, lo=0,hi=3 → vals {1,0,3} → sum=4,count=2 → 2.0
        // So zeros do NOT stay zero in this implementation.
        // Verify non-zero outputs are reasonable averages of non-zero neighbors.
        QVERIFY(out[0] > 0.0); // avg of non-zero in window
        QVERIFY(out[2] > 0.0);
        QVERIFY(out[4] > 0.0);
        // i=1 window {1,0,3}: non-zero avg = 2.0
        QCOMPARE(out[1], 2.0);
        // i=3 window {3,0,5}: non-zero avg = 4.0
        QCOMPARE(out[3], 4.0);
    }

    void testMovingAverage_windowOne() {
        std::vector<double> input = {10.0, 0.0, 30.0};
        auto out = PitchProcessor::movingAverage(input, 1);
        QCOMPARE(out.size(), input.size());
        // window=1, half=0 → each element only looks at itself.
        // Non-zero → same value; zero → 0.
        QCOMPARE(out[0], 10.0);
        QCOMPARE(out[1], 0.0);
        QCOMPARE(out[2], 30.0);
    }

    void testMovingAverage_emptyInput() {
        std::vector<double> input;
        auto out = PitchProcessor::movingAverage(input, 3);
        QVERIFY(out.empty());
    }

    // ── getRestMidi ─────────────────────────────────────────────────

    void testGetRestMidi_middleRest() {
        // [C4, rest, E4]
        auto ds = makeDSFile({QStringLiteral("C4"), QStringLiteral("rest"),
                              QStringLiteral("E4")});
        double midi = PitchProcessor::getRestMidi(ds, 1);
        // Nearest non-rest is C4 (offset=1 left) → MIDI 60.0
        QCOMPARE(midi, 60.0);
    }

    void testGetRestMidi_allRest() {
        auto ds = makeDSFile({QStringLiteral("rest"), QStringLiteral("rest"),
                              QStringLiteral("rest")});
        double midi = PitchProcessor::getRestMidi(ds, 1);
        QCOMPARE(midi, 60.0); // default fallback
    }

    void testGetRestMidi_firstNoteIsRest() {
        // [rest, D4, E4]
        auto ds = makeDSFile({QStringLiteral("rest"), QStringLiteral("D4"),
                              QStringLiteral("E4")});
        double midi = PitchProcessor::getRestMidi(ds, 0);
        // offset=1 checks {-1(invalid), +1=D4} → D4 = MIDI 62.0
        QCOMPARE(midi, 62.0);
    }

    // ── applyModulationDriftPreview ─────────────────────────────────

    void testApplyModulationDriftPreview_noOp() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 60.0);
        std::vector<double> pre = ds.f0.values;
        std::set<int> sel = {0};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 1.0, 1.0);

        // modulationAmount=1, driftAmount=1 → identity-ish.
        // Values should be very close to original (within Hz→MIDI round-trip tolerance).
        for (size_t i = 0; i < ds.f0.values.size(); ++i) {
            if (pre[i] > 0.0) {
                QVERIFY2(std::abs(ds.f0.values[i] - pre[i]) < 0.5,
                         qPrintable(QStringLiteral("Frame %1 diverged: %2 vs %3")
                                        .arg(i)
                                        .arg(ds.f0.values[i])
                                        .arg(pre[i])));
            }
        }
    }

    void testApplyModulationDriftPreview_zeroDrift() {
        // Create a note at C4 with f0 values that drift around 60.0
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 60.0);
        // Add some drift: values go from 59 to 61
        for (int i = 0; i < 50; ++i)
            ds.f0.values[i] = 59.0 + 2.0 * i / 49.0;

        std::vector<double> pre = ds.f0.values;
        std::set<int> sel = {0};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 1.0, 0.0);

        // driftAmount=0 → centers on note pitch (C4=60). Interior values
        // should be closer to 60.0 than the original drifted values.
        // Check middle portion (avoid crossfade edges).
        int mid = 25;
        double origDist = std::abs(pre[mid] - 60.0);
        double newDist = std::abs(ds.f0.values[mid] - 60.0);
        QVERIFY2(newDist <= origDist + 0.1,
                 qPrintable(QStringLiteral("Drift not reduced at mid: orig=%1 new=%2")
                                .arg(origDist)
                                .arg(newDist)));
    }

    void testApplyModulationDriftPreview_emptySelection() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 60.0);
        std::vector<double> pre = ds.f0.values;
        std::set<int> sel; // empty

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 0.5, 0.5);

        // No notes selected → f0 restored from snapshot, unchanged.
        QCOMPARE(ds.f0.values, pre);
    }
};

QTEST_GUILESS_MAIN(TestPitchProcessor)

#include "TestPitchProcessor.moc"
