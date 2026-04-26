#include <QtTest/QtTest>

#include <dstools/ProjectPaths.h>
#include <dsfw/PathUtils.h>

class TestProjectPaths : public QObject {
    Q_OBJECT

private slots:
    void testDsItemsDir() {
        auto result = dstools::ProjectPaths::dsItemsDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/dsitems"));
    }

    void testSlicesDir() {
        auto result = dstools::ProjectPaths::slicesDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/slices"));
    }

    void testContextsDir() {
        auto result = dstools::ProjectPaths::contextsDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/contexts"));
    }

    void testDstextDir() {
        auto result = dstools::ProjectPaths::dstextDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/dstext"));
    }

    void testBuildCsvDir() {
        auto result = dstools::ProjectPaths::buildCsvDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/build_csv"));
    }

    void testTranscriptionsCsvPath() {
        auto result = dstools::ProjectPaths::transcriptionsCsvPath("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/build_csv/transcriptions.csv"));
    }

    void testBuildDsDir() {
        auto result = dstools::ProjectPaths::buildDsDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/build_ds"));
    }

    void testSlicerOutputDir() {
        auto result = dstools::ProjectPaths::slicerOutputDir("/tmp/project");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/slicer_output"));
    }

    void testWavsDir() {
        auto result = dstools::ProjectPaths::wavsDir("/tmp/output");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/output/wavs"));
    }

    void testDsDir() {
        auto result = dstools::ProjectPaths::dsDir("/tmp/output");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/output/ds"));
    }

    void testSliceAudioPath() {
        auto result = dstools::ProjectPaths::sliceAudioPath("/tmp/project", "slice_001");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/slices/slice_001.wav"));
    }

    void testSliceContextPath() {
        auto result = dstools::ProjectPaths::sliceContextPath("/tmp/project", "slice_001");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/contexts/slice_001.json"));
    }

    void testSliceDstextPath() {
        auto result = dstools::ProjectPaths::sliceDstextPath("/tmp/project", "slice_001");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/dstext/slice_001.dstext"));
    }

    void testSlicerOutputAudioPath() {
        auto result = dstools::ProjectPaths::slicerOutputAudioPath("/tmp/project", "slice_001");
        QCOMPARE(dsfw::PathUtils::toPosixSeparators(result), QStringLiteral("/tmp/project/dstemp/slicer_output/slice_001.wav"));
    }

    void testEmptyWorkingDir() {
        auto result = dstools::ProjectPaths::slicesDir(QString());
        if (result.isEmpty()) {
            QWARN("Empty working directory produced empty result");
        } else {
            QWARN(qPrintable(QString("Empty working directory: result = %1").arg(result)));
        }
    }
};

QTEST_MAIN(TestProjectPaths)
#include "TestProjectPaths.moc"
