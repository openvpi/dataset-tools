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
        m_path.toStdString();
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
    QVector<std::tuple<short, short>> nullVectorThumbnail;
    peakCacheMipmap.swap(nullVectorThumbnail);

    sampleRate = sf.samplerate();
    channels = sf.channels();
    frames = sf.frames();
    auto totalSize = frames * channels;
    // qDebug() << frames;

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
        }

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

    // Create mipmap from peak cache
    short min = 0;
    short max = 0;
    bool hasTail = false;
    for (int i = 0; i < peakCache.count(); i++) {
        if ((i + 1) % mipmapScale == 0) {
            peakCacheMipmap.append(std::make_pair(min, max));
            min = 0;
            max = 0;
            hasTail = false;
        }
        else {
            auto frame = peakCache.at(i);
            auto frameMin = std::get<0>(frame);
            auto frameMax = std::get<1>(frame);
            if (frameMin < min)
                min = frameMin;
            if (frameMax > max)
                max = frameMax;
            hasTail = true;
        }
    }
    if (hasTail)
        peakCacheMipmap.append(std::make_pair(min, max));

    // QThread::msleep(3000);
    emit finished(true, nullptr);
}