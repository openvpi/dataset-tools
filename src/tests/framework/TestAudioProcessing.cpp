#include <QTest>
#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

#include <QDir>
#include <QTemporaryDir>

#include <sndfile.hh>

#include <cmath>

using namespace dstools::audio;

class TestAudioProcessing : public QObject {
    Q_OBJECT

    static QString generateSineWav(const QString &dir, int sampleRate, int channels,
                                    float frequency, float durationSec, int bitDepth = 16) {
        QString filename = QStringLiteral("sine_%1_%2_%3bit.wav")
                               .arg(sampleRate)
                               .arg(channels)
                               .arg(bitDepth);
        QString filepath = QDir(dir).filePath(filename);

        int sndFormat = SF_FORMAT_WAV;
        switch (bitDepth) {
            case 16: sndFormat |= SF_FORMAT_PCM_16; break;
            case 24: sndFormat |= SF_FORMAT_PCM_24; break;
            case 32: sndFormat |= SF_FORMAT_PCM_32; break;
            default: sndFormat |= SF_FORMAT_FLOAT; break;
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
    void testWaveFormatDefault() {
        WaveFormat fmt;
        QCOMPARE(fmt.sampleRate(), 44100);
        QCOMPARE(fmt.bitsPerSample(), 16);
        QCOMPARE(fmt.channels(), 2);
    }

    void testWaveFormatCustom() {
        WaveFormat fmt(22050, 32, 1);
        QCOMPARE(fmt.sampleRate(), 22050);
        QCOMPARE(fmt.bitsPerSample(), 32);
        QCOMPARE(fmt.channels(), 1);
    }

    void testWaveFormatBlockAlign() {
        WaveFormat fmt(44100, 16, 2);
        QCOMPARE(fmt.blockAlign(), 4);
    }

    void testWaveFormatBlockAlignMono() {
        WaveFormat fmt(44100, 24, 1);
        QCOMPARE(fmt.blockAlign(), 3);
    }

    void testWaveFormatAvgBytesPerSec() {
        WaveFormat fmt(44100, 16, 2);
        QCOMPARE(fmt.averageBytesPerSecond(), 44100 * 4);
    }

    void testWaveFormatAvgBytesPerSecMono32() {
        WaveFormat fmt(22050, 32, 1);
        QCOMPARE(fmt.averageBytesPerSecond(), 22050 * 4);
    }

    void testAudioDecoderOpenNonExistent() {
        AudioDecoder decoder;
        QVERIFY(!decoder.open(QStringLiteral("/nonexistent/file.wav")));
        QVERIFY(!decoder.isOpen());
    }

    void testAudioDecoderOpenAndClose() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 2, 440.0f, 1.0f);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));
        QVERIFY(decoder.isOpen());

        WaveFormat fmt = decoder.format();
        QCOMPARE(fmt.sampleRate(), 44100);
        QCOMPARE(fmt.channels(), 2);

        decoder.close();
        QVERIFY(!decoder.isOpen());
    }

    void testAudioDecoderReadFloatSamples() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 22050, 1, 440.0f, 0.5f, 16);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));

        std::vector<float> buffer(22050);
        int read = decoder.read(buffer.data(), 0, 22050);
        QVERIFY(read > 0);

        bool hasNonZero = false;
        for (int i = 0; i < read; ++i) {
            if (std::abs(buffer[i]) > 1e-6f) {
                hasNonZero = true;
                break;
            }
        }
        QVERIFY(hasNonZero);

        decoder.close();
    }

    void testAudioDecoderReadStereo() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 2, 440.0f, 1.0f);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));

        WaveFormat fmt = decoder.format();
        QCOMPARE(fmt.channels(), 2);

        std::vector<float> buffer(4096);
        int read = decoder.read(buffer.data(), 0, 4096);
        QVERIFY(read > 0);

        decoder.close();
    }

    void testAudioDecoderLength() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 1.0f);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));

        qint64 len = decoder.length();
        QVERIFY(len > 0);

        decoder.close();
    }

    void testAudioDecoderPosition() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 1.0f);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));

        qint64 initialPos = decoder.position();
        QCOMPARE(initialPos, 0);

        std::vector<float> buffer(1024);
        decoder.read(buffer.data(), 0, 1024);

        qint64 afterRead = decoder.position();
        QVERIFY(afterRead >= 0);

        decoder.close();
    }

    void testAudioDecoderDifferentSampleRates() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        for (int sr : {22050, 44100, 48000}) {
            QString wavPath = generateSineWav(tmpDir.path(), sr, 1, 440.0f, 0.5f);
            AudioDecoder decoder;
            QVERIFY2(decoder.open(wavPath), qPrintable(QStringLiteral("Failed to open %1Hz WAV").arg(sr)));

            WaveFormat fmt = decoder.format();
            QCOMPARE(fmt.sampleRate(), 44100);

            decoder.close();
        }
    }

    void testAudioDecoder24BitWav() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 0.5f, 24);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));
        QVERIFY(decoder.isOpen());

        std::vector<float> buffer(4096);
        int read = decoder.read(buffer.data(), 0, 4096);
        QVERIFY(read > 0);

        decoder.close();
    }

    void testAudioDecoderFloatWav() {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString wavPath = generateSineWav(tmpDir.path(), 44100, 1, 440.0f, 0.5f, 32);

        AudioDecoder decoder;
        QVERIFY(decoder.open(wavPath));
        QVERIFY(decoder.isOpen());

        std::vector<float> buffer(4096);
        int read = decoder.read(buffer.data(), 0, 4096);
        QVERIFY(read > 0);

        decoder.close();
    }
};

QTEST_GUILESS_MAIN(TestAudioProcessing)
#include "TestAudioProcessing.moc"
