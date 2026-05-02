#include <QTest>
#include <dsfw/PipelineRunner.h>
#include <dsfw/TaskProcessorRegistry.h>

using namespace dstools;

// Simple mock processor that copies input layer to output layer
class MockProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("mock"); }
    QString displayName() const override { return QStringLiteral("Mock"); }
    TaskSpec taskSpec() const override {
        return {QStringLiteral("mock_task"),
                {{QStringLiteral("in"), QStringLiteral("input_cat")}},
                {{QStringLiteral("out"), QStringLiteral("output_cat")}}};
    }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override {
        return Result<void>::Ok();
    }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &input) override {
        TaskOutput out;
        auto it = input.layers.find(QStringLiteral("in"));
        if (it != input.layers.end())
            out.layers[QStringLiteral("out")] = it->second;
        else
            out.layers[QStringLiteral("out")] = nlohmann::json{{"processed", true}};
        return Result<TaskOutput>::Ok(std::move(out));
    }
};

// Register the mock
static TaskProcessorRegistry::Registrar<MockProcessor> s_mockReg(
    QStringLiteral("mock_task"), QStringLiteral("mock"));

class TestPipelineRunner : public QObject {
    Q_OBJECT
private slots:
    void single_step();
    void skip_discarded();
    void optional_missing_processor();
    void validator_discard();
};

// Test: single step processes all contexts
void TestPipelineRunner::single_step() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(2);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = nlohmann::json{{"data", "hello"}};
    ctxs[1].itemId = QStringLiteral("item1");
    ctxs[1].layers[QStringLiteral("input_cat")] = nlohmann::json{{"data", "world"}};

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());
    QVERIFY(ctxs[0].layers.count(QStringLiteral("output_cat")));
    QVERIFY(ctxs[1].layers.count(QStringLiteral("output_cat")));
    QCOMPARE(ctxs[0].completedSteps().size(), 1);
}

// Test: discarded items are skipped
void TestPipelineRunner::skip_discarded() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(2);
    ctxs[0].itemId = QStringLiteral("active");
    ctxs[0].layers[QStringLiteral("input_cat")] = nlohmann::json{{"data", "ok"}};
    ctxs[1].itemId = QStringLiteral("discarded");
    ctxs[1].status = PipelineContext::Status::Discarded;
    ctxs[1].layers[QStringLiteral("input_cat")] = nlohmann::json{{"data", "skip"}};

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());
    QVERIFY(ctxs[0].layers.count(QStringLiteral("output_cat")));
    QVERIFY(!ctxs[1].layers.count(QStringLiteral("output_cat"))); // skipped
}

// Test: optional step with missing processor doesn't fail
void TestPipelineRunner::optional_missing_processor() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("nonexistent_task");
    step.processorId = QStringLiteral("nonexistent");
    step.optional = true;
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok()); // optional, should not fail
}

// Test: validator can discard items
void TestPipelineRunner::validator_discard() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    step.validator = [](const PipelineContext &, const TaskSpec &, QString &reason) {
        reason = QStringLiteral("Too short");
        return PipelineContext::Status::Discarded;
    };
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = nlohmann::json{{"data", "test"}};

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());
    QVERIFY(ctxs[0].status == PipelineContext::Status::Discarded);
    QCOMPARE(ctxs[0].discardReason, QStringLiteral("Too short"));
}

QTEST_MAIN(TestPipelineRunner)
#include "TestPipelineRunner.moc"
