#include <QTest>
#include <dsfw/ServiceLocator.h>

using namespace dstools;

class IDummyService {
public:
    virtual ~IDummyService() = default;
    virtual int value() const = 0;
};

class DummyService : public IDummyService {
public:
    int value() const override { return 42; }
};

class TestServiceLocator : public QObject {
    Q_OBJECT
private slots:
    void cleanup();
    void testGetReturnsNullWhenNotRegistered();
    void testSetAndGet();
    void testReset();
    void testResetAll();
};

void TestServiceLocator::cleanup() {
    ServiceLocator::resetAll();
}

void TestServiceLocator::testGetReturnsNullWhenNotRegistered() {
    QVERIFY(ServiceLocator::get<IDummyService>() == nullptr);
}

void TestServiceLocator::testSetAndGet() {
    DummyService svc;
    ServiceLocator::set<IDummyService>(&svc);
    auto *got = ServiceLocator::get<IDummyService>();
    QVERIFY(got != nullptr);
    QCOMPARE(got->value(), 42);
}

void TestServiceLocator::testReset() {
    DummyService svc;
    ServiceLocator::set<IDummyService>(&svc);
    ServiceLocator::reset<IDummyService>();
    QVERIFY(ServiceLocator::get<IDummyService>() == nullptr);
}

void TestServiceLocator::testResetAll() {
    DummyService svc;
    ServiceLocator::set<IDummyService>(&svc);
    ServiceLocator::resetAll();
    QVERIFY(ServiceLocator::get<IDummyService>() == nullptr);
}

QTEST_GUILESS_MAIN(TestServiceLocator)
#include "TestServiceLocator.moc"
