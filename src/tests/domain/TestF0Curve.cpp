#include <QTest>
#include <dstools/F0Curve.h>

using namespace dstools;

class TestF0Curve : public QObject {
    Q_OBJECT
private slots:
    void testEmptyCurve();
    void testTotalDuration();
    void testSampleCount();
    void testGetValueAtTimeInterpolation();
    void testGetValueAtTimeBoundary();
    void testGetRange();
    void testGetRangeEmpty();
    void testSetRange();
    void testSetRangeOutOfBounds();
};

void TestF0Curve::testEmptyCurve() {
    F0Curve curve;
    QVERIFY(curve.isEmpty());
    QCOMPARE(curve.sampleCount(), 0);
    QCOMPARE(curve.totalDuration(), 0.0);
    QCOMPARE(curve.getValueAtTime(0.5), 0.0);
    QVERIFY(curve.getRange(0.0, 1.0).empty());
}

void TestF0Curve::testTotalDuration() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {440.0, 441.0, 442.0, 443.0, 444.0};
    QCOMPARE(curve.totalDuration(), 0.05);
}

void TestF0Curve::testSampleCount() {
    F0Curve curve;
    curve.values = {1.0, 2.0, 3.0};
    QCOMPARE(curve.sampleCount(), 3);
}

void TestF0Curve::testGetValueAtTimeInterpolation() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {100.0, 200.0, 300.0};

    // Exact sample points
    QCOMPARE(curve.getValueAtTime(0.0), 100.0);
    QCOMPARE(curve.getValueAtTime(0.01), 200.0);

    // Mid-point interpolation: between 100 and 200
    double mid = curve.getValueAtTime(0.005);
    QVERIFY(qFuzzyCompare(mid, 150.0));
}

void TestF0Curve::testGetValueAtTimeBoundary() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {100.0, 200.0, 300.0};

    // Before start → first value
    QCOMPARE(curve.getValueAtTime(-1.0), 100.0);

    // Beyond end → last value
    QCOMPARE(curve.getValueAtTime(10.0), 300.0);
}

void TestF0Curve::testGetRange() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {10.0, 20.0, 30.0, 40.0, 50.0};

    auto range = curve.getRange(0.01, 0.03);
    // startIdx=1, endIdx=4 → values[1..3]
    QCOMPARE(range.size(), size_t(3));
    QCOMPARE(range[0], 20.0);
    QCOMPARE(range[1], 30.0);
    QCOMPARE(range[2], 40.0);
}

void TestF0Curve::testGetRangeEmpty() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {10.0, 20.0};

    // Range beyond curve
    auto range = curve.getRange(5.0, 6.0);
    QVERIFY(range.empty());
}

void TestF0Curve::testSetRange() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {10.0, 20.0, 30.0, 40.0, 50.0};

    curve.setRange(0.02, {99.0, 88.0});
    QCOMPARE(curve.values[2], 99.0);
    QCOMPARE(curve.values[3], 88.0);
    // Unchanged
    QCOMPARE(curve.values[0], 10.0);
    QCOMPARE(curve.values[4], 50.0);
}

void TestF0Curve::testSetRangeOutOfBounds() {
    F0Curve curve;
    curve.timestep = 0.01;
    curve.values = {10.0, 20.0, 30.0};

    // Partially out of bounds — should only write valid indices
    curve.setRange(0.02, {99.0, 88.0, 77.0});
    QCOMPARE(curve.values[2], 99.0);
    // index 3 and 4 are out of bounds, should be ignored
    QCOMPARE(curve.values.size(), size_t(3));
}

QTEST_GUILESS_MAIN(TestF0Curve)
#include "TestF0Curve.moc"
