#include <QTest>
#include <dsfw/IDocument.h>

using namespace dstools;

static const auto MockFormat = DocumentFormatId(42);

class MockDocument : public IDocument {
public:
    std::filesystem::path m_filePath;
    bool m_modified = false;
    bool m_loadShouldFail = false;
    bool m_saveShouldFail = false;

    bool isModified() const override { return m_modified; }

    Result<void> load(const std::filesystem::path &path) override {
        if (m_loadShouldFail) {
            return Err("mock load error");
        }
        m_filePath = path;
        return Ok();
    }

    Result<void> save() override {
        if (m_saveShouldFail) {
            return Err("mock save error");
        }
        m_modified = false;
        return Ok();
    }

    Result<void> saveAs(const std::filesystem::path &path) override {
        if (m_saveShouldFail) {
            return Err("mock saveAs error");
        }
        m_filePath = path;
        m_modified = false;
        return Ok();
    }

    IDocumentFormat *format() const override { return nullptr; }
};

class TestIDocument : public QObject {
    Q_OBJECT
private slots:
    void testLifecycle();
    void testLoadError();
    void testSaveError();
    void testSaveAsError();
};

void TestIDocument::testLifecycle() {
    MockDocument doc;

    auto loadResult = doc.load("/tmp/test.txt");
    QVERIFY(loadResult);
    QCOMPARE(doc.m_filePath, std::filesystem::path("/tmp/test.txt"));

    QVERIFY(!doc.isModified());
    doc.m_modified = true;
    QVERIFY(doc.isModified());

    auto saveResult = doc.save();
    QVERIFY(saveResult);
    QVERIFY(!doc.isModified());

    doc.m_modified = true;
    auto saveAsResult = doc.saveAs("/tmp/other.txt");
    QVERIFY(saveAsResult);
    QCOMPARE(doc.m_filePath, std::filesystem::path("/tmp/other.txt"));
    QVERIFY(!doc.isModified());
}

void TestIDocument::testLoadError() {
    MockDocument doc;
    doc.m_loadShouldFail = true;

    auto result = doc.load("/tmp/fail.txt");
    QVERIFY(!result);
    QVERIFY(!result.error().empty());
}

void TestIDocument::testSaveError() {
    MockDocument doc;
    doc.m_saveShouldFail = true;

    auto result = doc.save();
    QVERIFY(!result);
    QVERIFY(!result.error().empty());
}

void TestIDocument::testSaveAsError() {
    MockDocument doc;
    doc.m_saveShouldFail = true;

    auto result = doc.saveAs("/tmp/fail.txt");
    QVERIFY(!result);
    QVERIFY(!result.error().empty());
}

QTEST_GUILESS_MAIN(TestIDocument)
#include "TestIDocument.moc"
