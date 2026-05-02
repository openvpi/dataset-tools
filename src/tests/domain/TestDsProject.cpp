#include <QTest>
#include <QTemporaryDir>
#include <dstools/DsProject.h>

using namespace dstools;

class TestDsProject : public QObject {
    Q_OBJECT

private slots:
    void defaultConstruction();
    void saveLoadRoundtrip();
    void v1Migration();
    void v3ItemsAndSlices();
    void posixPathConversion();
    void exportConfigRoundtrip();
    void preloadConfigRoundtrip();
};

void TestDsProject::defaultConstruction() {
    DsProject proj;
    QVERIFY(proj.workingDirectory().isEmpty());
    QVERIFY(proj.filePath().isEmpty());
    QVERIFY(proj.items().empty());
    QVERIFY(proj.defaults().taskModels.empty());
    QCOMPARE(proj.defaults().exportConfig.hopSize, 512);
    QCOMPARE(proj.defaults().exportConfig.sampleRate, 44100);
    QCOMPARE(proj.defaults().exportConfig.resampleRate, 44100);
    QVERIFY(!proj.defaults().exportConfig.includeDiscarded);
}

void TestDsProject::saveLoadRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    proj.setWorkingDirectory(QStringLiteral("/test/dir"));
    DsProjectDefaults defaults;

    TaskModelConfig hubertCfg;
    hubertCfg.processorId = QStringLiteral("hubert-fa");
    hubertCfg.modelPath = QStringLiteral("models/hubert.onnx");
    hubertCfg.provider = QStringLiteral("cuda");
    hubertCfg.deviceId = 0;
    defaults.taskModels[QStringLiteral("hubert")] = hubertCfg;

    defaults.exportConfig.hopSize = 256;
    defaults.exportConfig.sampleRate = 22050;
    defaults.exportConfig.resampleRate = 22050;
    defaults.exportConfig.formats = {QStringLiteral("ds"), QStringLiteral("csv")};
    defaults.exportConfig.includeDiscarded = true;

    PreloadConfig preloadCfg;
    preloadCfg.enabled = true;
    preloadCfg.count = 20;
    defaults.preload[QStringLiteral("hubert")] = preloadCfg;

    proj.setDefaults(defaults);

    QString path = tmpDir.path() + QStringLiteral("/test.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));
    QVERIFY(error.isEmpty());

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());
    QCOMPARE(loaded.workingDirectory(), QStringLiteral("/test/dir"));

    const auto &ld = loaded.defaults();
    QCOMPARE(ld.taskModels.size(), 1);
    QVERIFY(ld.taskModels.contains(QStringLiteral("hubert")));
    QCOMPARE(ld.taskModels.at(QStringLiteral("hubert")).processorId, QStringLiteral("hubert-fa"));
    QCOMPARE(ld.taskModels.at(QStringLiteral("hubert")).provider, QStringLiteral("cuda"));

    QCOMPARE(ld.exportConfig.hopSize, 256);
    QCOMPARE(ld.exportConfig.sampleRate, 22050);
    QCOMPARE(ld.exportConfig.resampleRate, 22050);
    QCOMPARE(ld.exportConfig.formats.size(), 2);
    QVERIFY(ld.exportConfig.includeDiscarded);

    QCOMPARE(ld.preload.size(), 1);
    QVERIFY(ld.preload.contains(QStringLiteral("hubert")));
    QCOMPARE(ld.preload.at(QStringLiteral("hubert")).enabled, true);
    QCOMPARE(ld.preload.at(QStringLiteral("hubert")).count, 20);
}

void TestDsProject::v1Migration() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QString path = tmpDir.path() + QStringLiteral("/v1.dsproj");
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(R"({
            "version": "1.0.0",
            "workingDirectory": "C:\\Users\\test\\project",
            "defaults": {
                "models": {
                    "asr": "/models/asr.onnx",
                    "hubert": "/models/hubert.onnx"
                },
                "gpuIndex": 1,
                "hopSize": 512,
                "sampleRate": 44100
            }
        })");
    }

    QString error;
    auto proj = DsProject::load(path, error);
    QVERIFY(error.isEmpty());

    const auto &defaults = proj.defaults();
    QCOMPARE(defaults.taskModels.size(), 2);
    QVERIFY(defaults.taskModels.contains(QStringLiteral("asr")));
    QCOMPARE(defaults.taskModels.at(QStringLiteral("asr")).modelPath, QStringLiteral("/models/asr.onnx"));
    QCOMPARE(defaults.taskModels.at(QStringLiteral("hubert")).modelPath, QStringLiteral("/models/hubert.onnx"));

    QCOMPARE(defaults.exportConfig.hopSize, 512);
    QCOMPARE(defaults.exportConfig.sampleRate, 44100);
}

