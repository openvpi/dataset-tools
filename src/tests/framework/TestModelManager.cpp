#include <QTest>
#include <dsfw/ModelManager.h>

using namespace dstools;

class MockModelProvider : public IModelProvider {
public:
    int loadCount = 0;
    int unloadCount = 0;
    ModelStatus m_status = ModelStatus::Unloaded;
    int64_t m_mem = 1024;

    ModelType type() const override { return ModelType::Custom; }
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
    mgr.registerProvider(ModelType::Custom, std::move(mock));
    QCOMPARE(mgr.provider(ModelType::Custom), ptr);
}

void TestModelManager::testStatusUnloaded() {
    ModelManager mgr;
    QCOMPARE(mgr.status(ModelType::Custom), ModelStatus::Unloaded);
}

void TestModelManager::testProviderReturnsNull() {
    ModelManager mgr;
    QVERIFY(mgr.provider(ModelType::Asr) == nullptr);
}

void TestModelManager::testMockLoadUnload() {
    ModelManager mgr;
    auto mock = std::make_unique<MockModelProvider>();
    auto *ptr = mock.get();
    mgr.registerProvider(ModelType::Custom, std::move(mock));

    auto r = mgr.ensureLoaded(ModelType::Custom, "/fake/path", 0);
    QVERIFY(r.ok());
    QCOMPARE(ptr->loadCount, 1);
    QCOMPARE(mgr.status(ModelType::Custom), ModelStatus::Ready);

    mgr.unload(ModelType::Custom);
    QCOMPARE(ptr->unloadCount, 1);
    QCOMPARE(mgr.status(ModelType::Custom), ModelStatus::Unloaded);
}

QTEST_GUILESS_MAIN(TestModelManager)
#include "TestModelManager.moc"
