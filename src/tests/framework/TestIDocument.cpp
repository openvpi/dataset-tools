#include <QTest>
#include <dsfw/IDocument.h>

using namespace dstools;

static const auto MockFormat = DocumentFormatId(42);

class MockDocument : public IDocument {
public:
    QString m_filePath;
    bool m_modified = false;
    bool m_loadShouldFail = false;
    bool m_saveShouldFail = false;
    bool m_closed = false;
    int m_entries = 0;
    double m_duration = 0.0;

    QString filePath() const override { return m_filePath; }
    DocumentFormatId format() const override { return MockFormat; }
    QString formatDisplayName() const override { return QStringLiteral("MockFormat"); }

    bool load(const QString &path, std::string &error) override {
        if (m_loadShouldFail) {
            error = "mock load error";
            return false;
        }
        m_filePath = path;
        return true;
    }

    bool save(std::string &error) override {
        if (m_saveShouldFail) {
            error = "mock save error";
            return false;
        }
        m_modified = false;
        return true;
    }

    bool saveAs(const QString &path, std::string &error) override {
        if (m_saveShouldFail) {
            error = "mock saveAs error";
            return false;
        }
        m_filePath = path;
        m_modified = false;
        return true;
    }

    void close() override { m_closed = true; }

    bool isModified() const override { return m_modified; }
    void setModified(bool modified) override { m_modified = modified; }

    DocumentInfo info() const override {
        return {m_filePath, MockFormat, QDateTime::currentDateTime(), m_modified};
    }

    int entryCount() const override { return m_entries; }
    double durationSec() const override { return m_duration; }
};

class TestIDocument : public QObject {
    Q_OBJECT
private slots:
    void testLifecycle();
    void testInfo();
    void testLoadError();
    void testSaveError();
    void testSaveAsError();
    void testEntryCountAndDuration();
    void testDocumentFormatId();
    void testRegisterDocumentFormat();
};

void TestIDocument::testLifecycle() {
    MockDocument doc;

    // load
    std::string err;
    QVERIFY(doc.load("/tmp/test.txt", err));
    QCOMPARE(doc.filePath(), QString("/tmp/test.txt"));

    // modify
    QVERIFY(!doc.isModified());
    doc.setModified(true);
    QVERIFY(doc.isModified());

    // save
    QVERIFY(doc.save(err));
    QVERIFY(!doc.isModified());

    // modify again and saveAs
    doc.setModified(true);
    QVERIFY(doc.saveAs("/tmp/other.txt", err));
    QCOMPARE(doc.filePath(), QString("/tmp/other.txt"));
    QVERIFY(!doc.isModified());

    // close
    doc.close();
    QVERIFY(doc.m_closed);
}

void TestIDocument::testInfo() {
    MockDocument doc;
    doc.m_filePath = "/tmp/info.txt";
    doc.setModified(true);

    DocumentInfo di = doc.info();
    QCOMPARE(di.filePath, QString("/tmp/info.txt"));
    QCOMPARE(di.format, MockFormat);
    QVERIFY(di.isModified);
}

void TestIDocument::testLoadError() {
    MockDocument doc;
    doc.m_loadShouldFail = true;

    std::string err;
    QVERIFY(!doc.load("/tmp/fail.txt", err));
    QVERIFY(!err.empty());
}

void TestIDocument::testSaveError() {
    MockDocument doc;
    doc.m_saveShouldFail = true;

    std::string err;
    QVERIFY(!doc.save(err));
    QVERIFY(!err.empty());
}

void TestIDocument::testSaveAsError() {
    MockDocument doc;
    doc.m_saveShouldFail = true;

    std::string err;
    QVERIFY(!doc.saveAs("/tmp/fail.txt", err));
    QVERIFY(!err.empty());
}

void TestIDocument::testEntryCountAndDuration() {
    MockDocument doc;
    QCOMPARE(doc.entryCount(), 0);
    QCOMPARE(doc.durationSec(), 0.0);

    doc.m_entries = 5;
    doc.m_duration = 12.5;
    QCOMPARE(doc.entryCount(), 5);
    QCOMPARE(doc.durationSec(), 12.5);
}

void TestIDocument::testDocumentFormatId() {
    DocumentFormatId invalid;
    QVERIFY(!invalid.isValid());
    QCOMPARE(invalid.id(), -1);

    DocumentFormatId a(1);
    DocumentFormatId b(1);
    DocumentFormatId c(2);
    QVERIFY(a.isValid());
    QVERIFY(a == b);
    QVERIFY(a != c);
    QVERIFY(a < c);
}

void TestIDocument::testRegisterDocumentFormat() {
    auto id1 = registerDocumentFormat("TestFmtA");
    auto id2 = registerDocumentFormat("TestFmtA");
    auto id3 = registerDocumentFormat("TestFmtB");

    QVERIFY(id1.isValid());
    QCOMPARE(id1, id2);
    QVERIFY(id1 != id3);
}

QTEST_GUILESS_MAIN(TestIDocument)
#include "TestIDocument.moc"
