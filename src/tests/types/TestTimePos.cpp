#include <QTest>
#include <dstools/TimePos.h>

using namespace dstools;

class TestTimePos : public QObject {
    Q_OBJECT
private slots:
    void secRoundtrip();
    void zero();
    void negative();
    void largeValue();
    void subUsRounding();
    void msRoundtrip();
    void f0Roundtrip();
    void f0Zero();
    void constants();
};

void TestTimePos::secRoundtrip() {
    QCOMPARE(secToUs(1.2), TimePos(1200000));
    QCOMPARE(usToSec(1200000), 1.2);

    // 6 decimal place round-trip
    QCOMPARE(secToUs(0.080000), TimePos(80000));
    QCOMPARE(usToSec(80000), 0.08);
}

void TestTimePos::zero() {
    QCOMPARE(secToUs(0.0), TimePos(0));
    QCOMPARE(usToSec(0), 0.0);
}

void TestTimePos::negative() {
    QCOMPARE(secToUs(-1.5), TimePos(-1500000));
    QCOMPARE(usToSec(-1500000), -1.5);
}

void TestTimePos::largeValue() {
    // 1 hour — no overflow
    QCOMPARE(secToUs(3600.0), TimePos(3600000000LL));
    QCOMPARE(usToSec(3600000000LL), 3600.0);
}

void TestTimePos::subUsRounding() {
    // 0.5 μs rounds to 1
    QCOMPARE(secToUs(0.0000005), TimePos(1));
    // 0.4 μs rounds to 0
    QCOMPARE(secToUs(0.0000004), TimePos(0));
}

void TestTimePos::msRoundtrip() {
    QCOMPARE(msToUs(1.0), TimePos(1000));
    QCOMPARE(usToMs(1000), 1.0);

    QCOMPARE(msToUs(0.5), TimePos(500));
    QCOMPARE(usToMs(500), 0.5);
}

void TestTimePos::f0Roundtrip() {
    QCOMPARE(hzToMhz(261.6), int32_t(261600));
    QCOMPARE(mhzToHz(261600), 261.6);

    // A4 = 440 Hz
    QCOMPARE(hzToMhz(440.0), int32_t(440000));
    QCOMPARE(mhzToHz(440000), 440.0);
}

void TestTimePos::f0Zero() {
    // Unvoiced frame
    QCOMPARE(hzToMhz(0.0), int32_t(0));
    QCOMPARE(mhzToHz(0), 0.0);
}

void TestTimePos::constants() {
    QCOMPARE(kMicrosecondsPerSecond, TimePos(1000000));
    QCOMPARE(kMicrosecondsPerMs, TimePos(1000));
}

QTEST_MAIN(TestTimePos)
#include "TestTimePos.moc"
