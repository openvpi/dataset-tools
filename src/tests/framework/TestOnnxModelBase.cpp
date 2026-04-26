#include <QTest>
#include <dstools/OnnxModelBase.h>

#include <QTemporaryDir>

using namespace dstools::infer;

class TestOnnxModelBase : public QObject {
    Q_OBJECT

private slots:
    void testValidateModelFileNonExistent() {
        auto result = OnnxModelBase::validateModelFile("/nonexistent/path/model.onnx");
        QVERIFY(!result.ok());
    }

    void testValidateModelFileEmpty() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString emptyPath = tmpDir.path() + "/empty.onnx";
        QFile f(emptyPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();

        auto result = OnnxModelBase::validateModelFile(emptyPath.toStdString());
        QVERIFY(!result.ok());
    }

    void testValidateModelFileTooSmall() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString smallPath = tmpDir.path() + "/small.onnx";
        QFile f(smallPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("short", 5);
        f.close();

        auto result = OnnxModelBase::validateModelFile(smallPath.toStdString());
        QVERIFY(!result.ok());
    }

    void testValidateModelFileJsonInsteadOfOnnx() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString jsonPath = tmpDir.path() + "/model.onnx";
        QFile f(jsonPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(R"({"model": "not_onnx"})");
        f.close();

        auto result = OnnxModelBase::validateModelFile(jsonPath.toStdString());
        QVERIFY(!result.ok());
    }

    void testValidateModelFileValidHeader() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString validPath = tmpDir.path() + "/valid.onnx";
        QFile f(validPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        char header[32] = {};
        header[0] = 0x08;
        header[1] = 0x07;
        for (int i = 2; i < 32; ++i)
            header[i] = static_cast<char>(i);
        f.write(header, 32);
        f.close();

        auto result = OnnxModelBase::validateModelFile(validPath.toStdString());
        QVERIFY(result.ok());
    }

    void testValidateModelFileUnknownHeader() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString unknownPath = tmpDir.path() + "/unknown.onnx";
        QFile f(unknownPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        char header[32] = {};
        header[0] = 0x12;
        header[1] = 0x34;
        for (int i = 2; i < 32; ++i)
            header[i] = 0;
        f.write(header, 32);
        f.close();

        auto result = OnnxModelBase::validateModelFile(unknownPath.toStdString());
        QVERIFY(result.ok());
    }
};

QTEST_GUILESS_MAIN(TestOnnxModelBase)
#include "TestOnnxModelBase.moc"
