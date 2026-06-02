#include <QTest>
#include <dsfw/PipelineRunner.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <QTemporaryDir>
#include <QDir>
#include <nlohmann/json.hpp>

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
    Result<void> initialize(ModelManager&, const ProcessorConfig&) override { return Result<void>::Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput& input) override {
        TaskOutput out;
        auto it = input.layers.find(QStringLiteral("in"));
        if (it != input.layers.end())
            out.layers[QStringLiteral("out")] = it->second;
        else
            out.layers[QStringLiteral("out")] = LayerData::fromJson(nlohmann::json{{"processed", true}});
        return Result<TaskOutput>::Ok(std::move(out));
    }
};

// Register the mock
static TaskProcessorRegistry::Registrar<MockProcessor> s_mockReg(QStringLiteral("mock_task"), QStringLiteral("mock"));

class TestPipelineRunner : public QObject {
    Q_OBJECT
private slots:
    void single_step();
    void skip_discarded();
    void optional_missing_processor();
    void required_missing_processor_fails();
    void validator_discard();
    void cancel_during_execution();
    void dirty_propagation_single_layer();
    void dirty_propagation_downstream();
    void snapshot_save_and_load();
    void snapshot_cleanup_old();
    void snapshot_has_latest();
    void snapshot_round_trip_all_fields();
    void precondition_missing_layer_fails();
    void precondition_empty_layer_fails();
    void precondition_optional_skips_gracefully();
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
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "hello"}});
    ctxs[1].itemId = QStringLiteral("item1");
    ctxs[1].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "world"}});

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
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "ok"}});
    ctxs[1].itemId = QStringLiteral("discarded");
    ctxs[1].status = PipelineContext::Status::Discarded;
    ctxs[1].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "skip"}});

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
    step.validator = [](const PipelineContext&, const TaskSpec&, QString& reason) {
        reason = QStringLiteral("Too short");
        return PipelineContext::Status::Discarded;
    };
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "test"}});

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());
    QVERIFY(ctxs[0].status == PipelineContext::Status::Discarded);
    QCOMPARE(ctxs[0].discardReason, QStringLiteral("Too short"));
}

// Test: required step with missing processor fails
void TestPipelineRunner::required_missing_processor_fails() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("nonexistent_task");
    step.processorId = QStringLiteral("nonexistent");
    step.optional = false;
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");

    auto res = runner.run(opts, ctxs);
    QVERIFY(!res.ok());
}

// Test: cancel token stops execution
void TestPipelineRunner::cancel_during_execution() {
    PipelineRunner runner;
    PipelineOptions opts;

    StepConfig step1;
    step1.taskName = QStringLiteral("mock_task");
    step1.processorId = QStringLiteral("mock");
    StepConfig step2;
    step2.taskName = QStringLiteral("mock_task");
    step2.processorId = QStringLiteral("mock");
    opts.steps = {step1, step2};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "test"}});

    auto cancelToken = std::make_shared<std::atomic<bool>>(true);
    auto res = runner.run(opts, ctxs, cancelToken);
    QVERIFY(!res.ok());
}

// Test: dirty propagation marks a layer as dirty
void TestPipelineRunner::dirty_propagation_single_layer() {
    PipelineContext ctx;
    ctx.addDirtyLayer(QStringLiteral("wave"));
    QVERIFY(ctx.dirty.contains(QStringLiteral("wave")));
    ctx.removeDirtyLayer(QStringLiteral("wave"));
    QVERIFY(!ctx.dirty.contains(QStringLiteral("wave")));
}

// Test: propagateDirty adds the layer and its downstream dependencies to dirty set
void TestPipelineRunner::dirty_propagation_downstream() {
    PipelineContext ctx;
    ctx.propagateDirty(QStringLiteral("wave"));
    QVERIFY(ctx.dirty.contains(QStringLiteral("wave")));
}

void TestPipelineRunner::snapshot_save_and_load() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};
    opts.snapshotDir = tmpDir.path();

    std::vector<PipelineContext> ctxs(2);
    ctxs[0].itemId = QStringLiteral("snap_item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "snap0"}});
    ctxs[1].itemId = QStringLiteral("snap_item1");
    ctxs[1].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", "snap1"}});

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());

    auto loadedResult = PipelineRunner::loadLatestSnapshot(tmpDir.path());
    QVERIFY(loadedResult.ok());

    const auto& [loadedCtxs, loadedStep] = loadedResult.value();
    QVERIFY(!loadedCtxs.empty());
    QVERIFY(loadedStep >= 0);
}

