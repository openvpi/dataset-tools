#include <QTest>
#include <QTemporaryDir>
#include <dstools/DsTextTypes.h>

using namespace dstools;

class TestDsTextDocument : public QObject {
    Q_OBJECT
private slots:
    void load_v2_roundtrip();
    void sort_boundaries();
    void nextId();
    void groups_lookup();
    void curve_layer();
};

void TestDsTextDocument::load_v2_roundtrip() {
    const QByteArray json = R"({
        "version": "2.0.0",
        "audio": {"path": "test.wav", "in": 1000000, "out": 5000000},
        "layers": [
            {
                "name": "phones",
                "boundaries": [
                    {"id": 1, "pos": 0, "text": "a"},
                    {"id": 2, "pos": 500000, "text": "b"},
                    {"id": 3, "pos": 1000000, "text": "c"}
                ]
            }
        ],
        "curves": [
            {"name": "f0", "timestep": 5000, "values": [261600, 262000, 0]}
        ],
        "groups": [[1, 2]]
    })";

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path1 = tmp.filePath("doc.dstext");
    {
        QFile f(path1);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
    }

    auto res = DsTextDocument::load(path1);
    QVERIFY(res.ok());
    const auto &doc = res.value();
    QCOMPARE(doc.version, "2.0.0");
    QCOMPARE(doc.audio.path, "test.wav");
    QCOMPARE(doc.audio.in, 1000000);
    QCOMPARE(doc.audio.out, 5000000);
    QCOMPARE(doc.layers.size(), size_t(1));
    QCOMPARE(doc.layers[0].name, "phones");
    QCOMPARE(doc.layers[0].boundaries.size(), size_t(3));
    QCOMPARE(doc.layers[0].boundaries[0].text, "a");
    QCOMPARE(doc.curves.size(), size_t(1));
    QCOMPARE(doc.curves[0].values.size(), size_t(3));
    QCOMPARE(doc.groups.size(), size_t(1));

    // Save and reload
    const QString path2 = tmp.filePath("doc2.dstext");
    auto saveRes = doc.save(path2);
    QVERIFY(saveRes.ok());

    auto res2 = DsTextDocument::load(path2);
    QVERIFY(res2.ok());
    const auto &doc2 = res2.value();
    QCOMPARE(doc2.version, doc.version);
    QCOMPARE(doc2.audio.path, doc.audio.path);
    QCOMPARE(doc2.audio.in, doc.audio.in);
    QCOMPARE(doc2.audio.out, doc.audio.out);
    QCOMPARE(doc2.layers.size(), doc.layers.size());
    QCOMPARE(doc2.layers[0].boundaries.size(), doc.layers[0].boundaries.size());
    for (size_t i = 0; i < doc.layers[0].boundaries.size(); ++i) {
        QCOMPARE(doc2.layers[0].boundaries[i].id, doc.layers[0].boundaries[i].id);
        QCOMPARE(doc2.layers[0].boundaries[i].pos, doc.layers[0].boundaries[i].pos);
        QCOMPARE(doc2.layers[0].boundaries[i].text, doc.layers[0].boundaries[i].text);
    }
    QCOMPARE(doc2.curves.size(), doc.curves.size());
    QCOMPARE(doc2.groups.size(), doc.groups.size());
}

void TestDsTextDocument::sort_boundaries() {
    IntervalLayer layer;
    layer.name = "test";
    layer.boundaries = {
        {3, 300, "c"},
        {1, 100, "a"},
        {2, 200, "b"},
    };
    layer.sortBoundaries();
    QCOMPARE(layer.boundaries[0].pos, TimePos(100));
    QCOMPARE(layer.boundaries[1].pos, TimePos(200));
    QCOMPARE(layer.boundaries[2].pos, TimePos(300));
}

void TestDsTextDocument::nextId() {
    IntervalLayer layer;
    QCOMPARE(layer.nextId(), 1);

    layer.boundaries = {{5, 0, ""}, {3, 0, ""}, {10, 0, ""}};
    QCOMPARE(layer.nextId(), 11);
}

void TestDsTextDocument::groups_lookup() {
    DsTextDocument doc;
    doc.groups = {{1, 2, 3}, {4, 5}};

    const auto *g = doc.findGroup(2);
    QVERIFY(g != nullptr);
    QCOMPARE(g->size(), size_t(3));

    QVERIFY(doc.findGroup(99) == nullptr);

    auto bound = doc.boundIdsOf(2);
    QCOMPARE(bound.size(), size_t(2));
    QVERIFY(std::find(bound.begin(), bound.end(), 1) != bound.end());
    QVERIFY(std::find(bound.begin(), bound.end(), 3) != bound.end());

    auto empty = doc.boundIdsOf(99);
    QVERIFY(empty.empty());
}

void TestDsTextDocument::curve_layer() {
    const QByteArray json = R"({
        "version": "2.0.0",
        "audio": {"path": "", "in": 0, "out": 0},
        "layers": [],
        "curves": [
            {"name": "f0", "timestep": 5000, "values": [100, 200, 300]}
        ]
    })";

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path1 = tmp.filePath("curve.dstext");
    {
        QFile f(path1);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(json);
    }

    auto res = DsTextDocument::load(path1);
    QVERIFY(res.ok());
    const auto &doc = res.value();
    QCOMPARE(doc.curves.size(), size_t(1));
    QCOMPARE(doc.curves[0].name, "f0");
    QCOMPARE(doc.curves[0].timestep, TimePos(5000));
    QCOMPARE(doc.curves[0].values, (std::vector<int32_t>{100, 200, 300}));

    const QString path2 = tmp.filePath("curve2.dstext");
    QVERIFY(doc.save(path2).ok());

    auto res2 = DsTextDocument::load(path2);
    QVERIFY(res2.ok());
    QCOMPARE(res2.value().curves[0].values, doc.curves[0].values);
}

QTEST_MAIN(TestDsTextDocument)
#include "TestDsTextDocument.moc"
