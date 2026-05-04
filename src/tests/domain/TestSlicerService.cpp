#include <SlicerService.h>

#include <QDir>
#include <QTemporaryDir>

#include <QtTest/QtTest>

#include <audio-util/Slicer.h>

#include <cmath>

class TestSlicerService : public QObject {
    Q_OBJECT

private slots:
    void testComputeSlicePointsEmptySamples() {
        std::vector<float> samples;
        AudioUtil::SlicerParams params;
        auto points = dstools::SlicerService::computeSlicePoints(samples, 44100, params);
        QVERIFY(points.empty());
    }

    void testComputeSlicePointsZeroSampleRate() {
        std::vector<float> samples(44100, 0.0f);
        AudioUtil::SlicerParams params;
        auto points = dstools::SlicerService::computeSlicePoints(samples, 0, params);
        QVERIFY(points.empty());
    }

    void testComputeSlicePointsSilence() {
        int sr = 44100;
        std::vector<float> samples(sr * 5, 0.0f);
        AudioUtil::SlicerParams params;
        params.threshold = -40.0;
        params.minLength = 4000;
        auto points = dstools::SlicerService::computeSlicePoints(samples, sr, params);
        QVERIFY(points.empty());
    }

    void testComputeSlicePointsToneBurst() {
        int sr = 44100;
        int durationSec = 10;
        std::vector<float> samples(sr * durationSec, 0.0f);

        int tone1Start = sr * 1;
        int tone1End = sr * 3;
        for (int i = tone1Start; i < tone1End && i < static_cast<int>(samples.size()); ++i) {
            samples[i] = 0.9f * std::sin(2.0 * M_PI * 440.0 * i / sr);
        }

        int tone2Start = sr * 5;
        int tone2End = sr * 8;
        for (int i = tone2Start; i < tone2End && i < static_cast<int>(samples.size()); ++i) {
            samples[i] = 0.9f * std::sin(2.0 * M_PI * 440.0 * i / sr);
        }

        AudioUtil::SlicerParams params;
        params.threshold = -30.0;
        params.minLength = 500;
        params.minInterval = 200;
        params.hopSize = 10;
        params.maxSilKept = 2000;

        auto points = dstools::SlicerService::computeSlicePoints(samples, sr, params);

        for (double p : points) {
            QVERIFY(p > 0.0);
            QVERIFY(p < static_cast<double>(durationSec));
        }
    }

    void testExportSlicesEmptySamples() {
        std::vector<float> samples;
        std::vector<double> points = {1.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = QDir::tempPath();
        opts.prefix = "test";

        auto result = dstools::SlicerService::exportSlices(samples, 44100, points, opts);
        QCOMPARE(result.exportedCount, 0);
    }

    void testExportSlicesEmptyPoints() {
        std::vector<float> samples(44100, 0.5f);
        std::vector<double> points;
        dstools::SliceExportOptions opts;
        opts.outputDir = QDir::tempPath();
        opts.prefix = "test";

        auto result = dstools::SlicerService::exportSlices(samples, 44100, points, opts);
        QCOMPARE(result.exportedCount, 0);
    }

    void testExportSlicesSingleSegment() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int sr = 22050;
        std::vector<float> samples(sr * 2, 0.5f);
        std::vector<double> points = {1.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = tmpDir.path();
        opts.prefix = "seg";
        opts.numDigits = 3;
        opts.bitDepth = 16;
        opts.channels = 1;

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.exportedCount, 2);
        QCOMPARE(result.items.size(), static_cast<size_t>(2));
        QVERIFY(result.errors.isEmpty());

        QCOMPARE(result.items[0].id, QString("seg_001"));
        QCOMPARE(result.items[0].slices.size(), static_cast<size_t>(1));
        QCOMPARE(result.items[0].slices[0].status, QString("active"));

        QVERIFY(QFileInfo::exists(QDir(tmpDir.path()).filePath("seg_001.wav")));
        QVERIFY(QFileInfo::exists(QDir(tmpDir.path()).filePath("seg_002.wav")));
    }

    void testExportSlicesTwoSegments() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int sr = 22050;
        std::vector<float> samples(sr * 4, 0.3f);
        std::vector<double> points = {2.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = tmpDir.path();
        opts.prefix = "cut";
        opts.numDigits = 2;
        opts.bitDepth = 16;
        opts.channels = 1;

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.exportedCount, 2);
        QCOMPARE(result.items.size(), static_cast<size_t>(2));

        QCOMPARE(result.items[0].id, QString("cut_01"));
        QCOMPARE(result.items[1].id, QString("cut_02"));

        QVERIFY(QFileInfo::exists(QDir(tmpDir.path()).filePath("cut_01.wav")));
        QVERIFY(QFileInfo::exists(QDir(tmpDir.path()).filePath("cut_02.wav")));
    }

    void testExportSlicesDiscardedIndices() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int sr = 22050;
        std::vector<float> samples(sr * 4, 0.3f);
        std::vector<double> points = {2.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = tmpDir.path();
        opts.prefix = "d";
        opts.numDigits = 1;
        opts.bitDepth = 16;
        opts.channels = 1;
        opts.discardedIndices = {1};

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.exportedCount, 2);
        QCOMPARE(result.items[0].slices[0].status, QString("active"));
        QCOMPARE(result.items[1].slices[0].status, QString("discarded"));
    }

    void testExportSlicesBitDepth24() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int sr = 22050;
        std::vector<float> samples(sr * 2, 0.5f);
        std::vector<double> points = {1.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = tmpDir.path();
        opts.prefix = "bd24";
        opts.bitDepth = 24;
        opts.channels = 1;

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.exportedCount, 2);
        QVERIFY(QFileInfo::exists(QDir(tmpDir.path()).filePath("bd24_001.wav")));
    }

    void testExportSlicesCreatesOutputDir() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QString subDir = tmpDir.path() + "/nested/sub";
        int sr = 22050;
        std::vector<float> samples(sr * 2, 0.5f);
        std::vector<double> points = {1.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = subDir;
        opts.prefix = "nested";
        opts.bitDepth = 16;
        opts.channels = 1;

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.exportedCount, 2);
        QVERIFY(QFileInfo::exists(QDir(subDir).filePath("nested_001.wav")));
    }

    void testExportSlicesItemPositions() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        int sr = 22050;
        std::vector<float> samples(sr * 4, 0.3f);
        std::vector<double> points = {2.0};
        dstools::SliceExportOptions opts;
        opts.outputDir = tmpDir.path();
        opts.prefix = "pos";
        opts.bitDepth = 16;
        opts.channels = 1;

        auto result = dstools::SlicerService::exportSlices(samples, sr, points, opts);
        QCOMPARE(result.items.size(), static_cast<size_t>(2));

        QCOMPARE(result.items[0].slices[0].inPos, static_cast<int64_t>(0));
        QCOMPARE(result.items[0].slices[0].outPos, static_cast<int64_t>(2000000));

        QCOMPARE(result.items[1].slices[0].inPos, static_cast<int64_t>(2000000));
        QCOMPARE(result.items[1].slices[0].outPos, static_cast<int64_t>(4000000));
    }
};

QTEST_MAIN(TestSlicerService)
#include "TestSlicerService.moc"
