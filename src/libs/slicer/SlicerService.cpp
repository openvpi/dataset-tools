#include "SlicerService.h"

#include "Slicer.h"

#include <QFileInfo>
#include <sndfile.hh>

Result<dstools::SliceResult> SlicerService::slice(const QString &audioPath, double threshold,
                                                   int minLength, int minInterval, int hopSize) {
    if (!QFileInfo::exists(audioPath)) {
        return Result<SliceResult>::Error("Audio file does not exist: " + audioPath.toStdString());
    }

    SndfileHandle sf(audioPath.toLocal8Bit().constData());
    if (sf.error()) {
        return Result<SliceResult>::Error("Failed to open audio file: " + audioPath.toStdString());
    }

    int sampleRate = sf.samplerate();
    qint64 maxSilKept = 5000;

    Slicer slicer(&sf, threshold, minLength, minInterval, hopSize, maxSilKept);
    MarkerList markers = slicer.slice();

    if (slicer.getErrorCode() != SLICER_OK) {
        return Result<SliceResult>::Error(slicer.getErrorMsg().toStdString());
    }

    dstools::SliceResult result;
    result.sampleRate = sampleRate;
    result.chunks.reserve(markers.size());
    for (const auto &marker : markers) {
        result.chunks.emplace_back(marker.first, marker.second);
    }

    return Result<SliceResult>::Ok(std::move(result));
}
