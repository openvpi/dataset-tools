#include <QTest>

#include "PitchProcessor.h"
#include "DSFile.h"

#include <dstools/CurveTools.h>
#include <dstools/TimePos.h>

using namespace dstools::pitchlabeler;
using namespace dstools::pitchlabeler::ui;
using dstools::secToUs;
using dstools::usToSec;
using dstools::usToMs;
using dstools::hzToMhz;
using dstools::mhzToHz;

class TestPitchProcessor : public QObject {
    Q_OBJECT

private:
    static DSFile makeDSFile(const QStringList &noteNames, int f0Frames = 100,
                             double f0HzValue = 261.6) {
        DSFile ds;
        ds.offset = 0;
        TimePos t = 0;
        for (const auto &name : noteNames) {
            Note n;
            n.name = name;
            n.duration = secToUs(0.5);
            n.slur = 0;
            n.glide = QStringLiteral("none");
            n.start = t;
            ds.notes.push_back(n);
            t += secToUs(0.5);
        }
        ds.f0.timestep = secToUs(0.01);
        ds.f0.values.assign(f0Frames, hzToMhz(f0HzValue));
        return ds;
    }

private slots:
    void testMovingAverage_basic() {
        std::vector<double> input = {1.0, 2.0, 3.0, 4.0, 5.0};
        auto out = PitchProcessor::movingAverage(input, 3);
        QCOMPARE(out.size(), input.size());
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
        QVERIFY(out[0] > 0.0);
        QVERIFY(out[2] > 0.0);
        QVERIFY(out[4] > 0.0);
        QCOMPARE(out[1], 0.0);
        QCOMPARE(out[3], 0.0);
    }

    void testMovingAverage_windowOne() {
        std::vector<double> input = {10.0, 0.0, 30.0};
        auto out = PitchProcessor::movingAverage(input, 1);
        QCOMPARE(out.size(), input.size());
        QCOMPARE(out[0], 10.0);
        QCOMPARE(out[1], 0.0);
        QCOMPARE(out[2], 30.0);
    }

    void testMovingAverage_emptyInput() {
        std::vector<double> input;
        auto out = PitchProcessor::movingAverage(input, 3);
        QVERIFY(out.empty());
    }

    void testGetRestMidi_middleRest() {
        auto ds = makeDSFile({QStringLiteral("C4"), QStringLiteral("rest"),
                              QStringLiteral("E4")});
        double midi = PitchProcessor::getRestMidi(ds, 1);
        QCOMPARE(midi, 60.0);
    }

    void testGetRestMidi_allRest() {
        auto ds = makeDSFile({QStringLiteral("rest"), QStringLiteral("rest"),
                              QStringLiteral("rest")});
        double midi = PitchProcessor::getRestMidi(ds, 1);
        QCOMPARE(midi, 60.0);
    }

    void testGetRestMidi_firstNoteIsRest() {
        auto ds = makeDSFile({QStringLiteral("rest"), QStringLiteral("D4"),
                              QStringLiteral("E4")});
        double midi = PitchProcessor::getRestMidi(ds, 0);
        QCOMPARE(midi, 62.0);
    }

    void testApplyModulationDriftPreview_noOp() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 261.6);
        std::vector<int32_t> pre = ds.f0.values;
        std::set<int> sel = {0};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 1.0, 1.0);

        for (size_t i = 0; i < ds.f0.values.size(); ++i) {
            if (pre[i] > 0) {
                QVERIFY2(std::abs(ds.f0.values[i] - pre[i]) < 500,
                         qPrintable(QStringLiteral("Frame %1 diverged: %2 vs %3")
                                        .arg(i)
                                        .arg(ds.f0.values[i])
                                        .arg(pre[i])));
            }
        }
    }

    void testApplyModulationDriftPreview_zeroDrift() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 261.6);
        for (int i = 0; i < 50; ++i)
            ds.f0.values[i] = hzToMhz(250.0 + 30.0 * i / 49.0);

        std::vector<int32_t> pre = ds.f0.values;
        std::set<int> sel = {0};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 1.0, 0.0);

        int mid = 25;
        double targetHz = midiToFreq(60.0);
        double origDist = std::abs(mhzToHz(pre[mid]) - targetHz);
        double newDist = std::abs(mhzToHz(ds.f0.values[mid]) - targetHz);
        QVERIFY2(newDist <= origDist + 1.0,
                 qPrintable(QStringLiteral("Drift not reduced at mid: orig=%1 new=%2")
                                .arg(origDist)
                                .arg(newDist)));
    }

    void testApplyModulationDriftPreview_emptySelection() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 261.6);
        std::vector<int32_t> pre = ds.f0.values;
        std::set<int> sel;

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 0.5, 0.5);

        QCOMPARE(ds.f0.values, pre);
    }

    void testMovingAverage_allZeros() {
        std::vector<double> input(10, 0.0);
        auto out = PitchProcessor::movingAverage(input, 3);
        QCOMPARE(out.size(), input.size());
        for (auto v : out)
            QCOMPARE(v, 0.0);
    }

    void testMovingAverage_singleElement() {
        std::vector<double> input = {42.0};
        auto out = PitchProcessor::movingAverage(input, 3);
        QCOMPARE(out.size(), size_t(1));
        QCOMPARE(out[0], 42.0);
    }

    void testGetRestMidi_lastNoteIsRest() {
        auto ds = makeDSFile({QStringLiteral("C4"), QStringLiteral("E4"),
                              QStringLiteral("rest")});
        double midi = PitchProcessor::getRestMidi(ds, 2);
        QCOMPARE(midi, 64.0);
    }

    void testApplyModulationDriftPreview_modulationOnly() {
        auto ds = makeDSFile({QStringLiteral("C4")}, 50, 261.6);
        std::vector<int32_t> pre = ds.f0.values;
        std::set<int> sel = {0};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 0.5, 1.0);

        QVERIFY(!ds.f0.values.empty());
        for (size_t i = 0; i < ds.f0.values.size(); ++i) {
            if (pre[i] > 0) {
                QVERIFY(ds.f0.values[i] > 0);
            }
        }
    }

    void testApplyModulationDriftPreview_restNoteSkipped() {
        auto ds = makeDSFile({QStringLiteral("C4"), QStringLiteral("rest")}, 100, 261.6);
        std::vector<int32_t> pre = ds.f0.values;
        std::set<int> sel = {0, 1};

        PitchProcessor::applyModulationDriftPreview(ds, pre, sel, 0.8, 0.8);

        QVERIFY(!ds.f0.values.empty());
    }
};

QTEST_GUILESS_MAIN(TestPitchProcessor)

#include "TestPitchProcessor.moc"
