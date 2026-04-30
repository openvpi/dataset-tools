#pragma once

#include <QString>
#include <cstdint>

#include "Enumerations.h"

struct SliceJobParams {
    double threshold;
    qint64 minLength;
    qint64 minInterval;
    qint64 hopSize;
    qint64 maxSilKept;
    int outputWaveFormat = WF_INT16_PCM;
    bool saveAudio = true;
    bool saveMarkers = false;
    bool loadMarkers = false;
    bool overwriteMarkers = false;
    int minimumDigits = 3;
};

struct SliceJobResult {
    bool success;
    QString errorMessage;
    int chunksWritten = 0;
};

struct ISliceJobSink {
    virtual ~ISliceJobSink() = default;
    virtual void onInfo(const QString &msg) = 0;
    virtual void onError(const QString &msg) = 0;
};

class SliceJob {
public:
    static SliceJobResult run(const QString &filename, const QString &outPath,
                              const SliceJobParams &params, ISliceJobSink *sink = nullptr);
};
