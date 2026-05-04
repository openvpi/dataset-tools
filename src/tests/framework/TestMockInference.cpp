#include <QTest>
#include <dsfw/PipelineRunner.h>
#include <dsfw/PipelineContext.h>
#include <dsfw/TaskProcessorRegistry.h>

using namespace dstools;

class MockPitchProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("mock-rmvpe"); }
    QString displayName() const override { return QStringLiteral("Mock RMVPE"); }
    TaskSpec taskSpec() const override {
        return {QStringLiteral("pitch_extraction"),
                {{QStringLiteral("audio_in"), QStringLiteral("audio")}},
                {{QStringLiteral("f0_out"), QStringLiteral("pitch")}}};
    }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override { return Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override {
        TaskOutput out;
        int numFrames = 100;
        std::vector<float> f0(numFrames);
        for (int i = 0; i < numFrames; ++i)
            f0[i] = 440.0f;
        nlohmann::json pitchJson;
        pitchJson["f0"] = f0;
        pitchJson["timestep"] = 0.01;
        out.layers[QStringLiteral("f0_out")] = pitchJson;
        return Ok(std::move(out));
    }
};

class MockMidiProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("mock-game"); }
    QString displayName() const override { return QStringLiteral("Mock GAME"); }
    TaskSpec taskSpec() const override {
        return {QStringLiteral("midi_transcription"),
                {{QStringLiteral("audio_in"), QStringLiteral("audio")}},
                {{QStringLiteral("midi_out"), QStringLiteral("midi")}}};
    }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override { return Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override {
        TaskOutput out;
        nlohmann::json midiJson = nlohmann::json::array();
        midiJson.push_back({{"pitch", 60}, {"onset", 0.0}, {"duration", 1.0}, {"voiced", true}});
        midiJson.push_back({{"pitch", 64}, {"onset", 1.0}, {"duration", 0.5}, {"voiced", true}});
        out.layers[QStringLiteral("midi_out")] = midiJson;
        return Ok(std::move(out));
    }
};

class MockAlignmentProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("mock-hfa"); }
    QString displayName() const override { return QStringLiteral("Mock HFA"); }
    TaskSpec taskSpec() const override {
        return {QStringLiteral("phoneme_alignment"),
                {{QStringLiteral("audio_in"), QStringLiteral("audio")},
                 {QStringLiteral("graph_in"), QStringLiteral("grapheme")}},
                {{QStringLiteral("phone_out"), QStringLiteral("phoneme")}}};
    }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override { return Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override {
        TaskOutput out;
        nlohmann::json phoneJson;
        phoneJson["phonemes"] = "h eh l ow";
        phoneJson["durations"] = {0.1, 0.1, 0.2, 0.1, 0.1};
        out.layers[QStringLiteral("phone_out")] = phoneJson;
        return Ok(std::move(out));
    }
};

class FailingProcessor : public ITaskProcessor {
public:
    QString processorId() const override { return QStringLiteral("failing"); }
    QString displayName() const override { return QStringLiteral("Failing"); }
    TaskSpec taskSpec() const override {
        return {QStringLiteral("failing_task"),
                {{QStringLiteral("in"), QStringLiteral("input")}},
                {{QStringLiteral("out"), QStringLiteral("output")}}};
    }
    Result<void> initialize(IModelManager &, const ProcessorConfig &) override { return Ok(); }
    void release() override {}
    Result<TaskOutput> process(const TaskInput &) override {
        return Err<TaskOutput>("Simulated inference failure");
    }
};

