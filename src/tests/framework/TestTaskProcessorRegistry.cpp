#include <QTest>
#include <dsfw/TaskProcessorRegistry.h>

using namespace dstools;

class DummyProcessor : public ITaskProcessor {
public:
    explicit DummyProcessor(QString id = QStringLiteral("dummy"), QString task = QStringLiteral("test_task")) :
        m_id(std::move(id)), m_task(std::move(task)) {
    }

    QString processorId() const override {
        return m_id;
    }
    QString displayName() const override {
        return QStringLiteral("Dummy");
    }
    TaskSpec taskSpec() const override {
        return {m_task, {}, {}};
    }
    Result<void> initialize(ModelManager &, const ProcessorConfig &) override {
        return Ok();
    }
    void release() override {
    }
    Result<TaskOutput> process(const TaskInput &) override {
        return Ok(TaskOutput{});
    }

private:
    QString m_id;
    QString m_task;
};

// Static Registrar instance for testRegistrar
static TaskProcessorRegistry::Registrar<DummyProcessor> s_dummyRegistrar(QStringLiteral("registrar_task"),
                                                                         QStringLiteral("auto_dummy"));

class TestTaskProcessorRegistry : public QObject {
    Q_OBJECT
private slots:
    void testRegisterAndCreate();
    void testCreateUnknownReturnsNull();
    void testAvailableProcessors();
    void testAvailableTasks();
    void testRegistrar();
    void testConsistentTaskSpecAccepted();
    void testInconsistentInputCountRejected();
    void testInconsistentSlotNameRejected();
    void testInconsistentOutputCountRejected();
};

void TestTaskProcessorRegistry::testRegisterAndCreate() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(QStringLiteral("task_rc"), QStringLiteral("proc_rc"), [] {
        return std::make_unique<DummyProcessor>(QStringLiteral("proc_rc"), QStringLiteral("task_rc"));
    });

    auto proc = reg.create(QStringLiteral("task_rc"), QStringLiteral("proc_rc"));
    QVERIFY2(proc != nullptr, "create() should return a valid processor");
    QCOMPARE(proc->processorId(), QStringLiteral("proc_rc"));
}

void TestTaskProcessorRegistry::testCreateUnknownReturnsNull() {
    auto &reg = TaskProcessorRegistry::instance();
    QVERIFY(reg.create(QStringLiteral("no_task"), QStringLiteral("no_proc")) == nullptr);
    // Known task, unknown processor
    reg.registerProcessor(QStringLiteral("task_null"), QStringLiteral("proc_null"),
                          [] { return std::make_unique<DummyProcessor>(); });
    QVERIFY(reg.create(QStringLiteral("task_null"), QStringLiteral("unknown")) == nullptr);
}

void TestTaskProcessorRegistry::testAvailableProcessors() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(QStringLiteral("task_ap"), QStringLiteral("proc_a"),
                          [] { return std::make_unique<DummyProcessor>(); });
    reg.registerProcessor(QStringLiteral("task_ap"), QStringLiteral("proc_b"),
                          [] { return std::make_unique<DummyProcessor>(); });

    auto list = reg.availableProcessors(QStringLiteral("task_ap"));
    QVERIFY2(list.contains(QStringLiteral("proc_a")), "should contain proc_a");
    QVERIFY2(list.contains(QStringLiteral("proc_b")), "should contain proc_b");
    QCOMPARE(list.size(), 2);
}

void TestTaskProcessorRegistry::testAvailableTasks() {
    auto &reg = TaskProcessorRegistry::instance();
    reg.registerProcessor(QStringLiteral("task_at1"), QStringLiteral("p1"),
                          [] { return std::make_unique<DummyProcessor>(); });
    reg.registerProcessor(QStringLiteral("task_at2"), QStringLiteral("p2"),
                          [] { return std::make_unique<DummyProcessor>(); });

    auto tasks = reg.availableTasks();
    QVERIFY2(tasks.contains(QStringLiteral("task_at1")), "should contain task_at1");
    QVERIFY2(tasks.contains(QStringLiteral("task_at2")), "should contain task_at2");
}

void TestTaskProcessorRegistry::testRegistrar() {
    auto &reg = TaskProcessorRegistry::instance();
    auto proc = reg.create(QStringLiteral("registrar_task"), QStringLiteral("auto_dummy"));
    QVERIFY2(proc != nullptr, "Registrar should auto-register at static init");
    QCOMPARE(proc->processorId(), QStringLiteral("dummy"));
}

class TaskSpecValidationProcessor : public ITaskProcessor {
public:
    TaskSpecValidationProcessor(QString id, QString task,
                                std::vector<SlotSpec> inputs,
                                std::vector<SlotSpec> outputs) :
        m_id(std::move(id)), m_task(std::move(task)),
        m_inputs(std::move(inputs)), m_outputs(std::move(outputs)) {
    }

