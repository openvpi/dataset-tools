//
// Created by FlutyDeer on 2023/12/4.
//

#include <QDebug>
#include <QThread>

#include "AudioClipBackgroundWorker.h"
AudioClipBackgroundWorker::AudioClipBackgroundWorker(const QString &path) {
    m_path = path;
}
void AudioClipBackgroundWorker::setPath(const QString &path) {
    m_path = path;
}
void AudioClipBackgroundWorker::run() {
    auto pathStr =
#ifdef Q_OS_WIN
        m_path.toStdWString();
#else
        path.toStdString();
#endif
    //    SndfileHandle sf(pathStr.c_str());
    sf = SndfileHandle(pathStr.c_str());
    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    if (sfErrCode) {
        emit finished(false, sfErrMsg);
        return;
    }

    QVector<std::tuple<short, short>> nullVector;
    peakCache.swap(nullVector);

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();
    auto totalSize = frames * channels;

    int chunkSize = 512;
    std::vector<double> buffer(chunkSize * channels);
    qint64 samplesRead = 0;
    while (samplesRead < frames * channels) {
        samplesRead = sf.read(buffer.data(), chunkSize * channels);
        if (samplesRead == 0) {
            break;
        }
        double sampleMax = 0;
        double sampleMin = 0;
        qint64 framesRead = samplesRead / channels;
        for (qint64 i = 0; i < framesRead; i++) {
            double monoSample = 0.0;
            for (int j = 0; j < channels; j++) {
                monoSample += buffer[i * channels + j] / static_cast<double>(channels);
            }
            if (monoSample > sampleMax)
                sampleMax = monoSample;
            if (monoSample < sampleMin)
                sampleMin = monoSample;

            auto toShortInt = [](double d) -> short{
                if (d < -1)
                    d = -1;
                else if (d > 1)
                    d = 1;
                return static_cast<short>(d * 32767);
            };

            short max = toShortInt(sampleMax);
            short min = toShortInt(sampleMin);

            auto pair = std::make_pair(min, max);
            peakCache.append(pair);
        }
    }

    // QThread::msleep(3000);
    emit finished(true, nullptr);
}