class TestMockInference : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        auto &reg = TaskProcessorRegistry::instance();
        reg.registerProcessor(
            QStringLiteral("pitch_extraction"), QStringLiteral("mock-rmvpe"),
            [] { return std::make_unique<MockPitchProcessor>(); });
        reg.registerProcessor(
            QStringLiteral("midi_transcription"), QStringLiteral("mock-game"),
            [] { return std::make_unique<MockMidiProcessor>(); });
        reg.registerProcessor(
            QStringLiteral("phoneme_alignment"), QStringLiteral("mock-hfa"),
            [] { return std::make_unique<MockAlignmentProcessor>(); });
        reg.registerProcessor(
            QStringLiteral("failing_task"), QStringLiteral("failing"),
            [] { return std::make_unique<FailingProcessor>(); });
    }

    void testMockPitchExtraction() {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("pitch_extraction");
        step.processorId = QStringLiteral("mock-rmvpe");
        opts.steps = {step};

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("item0");
        ctxs[0].audioPath = QStringLiteral("/fake/audio.wav");
        ctxs[0].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QVERIFY(ctxs[0].layers.count(QStringLiteral("pitch")));
        const auto &pitch = ctxs[0].layers[QStringLiteral("pitch")];
        QVERIFY(pitch.contains("f0"));
        QCOMPARE(pitch["f0"].size(), 100);
    }

    void testMockMidiTranscription() {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("midi_transcription");
        step.processorId = QStringLiteral("mock-game");
        opts.steps = {step};

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("item0");
        ctxs[0].audioPath = QStringLiteral("/fake/audio.wav");
        ctxs[0].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QVERIFY(ctxs[0].layers.count(QStringLiteral("midi")));
        const auto &midi = ctxs[0].layers[QStringLiteral("midi")];
        QVERIFY(midi.is_array());
        QCOMPARE(midi.size(), 2);
        QCOMPARE(midi[0]["pitch"].get<int>(), 60);
        QCOMPARE(midi[1]["pitch"].get<int>(), 64);
    }

    void testMockPhonemeAlignment() {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("phoneme_alignment");
        step.processorId = QStringLiteral("mock-hfa");
        opts.steps = {step};

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("item0");
        ctxs[0].audioPath = QStringLiteral("/fake/audio.wav");
        ctxs[0].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};
        ctxs[0].layers[QStringLiteral("grapheme")] = {{"text", "hello"}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QVERIFY(ctxs[0].layers.count(QStringLiteral("phoneme")));
        const auto &phone = ctxs[0].layers[QStringLiteral("phoneme")];
        QVERIFY(phone.contains("phonemes"));
        QCOMPARE(phone["phonemes"].get<std::string>(), "h eh l ow");
    }

    void testMultiStepPipeline() {
        PipelineRunner runner;
        PipelineOptions opts;

        StepConfig alignStep;
        alignStep.taskName = QStringLiteral("phoneme_alignment");
        alignStep.processorId = QStringLiteral("mock-hfa");

        StepConfig pitchStep;
        pitchStep.taskName = QStringLiteral("pitch_extraction");
        pitchStep.processorId = QStringLiteral("mock-rmvpe");

        StepConfig midiStep;
        midiStep.taskName = QStringLiteral("midi_transcription");
        midiStep.processorId = QStringLiteral("mock-game");

        opts.steps = {alignStep, pitchStep, midiStep};

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("item0");
        ctxs[0].audioPath = QStringLiteral("/fake/audio.wav");
        ctxs[0].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};
        ctxs[0].layers[QStringLiteral("grapheme")] = {{"text", "hello"}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QVERIFY(ctxs[0].layers.count(QStringLiteral("phoneme")));
        QVERIFY(ctxs[0].layers.count(QStringLiteral("pitch")));
        QVERIFY(ctxs[0].layers.count(QStringLiteral("midi")));
        QCOMPARE(ctxs[0].completedSteps().size(), 3);
    }

    void testFailingProcessor() {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("failing_task");
        step.processorId = QStringLiteral("failing");
        opts.steps = {step};

        std::vector<PipelineContext> ctxs(1);
        ctxs[0].itemId = QStringLiteral("item0");
        ctxs[0].layers[QStringLiteral("input")] = {{"data", "test"}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QCOMPARE(ctxs[0].status, PipelineContext::Status::Error);
        QVERIFY(!ctxs[0].discardReason.isEmpty());
    }

    void testPipelineWithValidatorDiscard() {
        PipelineRunner runner;
        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("pitch_extraction");
        step.processorId = QStringLiteral("mock-rmvpe");
        step.validator = [](const PipelineContext &ctx, const TaskSpec &, QString &reason) {
            if (ctx.audioPath.isEmpty()) {
                reason = QStringLiteral("No audio path");
                return PipelineContext::Status::Discarded;
            }
            return PipelineContext::Status::Active;
        };
        opts.steps = {step};

        std::vector<PipelineContext> ctxs(2);
        ctxs[0].itemId = QStringLiteral("has_audio");
        ctxs[0].audioPath = QStringLiteral("/fake/audio.wav");
        ctxs[0].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};
        ctxs[1].itemId = QStringLiteral("no_audio");
        ctxs[1].layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};

        auto res = runner.run(opts, ctxs);
        QVERIFY(res.ok());
        QCOMPARE(ctxs[0].status, PipelineContext::Status::Active);
        QCOMPARE(ctxs[1].status, PipelineContext::Status::Discarded);
        QCOMPARE(ctxs[1].discardReason, QStringLiteral("No audio path"));
    }

    void testPipelineContextBuildInputForPitch() {
        PipelineContext ctx;
        ctx.audioPath = QStringLiteral("/audio/slice_001.wav");
        ctx.layers[QStringLiteral("audio")] = {{"sampleRate", 44100}};

        TaskSpec spec;
        spec.taskName = QStringLiteral("pitch_extraction");
        spec.inputs = {{QStringLiteral("audio_in"), QStringLiteral("audio")}};

        auto result = ctx.buildTaskInput(spec);
        QVERIFY(result.ok());
        QCOMPARE(result.value().audioPath, QStringLiteral("/audio/slice_001.wav"));
        QVERIFY(result.value().layers.count(QStringLiteral("audio_in")));
    }

    void testPipelineContextApplyPitchOutput() {
        PipelineContext ctx;
        ctx.itemId = QStringLiteral("slice_001");

        TaskSpec spec;
        spec.taskName = QStringLiteral("pitch_extraction");
        spec.outputs = {{QStringLiteral("f0_out"), QStringLiteral("pitch")}};

        TaskOutput output;
        std::vector<float> f0(100, 440.0f);
        output.layers[QStringLiteral("f0_out")] = {{"f0", f0}, {"timestep", 0.01}};

        ctx.applyTaskOutput(spec, output);
        QVERIFY(ctx.layers.count(QStringLiteral("pitch")));
        QVERIFY(ctx.layers[QStringLiteral("pitch")].contains("f0"));
    }
};

QTEST_GUILESS_MAIN(TestMockInference)
#include "TestMockInference.moc"
