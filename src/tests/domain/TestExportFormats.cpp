#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <dsfw/JsonHelper.h>
#include <dstools/ExportFormats.h>

using namespace dstools;

class TestExportFormats : public QObject {
    Q_OBJECT
private:
    nlohmann::json makeValidDsItem() {
        nlohmann::json j;
        j["phonemes"] = nlohmann::json::array({
            {{"start", 0.0},  {"end", 0.15}, {"symbol", "k"}},
            {{"start", 0.15}, {"end", 0.3},  {"symbol", "a"}},
        });
        return j;
    }

    void createDsItemFile(const QString &path, const nlohmann::json &content) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write(QByteArray::fromStdString(content.dump(2)));
        f.close();
    }

    QString readFileContent(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(f.readAll());
    }

private slots:
    void testHtsFormatName() {
        HtsLabelExportFormat fmt;
        QCOMPARE(QString::fromLatin1(fmt.formatName()), QStringLiteral("HTS Labels"));
    }

    void testHtsFormatExtension() {
        HtsLabelExportFormat fmt;
        QCOMPARE(QString::fromLatin1(fmt.formatExtension()), QStringLiteral(".lab"));
    }

    void testHtsExportItem_valid() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcPath = tmp.path() + "/test.dsitem";
        createDsItemFile(srcPath, makeValidDsItem());

        QString dstPath = tmp.path() + "/output.lab";
        HtsLabelExportFormat fmt;
        auto result = fmt.exportItem(srcPath.toStdString(), tmp.path().toStdString(), dstPath.toStdString());
        QVERIFY(result.ok());

        QVERIFY(QFile::exists(dstPath));
        QString content = readFileContent(dstPath);
        QVERIFY(content.contains("0"));
        QVERIFY(content.contains("k"));
        QVERIFY(content.contains("a"));
    }

    void testHtsExportItem_nonExistentSource() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        HtsLabelExportFormat fmt;
        auto result = fmt.exportItem("/nonexistent/file.dsitem", tmp.path().toStdString(),
                                     (tmp.path() + "/out.lab").toStdString());
        QVERIFY(!result.ok());
        QVERIFY(!result.error().empty());
    }

    void testHtsExportItem_invalidOutputPath() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcPath = tmp.path() + "/test.dsitem";
        createDsItemFile(srcPath, makeValidDsItem());

        HtsLabelExportFormat fmt;
        auto result = fmt.exportItem(srcPath.toStdString(), tmp.path().toStdString(), "/invalid_dir/output.lab");
        QVERIFY(!result.ok());
        QVERIFY(!result.error().empty());
    }

    void testHtsExportAll_withItems() {
        QTemporaryDir tmpSrc;
        QVERIFY(tmpSrc.isValid());
        QTemporaryDir tmpDst;
        QVERIFY(tmpDst.isValid());

        createDsItemFile(tmpSrc.path() + "/a.dsitem", makeValidDsItem());
        createDsItemFile(tmpSrc.path() + "/b.dsitem", makeValidDsItem());

        // Create a non-.dsitem file that should be ignored
        QFile nonItem(tmpSrc.path() + "/not_dsitem.txt");
        QVERIFY(nonItem.open(QIODevice::WriteOnly | QIODevice::Text));
        nonItem.write("hello");
        nonItem.close();

        HtsLabelExportFormat fmt;
        auto result = fmt.exportAll(tmpSrc.path().toStdString(), tmpDst.path().toStdString());
        QVERIFY(result.ok());

        QVERIFY(QFile::exists(tmpDst.path() + "/a.lab"));
        QVERIFY(QFile::exists(tmpDst.path() + "/b.lab"));
        QVERIFY(!QFile::exists(tmpDst.path() + "/not_dsitem.lab"));
    }

    void testHtsExportAll_emptyDir() {
        QTemporaryDir tmpSrc;
        QVERIFY(tmpSrc.isValid());
        QTemporaryDir tmpDst;
        QVERIFY(tmpDst.isValid());

        HtsLabelExportFormat fmt;
        auto result = fmt.exportAll(tmpSrc.path().toStdString(), tmpDst.path().toStdString());
        QVERIFY(result.ok());
        QVERIFY(QDir(tmpDst.path()).entryList(QDir::Files).isEmpty());
    }

    void testSinsyFormatName() {
        SinsyXmlExportFormat fmt;
        QCOMPARE(QString::fromLatin1(fmt.formatName()), QStringLiteral("Sinsy XML"));
    }

    void testSinsyFormatExtension() {
        SinsyXmlExportFormat fmt;
        QCOMPARE(QString::fromLatin1(fmt.formatExtension()), QStringLiteral(".xml"));
    }

    void testSinsyExportItem_valid() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcPath = tmp.path() + "/test.dsitem";
        createDsItemFile(srcPath, makeValidDsItem());

        QString dstPath = tmp.path() + "/output.xml";
        SinsyXmlExportFormat fmt;
        auto result = fmt.exportItem(srcPath.toStdString(), tmp.path().toStdString(), dstPath.toStdString());
        QVERIFY(result.ok());

        QVERIFY(QFile::exists(dstPath));
        QString content = readFileContent(dstPath);
        QVERIFY(content.contains("<?xml version=\"1.0\""));
        QVERIFY(content.contains("<sinsy>"));
        QVERIFY(content.contains("</sinsy>"));
        QVERIFY(content.contains("symbol=\"k\""));
        QVERIFY(content.contains("symbol=\"a\""));
        QVERIFY(content.contains("start=\"0\""));
        QVERIFY(content.contains("end=\"0.15\""));
    }

    void testSinsyExportItem_nonExistentSource() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        SinsyXmlExportFormat fmt;
        auto result = fmt.exportItem("/nonexistent/file.dsitem", tmp.path().toStdString(),
                                     (tmp.path() + "/out.xml").toStdString());
        QVERIFY(!result.ok());
        QVERIFY(!result.error().empty());
    }

    void testSinsyExportItem_noPhonemes() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString srcPath = tmp.path() + "/nophon.dsitem";
        createDsItemFile(srcPath, nlohmann::json::object({
                                      {"other", 1}
        }));

        QString dstPath = tmp.path() + "/nophon.xml";
        SinsyXmlExportFormat fmt;
        auto result = fmt.exportItem(srcPath.toStdString(), tmp.path().toStdString(), dstPath.toStdString());
        QVERIFY(result.ok());

        QVERIFY(QFile::exists(dstPath));
        QString content = readFileContent(dstPath);
        QVERIFY(content.contains("<sinsy>"));
        QVERIFY(content.contains("</sinsy>"));
    }

    void testSinsyExportAll_withItems() {
        QTemporaryDir tmpSrc;
        QVERIFY(tmpSrc.isValid());
        QTemporaryDir tmpDst;
        QVERIFY(tmpDst.isValid());

        createDsItemFile(tmpSrc.path() + "/a.dsitem", makeValidDsItem());
        createDsItemFile(tmpSrc.path() + "/b.dsitem", makeValidDsItem());

        SinsyXmlExportFormat fmt;
        auto result = fmt.exportAll(tmpSrc.path().toStdString(), tmpDst.path().toStdString());
        QVERIFY(result.ok());

        QVERIFY(QFile::exists(tmpDst.path() + "/a.xml"));
        QVERIFY(QFile::exists(tmpDst.path() + "/b.xml"));
    }
};

QTEST_GUILESS_MAIN(TestExportFormats)
#include "TestExportFormats.moc"
