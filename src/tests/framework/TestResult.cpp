#include <QTest>
#include <dsfw/Result.h>


class TestResult : public QObject {
    Q_OBJECT
private slots:
    void testOkConstruction();
    void testErrorConstruction();
    void testValueAccess();
    void testValueOr();
    void testBoolConversion();
    void testErrorMessage();
    void testVoidOk();
    void testVoidError();
    void testVoidBoolConversion();
};

void TestResult::testOkConstruction() {
    auto r = dsfw::Result<int>::Ok(42);
    QVERIFY(r.ok());
    QCOMPARE(r.value(), 42);
}

void TestResult::testErrorConstruction() {
    auto r = dsfw::Result<int>::Error("bad");
    QVERIFY(!r.ok());
    QCOMPARE(r.error(), std::string("bad"));
}

void TestResult::testValueAccess() {
    auto r = dsfw::Result<std::string>::Ok("hello");
    QCOMPARE(*r, std::string("hello"));
    QCOMPARE(r->size(), 5u);
}

void TestResult::testValueOr() {
    auto ok = dsfw::Result<int>::Ok(10);
    QCOMPARE(ok.value_or(99), 10);

    auto err = dsfw::Result<int>::Error("fail");
    QCOMPARE(err.value_or(99), 99);
}

void TestResult::testBoolConversion() {
    auto ok = dsfw::Result<int>::Ok(1);
    QVERIFY(static_cast<bool>(ok));

    auto err = dsfw::Result<int>::Error("x");
    QVERIFY(!static_cast<bool>(err));
}

void TestResult::testErrorMessage() {
    auto r = dsfw::Result<int>::Error("something went wrong");
    QCOMPARE(r.error(), std::string("something went wrong"));
}

void TestResult::testVoidOk() {
    auto r = dsfw::Result<void>::Ok();
    QVERIFY(r.ok());
}

void TestResult::testVoidError() {
    auto r = dsfw::Result<void>::Error("oops");
    QVERIFY(!r.ok());
    QCOMPARE(r.error(), std::string("oops"));
}

void TestResult::testVoidBoolConversion() {
    QVERIFY(static_cast<bool>(dsfw::Result<void>::Ok()));
    QVERIFY(!static_cast<bool>(dsfw::Result<void>::Error("x")));
}

QTEST_GUILESS_MAIN(TestResult)
#include "TestResult.moc"
