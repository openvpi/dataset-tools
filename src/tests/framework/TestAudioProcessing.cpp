#include <QTest>
#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/audio/AudioFormatInfo.h>

#include <QDir>
#include <QTemporaryDir>

#include <sndfile.hh>

#include <cmath>

using namespace dsfw::audio;

class TestAudioProcessing : public QObject {
    Q_OBJECT

    static QString generateSineWav(const QString& dir, int sampleRate, int channels, float frequency, float durationSec,
                                   int bitDepth = 16) {
        QString filename = QStringLiteral("sine_%1_%2_%3bit.wav").arg(sampleRate).arg(channels).arg(bitDepth);
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

private slots:
    void testProbeNonExistent() {
        auto pipeline = AudioPipeline::create();
        auto result = pipeline.probe("/nonexistent/file.wav");
        QVERIFY(!result.ok());
    }

    void testProbeWav() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 2, 440.0f, 1.0f);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.probe(wavPath.toStdString());
        QVERIFY(result.ok());

        auto fmt = result.value();
        QCOMPARE(fmt.sampleRate, 44100);
        QCOMPARE(fmt.channelCount, 2);
        QVERIFY(fmt.durationSec > 0.0);
    }

    void testDecodeToMonoFloat() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 22050, 1, 440.0f, 0.5f, 16);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), 22050);
        QVERIFY(result.ok());

        auto buffer = result.value();
        auto floats = buffer.floats();
        QVERIFY(floats.size() > 0);

        bool hasNonZero = false;
        for (auto s : floats) {
            if (std::abs(s) > 1e-6f) {
                hasNonZero = true;
                break;
            }
        }
        QVERIFY(hasNonZero);
    }

    void testDecodeStereoToMono() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 2, 440.0f, 1.0f);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), 44100);
        QVERIFY(result.ok());

        auto buffer = result.value();
        auto floats = buffer.floats();
        QVERIFY(floats.size() > 0);
    }

    void testDecodeToMonoFloatResample() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 0.5f);

        auto pipeline = AudioPipeline::create();
        auto probeResult = pipeline.probe(wavPath.toStdString());
        QVERIFY(probeResult.ok());
        QCOMPARE(probeResult.value().sampleRate, 44100);

        auto decodeResult = pipeline.decodeToMonoFloat(wavPath.toStdString(), 22050);
        QVERIFY(decodeResult.ok());
        auto buffer = decodeResult.value();
        QVERIFY(buffer.floats().size() > 0);
    }

    void testDecodeDifferentSampleRates() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        for (int sr : {22050, 44100, 48000}) {
            QString wavPath = generateSineWav(tmpDir.path(), sr, 1, 440.0f, 0.5f);
            auto pipeline = AudioPipeline::create();
            auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), sr);
            QVERIFY2(result.ok(), qPrintable(QStringLiteral("Failed to decode %1Hz WAV").arg(sr)));

            auto buffer = result.value();
            QVERIFY(buffer.floats().size() > 0);
        }
    }

    void testDecode24BitWav() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 0.5f, 24);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), 44100);
        QVERIFY(result.ok());

        auto buffer = result.value();
        QVERIFY(buffer.floats().size() > 0);
    }

    void testDecodeFloatWav() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 0.5f, 32);

        auto pipeline = AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(wavPath.toStdString(), 44100);
        QVERIFY(result.ok());

        auto buffer = result.value();
        QVERIFY(buffer.floats().size() > 0);
    }
};

QTEST_GUILESS_MAIN(TestAudioProcessing)
#include "TestAudioProcessing.moc"