    QString processorId() const override { return m_id; }
    QString displayName() const override { return QStringLiteral("ValidationProcessor"); }
    TaskSpec taskSpec() const override { return {m_task, m_inputs, m_outputs}; }
    Result<void> initialize(ModelManager &, const ProcessorConfig &) override { return Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override { return Ok(TaskOutput{}); }

private:
    QString m_id;
    QString m_task;
    std::vector<SlotSpec> m_inputs;
    std::vector<SlotSpec> m_outputs;
};

void TestTaskProcessorRegistry::testConsistentTaskSpecAccepted() {
    auto &reg = TaskProcessorRegistry::instance();
    const QString taskName = QStringLiteral("ts_consistent");

    std::vector<SlotSpec> inputs = {{QStringLiteral("audio"), QStringLiteral("wave")}};
    std::vector<SlotSpec> outputs = {{QStringLiteral("pitch"), QStringLiteral("f0")}};

    reg.registerProcessor(taskName, QStringLiteral("proc_a"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("proc_a"), taskName, inputs, outputs);
    });
    reg.registerProcessor(taskName, QStringLiteral("proc_b"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("proc_b"), taskName, inputs, outputs);
    });

    auto procs = reg.availableProcessors(taskName);
    QCOMPARE(procs.size(), 2);
    QVERIFY2(procs.contains(QStringLiteral("proc_a")), "proc_a should be registered");
    QVERIFY2(procs.contains(QStringLiteral("proc_b")), "proc_b should be registered");
}

void TestTaskProcessorRegistry::testInconsistentInputCountRejected() {
    auto &reg = TaskProcessorRegistry::instance();
    const QString taskName = QStringLiteral("ts_input_count");

    std::vector<SlotSpec> baselineInputs = {{QStringLiteral("audio"), QStringLiteral("wave")}};
    std::vector<SlotSpec> baselineOutputs = {{QStringLiteral("pitch"), QStringLiteral("f0")}};
    std::vector<SlotSpec> badInputs = {}; // empty — different count

    reg.registerProcessor(taskName, QStringLiteral("baseline"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("baseline"), taskName,
                                                             baselineInputs, baselineOutputs);
    });
    reg.registerProcessor(taskName, QStringLiteral("bad"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("bad"), taskName,
                                                             badInputs, baselineOutputs);
    });

    auto procs = reg.availableProcessors(taskName);
    QCOMPARE(procs.size(), 1);
    QVERIFY2(procs.contains(QStringLiteral("baseline")), "baseline should still be registered");
    QVERIFY2(!procs.contains(QStringLiteral("bad")), "bad should be rejected");
}

void TestTaskProcessorRegistry::testInconsistentSlotNameRejected() {
    auto &reg = TaskProcessorRegistry::instance();
    const QString taskName = QStringLiteral("ts_slot_name");

    std::vector<SlotSpec> baselineInputs = {{QStringLiteral("audio"), QStringLiteral("wave")}};
    std::vector<SlotSpec> baselineOutputs = {{QStringLiteral("pitch"), QStringLiteral("f0")}};
    std::vector<SlotSpec> badInputs = {{QStringLiteral("different_name"), QStringLiteral("wave")}};

    reg.registerProcessor(taskName, QStringLiteral("baseline"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("baseline"), taskName,
                                                             baselineInputs, baselineOutputs);
    });
    reg.registerProcessor(taskName, QStringLiteral("bad"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("bad"), taskName,
                                                             badInputs, baselineOutputs);
    });

    auto procs = reg.availableProcessors(taskName);
    QCOMPARE(procs.size(), 1);
    QVERIFY2(!procs.contains(QStringLiteral("bad")), "bad should be rejected due to slot name mismatch");
}

void TestTaskProcessorRegistry::testInconsistentOutputCountRejected() {
    auto &reg = TaskProcessorRegistry::instance();
    const QString taskName = QStringLiteral("ts_output_count");

    std::vector<SlotSpec> baselineInputs = {{QStringLiteral("audio"), QStringLiteral("wave")}};
    std::vector<SlotSpec> baselineOutputs = {{QStringLiteral("pitch"), QStringLiteral("f0")}};
    std::vector<SlotSpec> badOutputs = {};

    reg.registerProcessor(taskName, QStringLiteral("baseline"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("baseline"), taskName,
                                                             baselineInputs, baselineOutputs);
    });
    reg.registerProcessor(taskName, QStringLiteral("bad"), [&] {
        return std::make_unique<TaskSpecValidationProcessor>(QStringLiteral("bad"), taskName,
                                                             baselineInputs, badOutputs);
    });

    auto procs = reg.availableProcessors(taskName);
    QCOMPARE(procs.size(), 1);
    QVERIFY2(!procs.contains(QStringLiteral("bad")), "bad should be rejected due to output count mismatch");
}

QTEST_GUILESS_MAIN(TestTaskProcessorRegistry)
#include "TestTaskProcessorRegistry.moc"
