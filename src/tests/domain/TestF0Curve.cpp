#include <QTest>
#include <dstools/F0Curve.h>
#include <dstools/TimePos.h>

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
    QCOMPARE(curve.totalDuration(), TimePos(0));
    QCOMPARE(curve.getValueAtTime(500000), int32_t(0));
    QVERIFY(curve.getRange(0, 1000000).empty());
}

void TestF0Curve::testTotalDuration() {
    F0Curve curve;
    curve.timestep = 10000; // 10ms = 10000us
    curve.values = {440000, 441000, 442000, 443000, 444000}; // mHz
    QCOMPARE(curve.totalDuration(), TimePos(50000)); // 5 * 10ms = 50ms = 50000us
}

void TestF0Curve::testSampleCount() {
    F0Curve curve;
    curve.values = {1000, 2000, 3000};
    QCOMPARE(curve.sampleCount(), 3);
}

void TestF0Curve::testGetValueAtTimeInterpolation() {
    F0Curve curve;
    curve.timestep = 10000; // 10ms
    curve.values = {100000, 200000, 300000}; // mHz

    QCOMPARE(curve.getValueAtTime(0), 100000);

    QCOMPARE(curve.getValueAtTime(10000), 200000);

    int32_t mid = curve.getValueAtTime(5000);
    QVERIFY(qAbs(mid - 150000) <= 1);
}

void TestF0Curve::testGetValueAtTimeBoundary() {
    F0Curve curve;
    curve.timestep = 10000;
    curve.values = {100000, 200000, 300000};

    QCOMPARE(curve.getValueAtTime(-1000000), 100000);

    QCOMPARE(curve.getValueAtTime(100000000), 300000);
}

void TestF0Curve::testGetRange() {
    F0Curve curve;
    curve.timestep = 10000;
    curve.values = {10000, 20000, 30000, 40000, 50000};

    auto range = curve.getRange(10000, 30000);
    QCOMPARE(range.size(), size_t(3));
    QCOMPARE(range[0], 20000);
    QCOMPARE(range[1], 30000);
    QCOMPARE(range[2], 40000);
}

void TestF0Curve::testGetRangeEmpty() {
    F0Curve curve;
    curve.timestep = 10000;
    curve.values = {10000, 20000};

    auto range = curve.getRange(50000000, 60000000);
    QVERIFY(range.empty());
}

void TestF0Curve::testSetRange() {
    F0Curve curve;
    curve.timestep = 10000;
    curve.values = {10000, 20000, 30000, 40000, 50000};

    curve.setRange(20000, {99000, 88000});
    QCOMPARE(curve.values[2], 99000);
    QCOMPARE(curve.values[3], 88000);
    QCOMPARE(curve.values[0], 10000);
    QCOMPARE(curve.values[4], 50000);
}

void TestF0Curve::testSetRangeOutOfBounds() {
    F0Curve curve;
    curve.timestep = 10000;
    curve.values = {10000, 20000, 30000};

    curve.setRange(20000, {99000, 88000, 77000});
    QCOMPARE(curve.values[2], 99000);
    QCOMPARE(curve.values.size(), size_t(3));
}

QTEST_GUILESS_MAIN(TestF0Curve)
#include "TestF0Curve.moc"