void TestPipelineRunner::snapshot_cleanup_old() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    for (int i = 0; i < PipelineRunner::kMaxSnapshots + 2; ++i) {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("mock_task");
        step.processorId = QStringLiteral("mock");
        opts.steps = {step};
        opts.snapshotDir = tmpDir.path();

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("cleanup_item_%1").arg(i);
        ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", i}});

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
    }

    QDir snapDir(tmpDir.path());
    const auto entries = snapDir.entryInfoList({QStringLiteral("step_*.json")}, QDir::Files);
    QVERIFY(entries.size() <= PipelineRunner::kMaxSnapshots);
}

void TestPipelineRunner::snapshot_has_latest() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QVERIFY(!PipelineRunner::hasLatestSnapshot(tmpDir.path()));

    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};
    opts.snapshotDir = tmpDir.path();

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData::fromJson(nlohmann::json{{"data", 42}});

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());

    QVERIFY(PipelineRunner::hasLatestSnapshot(tmpDir.path()));
}

void TestPipelineRunner::snapshot_round_trip_all_fields() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};
    opts.snapshotDir = tmpDir.path();

    std::vector<PipelineContext> ctxs(1);
    auto& ctx = ctxs[0];
    ctx.itemId = QStringLiteral("round_trip_item");
    ctx.audioPath = QStringLiteral("/fake/audio.wav");
    ctx.layers[QStringLiteral("wave")] = LayerData::fromJson(nlohmann::json{{"duration", 3.5}, {"sampleRate", 44100}});
    ctx.layers[QStringLiteral("pitch")] =
        LayerData::fromJson(nlohmann::json{{"f0", nlohmann::json::array({100.0, 200.0, 150.0})}, {"confidence", 0.95}});
    ctx.globalConfig["hopSize"] = ConfigValue(static_cast<int64_t>(256));
    ctx.dirty = {QStringLiteral("wave")};
    ctx.editedSteps = {QStringLiteral("mock_task")};
    ctx.manuallyEdited = {QStringLiteral("pitch")};

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());

    auto loadedResult = PipelineRunner::loadLatestSnapshot(tmpDir.path());
    QVERIFY(loadedResult.ok());

    const auto& [loadedCtxs, loadedStep] = loadedResult.value();
    QCOMPARE(loadedCtxs.size(), ctxs.size());
    QVERIFY(loadedStep >= 0);

    const auto& loaded = loadedCtxs[0];
    QCOMPARE(loaded.itemId, QStringLiteral("round_trip_item"));
    QCOMPARE(loaded.audioPath, QStringLiteral("/fake/audio.wav"));
    QVERIFY(loaded.layers.count(QStringLiteral("wave")));
    QVERIFY(loaded.layers.count(QStringLiteral("pitch")));
    QVERIFY(loaded.layers.count(QStringLiteral("output_cat")));
    QVERIFY(!loaded.stepHistory.empty());
    QCOMPARE(loaded.stepHistory.back().stepName, QStringLiteral("mock_task"));
    QVERIFY(loaded.stepHistory.back().success);
    QVERIFY(loaded.dirty.contains(QStringLiteral("wave")));
    QVERIFY(loaded.manuallyEdited.contains(QStringLiteral("pitch")));
    QCOMPARE(loaded.status, PipelineContext::Status::Active);
}

void TestPipelineRunner::precondition_missing_layer_fails() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");

    auto res = runner.run(opts, ctxs);
    QVERIFY(!res.ok());
    QVERIFY(ctxs[0].status == PipelineContext::Status::Error);
}

void TestPipelineRunner::precondition_empty_layer_fails() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");
    ctxs[0].layers[QStringLiteral("input_cat")] = LayerData();

    auto res = runner.run(opts, ctxs);
    QVERIFY(!res.ok());
    QVERIFY(ctxs[0].status == PipelineContext::Status::Error);
}

void TestPipelineRunner::precondition_optional_skips_gracefully() {
    PipelineRunner runner;
    PipelineOptions opts;
    StepConfig step;
    step.taskName = QStringLiteral("mock_task");
    step.processorId = QStringLiteral("mock");
    step.optional = true;
    opts.steps = {step};

    std::vector<PipelineContext> ctxs(1);
    ctxs[0].itemId = QStringLiteral("item0");

    auto res = runner.run(opts, ctxs);
    QVERIFY(res.ok());
}

QTEST_MAIN(TestPipelineRunner)
#include "TestPipelineRunner.moc"
