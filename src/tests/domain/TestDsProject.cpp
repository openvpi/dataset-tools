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
    void slicerStateRoundtrip();
};

void TestDsProject::defaultConstruction() {
    DsProject proj;
    QVERIFY(proj.workingDirectory().isEmpty());
    QVERIFY(proj.filePath().isEmpty());
    QVERIFY(proj.items().empty());
    QCOMPARE(proj.exportConfig().hopSize, 512);
    QCOMPARE(proj.exportConfig().sampleRate, 44100);
    QCOMPARE(proj.exportConfig().resampleRate, 44100);
    QVERIFY(!proj.exportConfig().includeDiscarded);
    QCOMPARE(proj.slicerState().params.threshold, -40.0);
    QCOMPARE(proj.slicerState().params.minLength, 5000);
}

void TestDsProject::saveLoadRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    proj.setWorkingDirectory(QStringLiteral("/test/dir"));

    ExportConfig exp;
    exp.hopSize = 256;
    exp.sampleRate = 22050;
    exp.resampleRate = 22050;
    exp.formats = {QStringLiteral("ds"), QStringLiteral("csv")};
    exp.includeDiscarded = true;
    proj.setExportConfig(exp);

    SlicerState state;
    state.params.threshold = -30.0;
    state.params.minLength = 3000;
    state.audioFiles = {QStringLiteral("audio/test.wav")};
    state.slicePoints[QStringLiteral("audio/test.wav")] = {1.5, 3.2, 5.7};
    proj.setSlicerState(std::move(state));

    QString path = tmpDir.path() + QStringLiteral("/test.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));
    QVERIFY(error.isEmpty());

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());
    QCOMPARE(loaded.workingDirectory(), DsProject::fromPosixPath(QStringLiteral("/test/dir")));

    const auto &lexp = loaded.exportConfig();
    QCOMPARE(lexp.hopSize, 256);
    QCOMPARE(lexp.sampleRate, 22050);
    QCOMPARE(lexp.resampleRate, 22050);
    QCOMPARE(lexp.formats.size(), 2);
    QVERIFY(lexp.includeDiscarded);

    const auto &ls = loaded.slicerState();
    QCOMPARE(ls.params.threshold, -30.0);
    QCOMPARE(ls.params.minLength, 3000);
    QCOMPARE(ls.audioFiles.size(), 1);
    QCOMPARE(ls.slicePoints.size(), 1);
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

    QCOMPARE(proj.exportConfig().hopSize, 512);
    QCOMPARE(proj.exportConfig().sampleRate, 44100);
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
#ifdef Q_OS_WIN
    QCOMPARE(DsProject::toPosixPath(QStringLiteral("C:\\Users\\test")), QStringLiteral("C:/Users/test"));
    QCOMPARE(DsProject::fromPosixPath(QStringLiteral("C:/Users/test")), QStringLiteral("C:\\Users\\test"));
#else
    // QDir::fromNativeSeparators / toNativeSeparators are no-ops on non-Windows
    QCOMPARE(DsProject::toPosixPath(QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/local/bin"));
    QCOMPARE(DsProject::fromPosixPath(QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/local/bin"));
#endif
    QCOMPARE(DsProject::toPosixPath(QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/local/bin"));
}

void TestDsProject::exportConfigRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    ExportConfig exp;
    exp.formats = {QStringLiteral("ds"), QStringLiteral("csv"), QStringLiteral("json")};
    exp.hopSize = 128;
    exp.sampleRate = 16000;
    exp.resampleRate = 16000;
    exp.includeDiscarded = true;
    proj.setExportConfig(exp);

    QString path = tmpDir.path() + QStringLiteral("/export.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());

    const auto &lexp = loaded.exportConfig();
    QCOMPARE(lexp.formats.size(), 3);
    QCOMPARE(lexp.hopSize, 128);
    QCOMPARE(lexp.sampleRate, 16000);
    QCOMPARE(lexp.resampleRate, 16000);
    QVERIFY(lexp.includeDiscarded);
}

void TestDsProject::slicerStateRoundtrip() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    DsProject proj;
    SlicerState state;
    state.params.threshold = -35.0;
    state.params.minLength = 4000;
    state.params.minInterval = 200;
    state.params.hopSize = 20;
    state.params.maxSilence = 300;
    state.audioFiles = {QStringLiteral("audio/a.wav"), QStringLiteral("audio/b.wav")};
    state.slicePoints[QStringLiteral("audio/a.wav")] = {1.0, 2.5, 4.0};
    proj.setSlicerState(std::move(state));

    QString path = tmpDir.path() + QStringLiteral("/slicer.dsproj");
    QString error;
    QVERIFY(proj.save(path, error));

    auto loaded = DsProject::load(path, error);
    QVERIFY(error.isEmpty());

    const auto &ls = loaded.slicerState();
    QCOMPARE(ls.params.threshold, -35.0);
    QCOMPARE(ls.params.minLength, 4000);
    QCOMPARE(ls.params.minInterval, 200);
    QCOMPARE(ls.params.hopSize, 20);
    QCOMPARE(ls.params.maxSilence, 300);
    QCOMPARE(ls.audioFiles.size(), 2);
    QCOMPARE(ls.slicePoints.size(), 1);
    QCOMPARE(ls.slicePoints.at(DsProject::fromPosixPath(QStringLiteral("audio/a.wav"))).size(), 3);
}

QTEST_MAIN(TestDsProject)
#include "TestDsProject.moc"
