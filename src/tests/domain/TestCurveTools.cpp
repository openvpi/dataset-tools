#include <QTest>
#include <dsfw/signal/curve_tools.h>

using namespace dstools;
using namespace dsfw::signal;

class TestCurveTools : public QObject {
    Q_OBJECT
private slots:
    // Resampling
    void resample_identity();
    void resample_upsample();
    void resample_downsample();
    void resample_align();
    void resample_tail_fill();
    void resample_rmvpe_to_ds();
    void resample_empty();

    // Unvoiced interpolation
    void interp_no_uv();
    void interp_all_uv();
    void interp_edges();
    void interp_middle();

    // Smoothing
    void moving_avg_basic();
    void moving_avg_skip_zero();
    void median_filter_spike();

    // Batch conversions
    void hz_mhz_batch();
    void hz_midi_batch();

    // Alignment
    void align_short();
    void align_long();

    // Crossfade
    void smoothstep_crossfade_basic();

    // Helpers
    void hopsize_to_timestep();
    void expected_frames();

    // Boundary drag
    void drag_boundary_center();
    void drag_boundary_edges();
    void drag_boundary_out_of_bounds();
    void drag_boundary_int32();

    // Double-precision variants
    void resample_curve_f();
    void interp_unvoiced_f();

    // mhzToMidi
    void mhz_to_midi_batch();
};

// ── Resampling ──

void TestCurveTools::resample_identity() {
    std::vector<float> in = {100.0f, 200.0f, 300.0f, 400.0f, 500.0f};
    auto out = resampleCurve(in, 10000, 10000); // same timestep
    QCOMPARE(static_cast<int>(out.size()), 5);
    for (int i = 0; i < 5; ++i)
        QCOMPARE(out[i], in[i]);
}

void TestCurveTools::resample_upsample() {
    // 10ms → 5ms: should roughly double length
    std::vector<float> in = {1000.0f, 2000.0f, 3000.0f};
    auto out = resampleCurve(in, 10000, 5000);
    // At t=0: 1000, t=5ms: 1500, t=10ms: 2000, t=15ms: 2500, t=20ms: 3000
    QCOMPARE(static_cast<int>(out.size()), 5);
    QCOMPARE(out[0], 1000.0f);
    QCOMPARE(out[1], 1500.0f);
    QCOMPARE(out[2], 2000.0f);
    QCOMPARE(out[3], 2500.0f);
    QCOMPARE(out[4], 3000.0f);
}

void TestCurveTools::resample_downsample() {
    // 5ms → 10ms: should halve length
    std::vector<float> in = {1000.0f, 1500.0f, 2000.0f, 2500.0f, 3000.0f};
    auto out = resampleCurve(in, 5000, 10000);
    QCOMPARE(static_cast<int>(out.size()), 3);
    QCOMPARE(out[0], 1000.0f);
    QCOMPARE(out[1], 2000.0f);
    QCOMPARE(out[2], 3000.0f);
}

void TestCurveTools::resample_align() {
    std::vector<float> in = {100.0f, 200.0f, 300.0f};
    auto out = resampleCurve(in, 10000, 10000, 100);
    QCOMPARE(static_cast<int>(out.size()), 100);
    QCOMPARE(out[0], 100.0f);
    QCOMPARE(out[2], 300.0f);
    // Tail filled with last value
    QCOMPARE(out[99], 300.0f);
}

void TestCurveTools::resample_tail_fill() {
    std::vector<float> in = {440000.0f}; // single frame
    auto out = resampleCurve(in, 10000, 10000, 10);
    QCOMPARE(static_cast<int>(out.size()), 10);
    for (int i = 0; i < 10; ++i)
        QCOMPARE(out[i], 440000.0f);
}

void TestCurveTools::resample_rmvpe_to_ds() {
    // RMVPE 10ms → DiffSinger 11.61ms (44100/512)
    TimePos src = secToUs(0.01);                 // 10000 μs
    TimePos dst = hopSizeToTimestep(512, 44100); // ~11610 μs
    QVERIFY(dst > 11600 && dst < 11620);

    // 100 frames at 10ms
    std::vector<float> in(100, 440000);
    int targetLen = 87; // ~100 * 10ms / 11.61ms
    auto out = resampleCurve(in, src, dst, targetLen);
    QCOMPARE(static_cast<int>(out.size()), 87);
    // All same value → output should be identical
    for (int i = 0; i < 87; ++i)
        QCOMPARE(out[i], 440000.0f);
}

