#include <QTest>
#include <dsfw/TaskProcessorRegistry.h>

using namespace dstools;

class DummyProcessor : public ITaskProcessor {
public:
    explicit DummyProcessor(QString id = QStringLiteral("dummy"),
                            QString task = QStringLiteral("test_task"))
        : m_id(std::move(id)), m_task(std::move(task)) {}

    QString processorId() const override { return m_id; }
    QString displayName() const override { return QStringLiteral("Dummy"); }
    TaskSpec taskSpec() const override { return {m_task, {}, {}}; }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override {
        return Ok();
    }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override {
        return Ok(TaskOutput{});
    }

private:
    QString m_id;
    QString m_task;
};

// Static Registrar instance for testRegistrar
static TaskProcessorRegistry::Registrar<DummyProcessor>
    s_dummyRegistrar(QStringLiteral("registrar_task"),
                     QStringLiteral("auto_dummy"));

class TestTaskProcessorRegistry : public QObject {
    Q_OBJECT
private slots:
    void testRegisterAndCreate();
    void testCreateUnknownReturnsNull();
    void testAvailableProcessors();
    void testAvailableTasks();
    void testRegistrar();
};

void TestTaskProcessorRegistry::testRegisterAndCreate() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(
        QStringLiteral("task_rc"), QStringLiteral("proc_rc"),
        [] { return std::make_unique<DummyProcessor>(QStringLiteral("proc_rc"),
                                                     QStringLiteral("task_rc")); });

    auto proc = reg.create(QStringLiteral("task_rc"), QStringLiteral("proc_rc"));
    QVERIFY2(proc != nullptr, "create() should return a valid processor");
    QCOMPARE(proc->processorId(), QStringLiteral("proc_rc"));
}

void TestTaskProcessorRegistry::testCreateUnknownReturnsNull() {
    auto &reg = TaskProcessorRegistry::instance();
    QVERIFY(reg.create(QStringLiteral("no_task"), QStringLiteral("no_proc")) == nullptr);
    // Known task, unknown processor
    reg.registerProcessor(
        QStringLiteral("task_null"), QStringLiteral("proc_null"),
        [] { return std::make_unique<DummyProcessor>(); });
    QVERIFY(reg.create(QStringLiteral("task_null"), QStringLiteral("unknown")) == nullptr);
}

void TestTaskProcessorRegistry::testAvailableProcessors() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(
        QStringLiteral("task_ap"), QStringLiteral("proc_a"),
        [] { return std::make_unique<DummyProcessor>(); });
    reg.registerProcessor(
        QStringLiteral("task_ap"), QStringLiteral("proc_b"),
        [] { return std::make_unique<DummyProcessor>(); });

    auto list = reg.availableProcessors(QStringLiteral("task_ap"));
    QVERIFY2(list.contains(QStringLiteral("proc_a")), "should contain proc_a");
    QVERIFY2(list.contains(QStringLiteral("proc_b")), "should contain proc_b");
    QCOMPARE(list.size(), 2);
}

void TestTaskProcessorRegistry::testAvailableTasks() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(
        QStringLiteral("task_at1"), QStringLiteral("p1"),
        [] { return std::make_unique<DummyProcessor>(); });
    reg.registerProcessor(
        QStringLiteral("task_at2"), QStringLiteral("p2"),
        [] { return std::make_unique<DummyProcessor>(); });

    auto tasks = reg.availableTasks();
    QVERIFY2(tasks.contains(QStringLiteral("task_at1")), "should contain task_at1");
    QVERIFY2(tasks.contains(QStringLiteral("task_at2")), "should contain task_at2");
}

void TestTaskProcessorRegistry::testRegistrar() {
    auto &reg = TaskProcessorRegistry::instance();
    auto proc = reg.create(QStringLiteral("registrar_task"),
                           QStringLiteral("auto_dummy"));
    QVERIFY2(proc != nullptr,
             "Registrar should auto-register at static init");
    QCOMPARE(proc->processorId(), QStringLiteral("dummy"));
}

QTEST_GUILESS_MAIN(TestTaskProcessorRegistry)
#include "TestTaskProcessorRegistry.moc"
