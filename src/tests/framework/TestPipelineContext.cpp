#include <QTest>
#include <dsfw/PipelineContext.h>

using namespace dstools;

class TestPipelineContext : public QObject {
    Q_OBJECT
private slots:
    void buildTaskInput_ok();
    void buildTaskInput_missing();
    void applyTaskOutput();
    void json_roundtrip();
    void status_serialization();
};

void TestPipelineContext::buildTaskInput_ok() {
    PipelineContext ctx;
    ctx.audioPath = QStringLiteral("/audio/test.wav");
    ctx.globalConfig = {{"sampleRate", 44100}};
    ctx.layers[QStringLiteral("grapheme")] = {{"text", "hello"}};
    ctx.layers[QStringLiteral("phoneme")] = {{"phones", "h eh l ow"}};

    TaskSpec spec;
    spec.taskName = QStringLiteral("alignment");
    spec.inputs = {{QStringLiteral("graph_in"), QStringLiteral("grapheme")},
                   {QStringLiteral("phone_in"), QStringLiteral("phoneme")}};

    auto result = ctx.buildTaskInput(spec);
    QVERIFY(result.ok());
    QCOMPARE(result.value().audioPath, ctx.audioPath);
    QVERIFY(result.value().layers.count(QStringLiteral("graph_in")));
    QVERIFY(result.value().layers.count(QStringLiteral("phone_in")));
    QVERIFY(result.value().config == ctx.globalConfig);
}

void TestPipelineContext::buildTaskInput_missing() {
    PipelineContext ctx;
    ctx.audioPath = QStringLiteral("/audio/test.wav");

    TaskSpec spec;
    spec.taskName = QStringLiteral("alignment");
    spec.inputs = {{QStringLiteral("graph_in"), QStringLiteral("grapheme")}};

    auto result = ctx.buildTaskInput(spec);
    QVERIFY(!result.ok());
    QVERIFY(result.error().find("grapheme") != std::string::npos);
}

void TestPipelineContext::applyTaskOutput() {
    PipelineContext ctx;

    TaskSpec spec;
    spec.outputs = {{QStringLiteral("aligned"), QStringLiteral("alignment")}};

    TaskOutput output;
    output.layers[QStringLiteral("aligned")] = {{"boundaries", "1 2 3"}};

    ctx.applyTaskOutput(spec, output);
    QVERIFY(ctx.layers.count(QStringLiteral("alignment")));
    QVERIFY(ctx.layers.at(QStringLiteral("alignment")) == output.layers.at(QStringLiteral("aligned")));
}

void TestPipelineContext::json_roundtrip() {
    PipelineContext ctx;
    ctx.audioPath = QStringLiteral("/audio/test.wav");
    ctx.itemId = QStringLiteral("item_001");
    ctx.globalConfig = {{"key", "value"}};
    ctx.layers[QStringLiteral("grapheme")] = {{"text", "hello"}};
    ctx.completedSteps = {QStringLiteral("step1"), QStringLiteral("step2")};
    ctx.status = PipelineContext::Status::Active;

    StepRecord rec;
    rec.stepName = QStringLiteral("step1");
    rec.processorId = QStringLiteral("proc1");
    rec.startTime = QDateTime::fromString(QStringLiteral("2025-01-01T00:00:00"), Qt::ISODate);
    rec.endTime = QDateTime::fromString(QStringLiteral("2025-01-01T00:01:00"), Qt::ISODate);
    rec.success = true;
    rec.usedConfig = {{"param", 42}};
    ctx.stepHistory.push_back(rec);

    auto j = ctx.toJson();
    auto result = PipelineContext::fromJson(j);
    QVERIFY(result.ok());

    const auto &restored = result.value();
    QCOMPARE(restored.audioPath, ctx.audioPath);
    QCOMPARE(restored.itemId, ctx.itemId);
    QVERIFY(restored.globalConfig == ctx.globalConfig);
    QCOMPARE(restored.completedSteps, ctx.completedSteps);
    QCOMPARE(restored.status, PipelineContext::Status::Active);
    QCOMPARE(restored.stepHistory.size(), ctx.stepHistory.size());
    QCOMPARE(restored.stepHistory[0].stepName, QStringLiteral("step1"));
    QCOMPARE(restored.stepHistory[0].success, true);
}

void TestPipelineContext::status_serialization() {
    PipelineContext ctx;
    ctx.audioPath = QStringLiteral("/audio/test.wav");
    ctx.itemId = QStringLiteral("item_002");
    ctx.status = PipelineContext::Status::Discarded;
    ctx.discardReason = QStringLiteral("Too short");
    ctx.discardedAtStep = QStringLiteral("validation");

    auto j = ctx.toJson();
    auto result = PipelineContext::fromJson(j);
    QVERIFY(result.ok());

    const auto &restored = result.value();
    QCOMPARE(restored.status, PipelineContext::Status::Discarded);
    QCOMPARE(restored.discardReason, QStringLiteral("Too short"));
    QCOMPARE(restored.discardedAtStep, QStringLiteral("validation"));
}

QTEST_MAIN(TestPipelineContext)
#include "TestPipelineContext.moc"