void TestCurveTools::resample_empty() {
    auto out = resampleCurve({}, 10000, 10000);
    QVERIFY(out.empty());
}

// ── Unvoiced interpolation ──

void TestCurveTools::interp_no_uv() {
    std::vector<int32_t> v = {440000, 441000, 442000};
    auto copy = v;
    interpUnvoiced(copy);
    QCOMPARE(copy, v); // no change
}

void TestCurveTools::interp_all_uv() {
    std::vector<int32_t> v = {0, 0, 0};
    interpUnvoiced(v);
    // All unvoiced → unchanged
    for (auto x : v)
        QCOMPARE(x, int32_t(0));
}

void TestCurveTools::interp_edges() {
    // Leading and trailing unvoiced
    std::vector<int32_t> v = {0, 0, 440000, 441000, 0, 0};
    std::vector<bool> uv;
    interpUnvoiced(v, &uv);

    QCOMPARE(uv[0], true);
    QCOMPARE(uv[2], false);
    // Leading filled with first voiced
    QCOMPARE(v[0], int32_t(440000));
    QCOMPARE(v[1], int32_t(440000));
    // Trailing filled with last voiced
    QCOMPARE(v[4], int32_t(441000));
    QCOMPARE(v[5], int32_t(441000));
}

void TestCurveTools::interp_middle() {
    // Interior unvoiced gap: log2 domain interpolation
    // 440Hz (440000 mHz) and 880Hz (880000 mHz) → middle should be ~622Hz (geometric mean)
    std::vector<int32_t> v = {440000, 0, 880000};
    interpUnvoiced(v);
    // log2(440)=8.781, log2(880)=9.781, midpoint=9.281 → 2^9.281 ≈ 622.25 Hz
    double midHz = mhzToHz(v[1]);
    QVERIFY(std::abs(midHz - 622.25) < 1.0);
}

// ── Smoothing ──

void TestCurveTools::moving_avg_basic() {
    std::vector<double> v = {10, 20, 30, 40, 50};
    auto out = movingAverage(v, 3, false);
    QCOMPARE(static_cast<int>(out.size()), 5);
    // Middle: (20+30+40)/3 = 30
    QCOMPARE(out[2], 30.0);
}

void TestCurveTools::moving_avg_skip_zero() {
    std::vector<double> v = {100, 0, 200, 0, 300};
    auto out = movingAverage(v, 3, true);
    // Zero values stay zero
    QCOMPARE(out[1], 0.0);
    QCOMPARE(out[3], 0.0);
    // Non-zero: avg of non-zero neighbours in window
    // v[2]=200: window [1,0 skip], [2,200], [3,0 skip] → only 200/1 = 200
    QCOMPARE(out[2], 200.0);
}

void TestCurveTools::median_filter_spike() {
    std::vector<double> v = {10, 10, 1000, 10, 10};
    auto out = medianFilter(v, 3);
    // Spike at index 2 should be eliminated
    QCOMPARE(out[2], 10.0);
}

// ── Batch conversions ──

void TestCurveTools::hz_mhz_batch() {
    auto mhz = hzToMhzBatch({440.0, 0.0, 261.6});
    QCOMPARE(mhz[0], int32_t(440000));
    QCOMPARE(mhz[1], int32_t(0)); // zero preserved
    QCOMPARE(mhz[2], int32_t(261600));

    auto hz = mhzToHzBatch(mhz);
    QCOMPARE(hz[0], 440.0);
    QCOMPARE(hz[1], 0.0);
    QCOMPARE(hz[2], 261.6);
}

void TestCurveTools::hz_midi_batch() {
    auto midi = hzToMidiBatch({440.0, 0.0});
    QCOMPARE(midi[0], 69.0); // A4
    QCOMPARE(midi[1], 0.0);  // unvoiced

    auto hz = midiToHzBatch({69.0});
    QCOMPARE(hz[0], 440.0);
}

