#ifndef AUDIO_SLICER_SLICER_H
#define AUDIO_SLICER_SLICER_H

#include <vector>
#include <tuple>

#include <QtGlobal>

class SndfileHandle;

enum SlicerErrorCode {
    SLICER_OK = 0,
    SLICER_INVALID_ARGUMENT,
    SLICER_AUDIO_ERROR
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
    QString m_errMsg;
    SndfileHandle *m_decoder;

public:
    explicit Slicer(SndfileHandle *decoder, double threshold = -40.0, qint64 minLength = 5000, qint64 minInterval = 300, qint64 hopSize = 20, qint64 maxSilKept = 5000);
    std::vector<std::pair<qint64, qint64>> slice();
    SlicerErrorCode getErrorCode();
    QString getErrorMsg();
};

#endif //AUDIO_SLICER_SLICER_H