void TestDsProject::v3ItemsAndSlices() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    Item item;
    item.id = QStringLiteral("item_001");
    item.name = QStringLiteral("test_item");
    item.speaker = QStringLiteral("spk1");
    item.language = QStringLiteral("zh");
    item.audioSource = QStringLiteral("audio/test.wav");

    Slice s1;
    s1.id = QStringLiteral("s1");
    s1.name = QStringLiteral("slice1");
    s1.inPos = 0;
    s1.outPos = 1000000;
    item.slices.push_back(s1);

    Slice s2;
    s2.id = QStringLiteral("s2");
    s2.name = QStringLiteral("slice2");
    s2.inPos = 1000000;
    s2.outPos = 2000000;
    s2.status = QStringLiteral("discarded");
    s2.discardReason = QStringLiteral("too short");
    s2.discardedAt = QStringLiteral("pitch_extraction");
    item.slices.push_back(s2);

    std::vector<Item> items;
    items.push_back(std::move(item));
    proj.setItems(std::move(items));

    QString path = tmpDir.path() + QStringLiteral("/items.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());
    QCOMPARE(loaded.items().size(), 1);

    const auto &li = loaded.items()[0];
    QCOMPARE(li.id, QStringLiteral("item_001"));
    QCOMPARE(li.name, QStringLiteral("test_item"));
    QCOMPARE(li.speaker, QStringLiteral("spk1"));
    QCOMPARE(li.language, QStringLiteral("zh"));
    QCOMPARE(li.slices.size(), 2);

    QCOMPARE(li.slices[0].id, QStringLiteral("s1"));
    QCOMPARE(li.slices[0].name, QStringLiteral("slice1"));
    QCOMPARE(li.slices[0].status, QStringLiteral("active"));

    QCOMPARE(li.slices[1].id, QStringLiteral("s2"));
    QCOMPARE(li.slices[1].name, QStringLiteral("slice2"));
    QCOMPARE(li.slices[1].status, QStringLiteral("discarded"));
    QCOMPARE(li.slices[1].discardReason, QStringLiteral("too short"));
    QCOMPARE(li.slices[1].discardedAt, QStringLiteral("pitch_extraction"));
}

void TestDsProject::posixPathConversion() {
    QCOMPARE(DsProject::toPosixPath(QStringLiteral("C:\\Users\\test")), QStringLiteral("C:/Users/test"));
    QCOMPARE(DsProject::fromPosixPath(QStringLiteral("C:/Users/test")), QStringLiteral("C:\\Users\\test"));
    QCOMPARE(DsProject::toPosixPath(QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/local/bin"));
}

void TestDsProject::exportConfigRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    DsProjectDefaults defaults;
    auto &exp = defaults.exportConfig;
    exp.formats = {QStringLiteral("ds"), QStringLiteral("csv"), QStringLiteral("json")};
    exp.hopSize = 128;
    exp.sampleRate = 16000;
    exp.resampleRate = 16000;
    exp.includeDiscarded = true;
    proj.setDefaults(defaults);

    QString path = tmpDir.path() + QStringLiteral("/export.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());

    const auto &lexp = loaded.defaults().exportConfig;
    QCOMPARE(lexp.formats.size(), 3);
    QCOMPARE(lexp.hopSize, 128);
    QCOMPARE(lexp.sampleRate, 16000);
    QCOMPARE(lexp.resampleRate, 16000);
    QVERIFY(lexp.includeDiscarded);
}

void TestDsProject::preloadConfigRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    DsProjectDefaults defaults;
    PreloadConfig cfg1;
    cfg1.enabled = true;
    cfg1.count = 50;
    defaults.preload[QStringLiteral("hubert")] = cfg1;

    PreloadConfig cfg2;
    cfg2.enabled = false;
    cfg2.count = 5;
    defaults.preload[QStringLiteral("rmvpe")] = cfg2;
    proj.setDefaults(defaults);

    QString path = tmpDir.path() + QStringLiteral("/preload.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());

    const auto &lp = loaded.defaults().preload;
    QCOMPARE(lp.size(), 2);
    QCOMPARE(lp.at(QStringLiteral("hubert")).enabled, true);
    QCOMPARE(lp.at(QStringLiteral("hubert")).count, 50);
    QCOMPARE(lp.at(QStringLiteral("rmvpe")).enabled, false);
    QCOMPARE(lp.at(QStringLiteral("rmvpe")).count, 5);
}

QTEST_MAIN(TestDsProject)
#include "TestDsProject.moc"
