#ifndef AUDIO_SLICER_SLICER_H
#define AUDIO_SLICER_SLICER_H

#include <vector>
#include <tuple>

#include <QtGlobal>

#include <Api/IAudioDecoder.h>

enum SlicerErrorCode {
    SLICER_OK = 0,
    SLICER_INVALID_ARGUMENT
};

class Slicer {
private:
    double m_threshold;
    qint64 m_hopSize;
    qint64 m_winSize;
    qint64 m_minLength;
    qint64 m_minInterval;
    qint64 m_maxSilKept;
    SlicerErrorCode m_errCode;
    QsApi::IAudioDecoder *m_decoder;

public:
    Slicer(QsApi::IAudioDecoder *decoder, double threshold = -40.0, qint64 minLength = 5000, qint64 minInterval = 300, qint64 hopSize = 20, qint64 maxSilKept = 5000);
    std::vector<std::tuple<qint64, qint64>> slice();
    SlicerErrorCode getErrorCode();
};

#endif //AUDIO_SLICER_SLICER_H