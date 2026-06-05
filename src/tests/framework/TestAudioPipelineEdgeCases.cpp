#include <QTest>
#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/audio/AudioBuffer.h>

#include <QDir>
#include <QTemporaryDir>

#include <sndfile.hh>
#include <cmath>

using namespace dsfw::audio;

/// @brief Edge-case tests for AudioPipeline: empty files, corrupt data,
///        segment decoding, and resample config validation.
class TestAudioPipelineEdgeCases : public QObject {
    Q_OBJECT

    /// Generate a short sine WAV for testing.
    static QString generateSineWav(const QString &dir, int sampleRate, int channels, float frequency,
                                   float durationSec, int bitDepth = 16) {
        QString filename =
            QStringLiteral("sine_%1_%2_%3bit.wav").arg(sampleRate).arg(channels).arg(bitDepth);
        QString filepath = QDir(dir).filePath(filename);

        int sndFormat = SF_FORMAT_WAV;
        switch (bitDepth) {
        case 16:
            sndFormat |= SF_FORMAT_PCM_16;
            break;
        case 24:
            sndFormat |= SF_FORMAT_PCM_24;
            break;
        case 32:
            sndFormat |= SF_FORMAT_PCM_32;
            break;
        default:
            sndFormat |= SF_FORMAT_FLOAT;
            break;
        }

#ifdef _WIN32
        auto pathStr = filepath.toStdWString();
#else
        auto pathStr = filepath.toStdString();
#endif
        SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, channels, sampleRate);

        int numFrames = static_cast<int>(sampleRate * durationSec);
        std::vector<float> buffer(numFrames * channels);
        for (int i = 0; i < numFrames; ++i) {
            float sample = 0.5f * std::sin(2.0f * static_cast<float>(M_PI) * frequency * i / sampleRate);
            for (int c = 0; c < channels; ++c)
                buffer[i * channels + c] = sample;
        }
        wf.write(buffer.data(), numFrames * channels);
        return filepath;
    }

    /// Create an empty (0-byte) file.
    static QString createEmptyFile(const QString &dir, const QString &name) {
        QString filepath = QDir(dir).filePath(name);
        QFile f(filepath);
        f.open(QIODevice::WriteOnly);
        f.close();
        return filepath;
    }

    /// Create a file with garbage (non-audio) data.
    static QString createCorruptFile(const QString &dir, const QString &name) {
        QString filepath = QDir(dir).filePath(name);
        QFile f(filepath);
        f.open(QIODevice::WriteOnly);
        f.write("This is not a valid audio file at all!");
        f.close();
        return filepath;
    }

private slots:
    void testProbeEmptyFile() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString emptyPath = createEmptyFile(tmpDir.path(), "empty.wav");

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.probe(emptyPath.toStdString());
        QVERIFY(!result.ok());
    }

    void testProbeCorruptFile() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString corruptPath = createCorruptFile(tmpDir.path(), "corrupt.wav");

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.probe(corruptPath.toStdString());
        QVERIFY(!result.ok());
    }

    void testDecodeEmptyFile() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString emptyPath = createEmptyFile(tmpDir.path(), "empty.wav");

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(emptyPath.toStdString(), 44100);
        QVERIFY(!result.ok());
    }

    void testDecodeCorruptFile() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString corruptPath = createCorruptFile(tmpDir.path(), "corrupt.wav");

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(corruptPath.toStdString(), 44100);
        QVERIFY(!result.ok());
    }

    void testDecodeSegmentAndResample() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        // 2-second sine at 44100Hz
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 2.0f);

        auto pipeline = AudioPipeline::create();
        ResampleConfig config;
        config.targetSampleRate = 22050;
        config.targetChannelCount = 1;

        // Decode only the first second
        auto result = pipeline.decodeSegmentAndResample(wavPath.toStdString(), 0.0, 1.0, config);
        QVERIFY(result.ok());

        auto buffer = result.value();
        QVERIFY(buffer.floats().size() > 0);
        // Should be roughly 22050 samples (1 second at 22050Hz)
        QVERIFY(buffer.floats().size() < 25000);
        QVERIFY(buffer.floats().size() > 20000);
    }

    void testDecodeSegmentOutOfRange() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 1.0f);

        auto pipeline = AudioPipeline::create();
        ResampleConfig config;
        config.targetSampleRate = 44100;
        config.targetChannelCount = 1;

        // Request segment beyond file duration
        auto result = pipeline.decodeSegmentAndResample(wavPath.toStdString(), 5.0, 10.0, config);
        // Should either fail or return empty buffer
        if (result.ok()) {
            QVERIFY(result.value().floats().size() == 0);
        }
    }

    void testDecodeAndResample() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 2, 440.0f, 1.0f);

        auto pipeline = AudioPipeline::create();
        ResampleConfig config;
        config.targetSampleRate = 16000;
        config.targetChannelCount = 1;

        auto result = pipeline.decodeAndResample(wavPath.toStdString(), config);
        QVERIFY(result.ok());

        auto buffer = result.value();
        QVERIFY(buffer.floats().size() > 0);
        // Should be roughly 16000 samples (1 second at 16000Hz)
        QVERIFY(buffer.floats().size() < 18000);
        QVERIFY(buffer.floats().size() > 14000);
    }

    void testProbeNonExistentPath() {
        auto pipeline = AudioPipeline::create();
        auto result = pipeline.probe("/nonexistent/path/audio.wav");
        QVERIFY(!result.ok());
    }

    void testDecodeNonExistentPath() {
        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat("/nonexistent/path/audio.wav", 44100);
        QVERIFY(!result.ok());
    }

    void testBufferFloatsAccess() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 22050, 1, 440.0f, 0.1f);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), 22050);
        QVERIFY(result.ok());

        auto buffer = result.value();
        auto floats = buffer.floats();
        QVERIFY(floats.size() > 0);

        // Verify data is valid (not all zeros)
        bool hasNonZero = false;
        for (auto s : floats) {
            if (std::abs(s) > 1e-6f) {
                hasNonZero = true;
                break;
            }
        }
        QVERIFY(hasNonZero);
    }
};

QTEST_GUILESS_MAIN(TestAudioPipelineEdgeCases)
#include "TestAudioPipelineEdgeCases.moc"
