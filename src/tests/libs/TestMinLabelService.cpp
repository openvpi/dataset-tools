#include <QTest>
#include <MinLabelService.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using namespace Minlabel;

class TestMinLabelService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void testRemoveTone_normal();
    void testRemoveTone_empty();
    void testRemoveTone_singleChar();
    void testRemoveTone_noSpaces();

    void testSaveAndLoadLabel();
    void testLoadLabel_nonexistent();
    void testSaveLabel_emptyContent();
    void testSaveLabel_writesLabFile();

    void testConvertLabToJson();
    void testConvertLabToJson_skipExisting();

private:
    QTemporaryDir m_tempDir;
};

void TestMinLabelService::initTestCase() {
    QVERIFY(m_tempDir.isValid());
}

void TestMinLabelService::testRemoveTone_normal() {
    QString result = MinLabelService::removeTone("zhong1 guo2");
    QCOMPARE(result, QString("zhong guo"));
}

void TestMinLabelService::testRemoveTone_empty() {
    QString result = MinLabelService::removeTone("");
    QVERIFY(result.isEmpty());
}

void TestMinLabelService::testRemoveTone_singleChar() {
    QString result = MinLabelService::removeTone("a");
    QVERIFY(result.isEmpty());
}

void TestMinLabelService::testRemoveTone_noSpaces() {
    QString result = MinLabelService::removeTone("abc");
    QVERIFY(result.isEmpty());
}

void TestMinLabelService::testSaveAndLoadLabel() {
    QString jsonPath = m_tempDir.path() + "/test.json";
    QString labPath = m_tempDir.path() + "/test.lab";

    LabelData data;
    data.lab = "zhong1 guo2";
    data.rawText = "中国";
    data.isCheck = true;

    auto saveResult = MinLabelService::saveLabel(jsonPath, labPath, data);
    QVERIFY(saveResult);

    auto loadResult = MinLabelService::loadLabel(jsonPath);
    QVERIFY(loadResult);
    QCOMPARE(loadResult.value().lab, QString("zhong1 guo2"));
    QCOMPARE(loadResult.value().rawText, QString("中国"));
    QVERIFY(loadResult.value().isCheck);
}

void TestMinLabelService::testLoadLabel_nonexistent() {
    auto result = MinLabelService::loadLabel("/nonexistent/path/file.json");
    QVERIFY(!result);
}

void TestMinLabelService::testSaveLabel_emptyContent() {
    QString jsonPath = m_tempDir.path() + "/empty.json";
    QString labPath = m_tempDir.path() + "/empty.lab";

    LabelData data;
    data.lab = "";
    data.rawText = "";

    auto saveResult = MinLabelService::saveLabel(jsonPath, labPath, data);
    QVERIFY(saveResult);

    auto loadResult = MinLabelService::loadLabel(jsonPath);
    QVERIFY(loadResult);
    QVERIFY(loadResult.value().lab.isEmpty());
}

void TestMinLabelService::testSaveLabel_writesLabFile() {
    QString jsonPath = m_tempDir.path() + "/withlab.json";
    QString labPath = m_tempDir.path() + "/withlab.lab";

    LabelData data;
    data.lab = "hello world";
    data.rawText = "test";

    auto saveResult = MinLabelService::saveLabel(jsonPath, labPath, data);
    QVERIFY(saveResult);

    QFile labFile(labPath);
    QVERIFY(labFile.exists());
    QVERIFY(labFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(labFile.readAll());
    QCOMPARE(content, QString("hello world"));
}

void TestMinLabelService::testConvertLabToJson() {
    QString labPath = m_tempDir.path() + "/convert.lab";
    QFile labFile(labPath);
    QVERIFY(labFile.open(QIODevice::WriteOnly | QIODevice::Text));
    labFile.write("ni3 hao3");
    labFile.close();

    auto result = MinLabelService::convertLabToJson(m_tempDir.path());
    QVERIFY(result);
    QCOMPARE(result.value().count, 1);

    QString jsonPath = m_tempDir.path() + "/convert.json";
    auto loadResult = MinLabelService::loadLabel(jsonPath);
    QVERIFY(loadResult);
    QCOMPARE(loadResult.value().lab, QString("ni3 hao3"));
}

void TestMinLabelService::testConvertLabToJson_skipExisting() {
    QString labPath = m_tempDir.path() + "/existing.lab";
    QFile labFile(labPath);
    QVERIFY(labFile.open(QIODevice::WriteOnly | QIODevice::Text));
    labFile.write("test");
    labFile.close();

    QString jsonPath = m_tempDir.path() + "/existing.json";
    QFile jsonFile(jsonPath);
    QVERIFY(jsonFile.open(QIODevice::WriteOnly | QIODevice::Text));
    jsonFile.write("{\"lab\":\"old\"}");
    jsonFile.close();

    auto result = MinLabelService::convertLabToJson(m_tempDir.path());
    QVERIFY(result);
    QCOMPARE(result.value().count, 0);
}

QTEST_GUILESS_MAIN(TestMinLabelService)
#include "TestMinLabelService.moc"
