#include <QTest>
#include <dstools/ModelManager.h>

using namespace dstools;

static const auto TestCustomType = registerModelType("Custom");
static const auto TestAsrType = registerModelType("Asr");

class MockModelProvider : public IModelProvider {
public:
    int loadCount = 0;
    int unloadCount = 0;
    ModelStatus m_status = ModelStatus::Unloaded;
    int64_t m_mem = 1024;

    ModelTypeId type() const override { return TestCustomType; }
    QString displayName() const override { return "MockModel"; }

    Result<void> load(const QString & /*modelPath*/, int /*gpuIndex*/) override {
        ++loadCount;
        m_status = ModelStatus::Ready;
        return Result<void>::Ok();
    }

    void unload() override {
        ++unloadCount;
        m_status = ModelStatus::Unloaded;
    }

    ModelStatus status() const override { return m_status; }
    int64_t estimatedMemoryBytes() const override { return m_mem; }
};

class TestModelManager : public QObject {
    Q_OBJECT
private slots:
    void testRegisterProvider();
    void testStatusUnloaded();
    void testProviderReturnsNull();
    void testMockLoadUnload();
};

void TestModelManager::testRegisterProvider() {
    ModelManager mgr;
    auto mock = std::make_unique<MockModelProvider>();
    auto *ptr = mock.get();
    mgr.registerProvider(TestCustomType, std::move(mock));
    QCOMPARE(mgr.provider(TestCustomType), ptr);
}

void TestModelManager::testStatusUnloaded() {
    ModelManager mgr;
    QCOMPARE(mgr.status(TestCustomType), ModelStatus::Unloaded);
}

void TestModelManager::testProviderReturnsNull() {
    ModelManager mgr;
    QVERIFY(mgr.provider(TestAsrType) == nullptr);
}

void TestModelManager::testMockLoadUnload() {
    ModelManager mgr;
    auto mock = std::make_unique<MockModelProvider>();
    auto *ptr = mock.get();
    mgr.registerProvider(TestCustomType, std::move(mock));

    auto r = mgr.ensureLoaded(TestCustomType, "/fake/path", 0);
    QVERIFY(r.ok());
    QCOMPARE(ptr->loadCount, 1);
    QCOMPARE(mgr.status(TestCustomType), ModelStatus::Ready);

    mgr.unload(TestCustomType);
    QCOMPARE(ptr->unloadCount, 1);
    QCOMPARE(mgr.status(TestCustomType), ModelStatus::Unloaded);
}

QTEST_GUILESS_MAIN(TestModelManager)
#include "TestModelManager.moc"