// ── Alignment ──

void TestCurveTools::align_short() {
    std::vector<float> v = {1.0f, 2.0f, 3.0f};
    auto out = alignToLength(v, 10);
    QCOMPARE(static_cast<int>(out.size()), 10);
    QCOMPARE(out[0], 1.0f);
    QCOMPARE(out[2], 3.0f);
    // Tail filled with last value (3), not fillValue (0)
    QCOMPARE(out[9], 3.0f);
}

void TestCurveTools::align_long() {
    std::vector<float> v = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    auto out = alignToLength(v, 3);
    QCOMPARE(static_cast<int>(out.size()), 3);
    QCOMPARE(out[0], 1.0f);
    QCOMPARE(out[2], 3.0f);
}

// ── Crossfade ──

void TestCurveTools::smoothstep_crossfade_basic() {
    std::vector<double> original(20, 100.0);
    std::vector<double> modified(20, 200.0);
    smoothstepCrossfade(modified, original, 5);
    // At head (i=0): should be close to original (100)
    QVERIFY(modified[0] < 110.0);
    // At tail (i=19): should be close to original (100)
    QVERIFY(modified[19] < 110.0);
    // In the middle: should be close to 200
    QVERIFY(modified[10] > 190.0);
}

// ── Helpers ──

void TestCurveTools::hopsize_to_timestep() {
    QCOMPARE(hopSizeToTimestep(512, 44100), TimePos(11610));
}

void TestCurveTools::expected_frames() {
    QCOMPARE(expectedFrameCount(44100, 512), 87);
}

QTEST_MAIN(TestCurveTools)

void TestCurveTools::drag_boundary_center() {
    std::vector<double> v = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int boundaryIdx = 5;
    double delta = 10.0;
    dsfw::signal::dragBoundary(v, boundaryIdx, delta, 3);
    QCOMPARE(v[boundaryIdx], 10.0);
    QVERIFY(v[0] < 1.0);
    QVERIFY(v[10] < 1.0);
}

void TestCurveTools::drag_boundary_edges() {
    std::vector<double> v = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    dsfw::signal::dragBoundary(v, 0, 5.0, 3);
    QCOMPARE(v[0], 5.0);
    QVERIFY(v[4] < 1.0);
}

void TestCurveTools::drag_boundary_out_of_bounds() {
    std::vector<double> v = {1, 2, 3};
    auto copy = v;
    dsfw::signal::dragBoundary(v, -1, 10.0, 3);
    QCOMPARE(v, copy);
    dsfw::signal::dragBoundary(v, 100, 10.0, 3);
    QCOMPARE(v, copy);
}

void TestCurveTools::drag_boundary_int32() {
    std::vector<int32_t> v = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    dsfw::signal::dragBoundary(v, 5, 100, 3);
    QCOMPARE(v[5], int32_t(100));
    QVERIFY(v[0] < 1);
    QVERIFY(v[10] < 1);
}

void TestCurveTools::resample_curve_f() {
    std::vector<double> in = {100.0, 200.0, 300.0, 400.0, 500.0};
    auto out = resampleCurveF(in, 10000, 10000);
    QCOMPARE(static_cast<int>(out.size()), 5);
    for (int i = 0; i < 5; ++i)
        QCOMPARE(out[i], in[i]);
}

void TestCurveTools::interp_unvoiced_f() {
    std::vector<double> v = {0.0, 0.0, 440.0, 441.0, 0.0, 0.0};
    std::vector<bool> uv;
    interpUnvoicedF(v, &uv);
    QCOMPARE(uv[0], true);
    QCOMPARE(uv[2], false);
    QCOMPARE(v[0], 440.0);
    QCOMPARE(v[1], 440.0);
    QCOMPARE(v[4], 441.0);
    QCOMPARE(v[5], 441.0);
}

void TestCurveTools::mhz_to_midi_batch() {
    auto midi = mhzToMidiBatch({440000, 0});
    QCOMPARE(midi[0], 69.0);
    QCOMPARE(midi[1], 0.0);
}

#include "TestCurveTools.moc"
