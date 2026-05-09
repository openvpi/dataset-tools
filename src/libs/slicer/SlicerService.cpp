#include "SlicerService.h"

#include <audio-util/PathCompat.h>
#include <audio-util/Slicer.h>
#include <dstools/PathEncoding.h>

#include <QFileInfo>

dstools::Result<dstools::SliceResult> SlicerService::slice(const QString &audioPath, double threshold, int minLength,
                                                           int minInterval, int hopSize, int maxSilKept) {
    if (!QFileInfo::exists(audioPath)) {
        return dstools::Result<dstools::SliceResult>::Error("Audio file does not exist: " + audioPath.toStdString());
    }

    std::filesystem::path path = audioPath.toStdWString();
    SndfileHandle sf = AudioUtil::openSndfile(path);
    if (sf.error()) {
        return dstools::Result<dstools::SliceResult>::Error("Failed to open audio file: " + dstools::pathToUtf8(path));
    }

    int sampleRate = sf.samplerate();

    AudioUtil::SlicerParams params;
    params.threshold = threshold;
    params.minLength = minLength;
    params.minInterval = minInterval;
    params.hopSize = hopSize;
    params.maxSilKept = maxSilKept;

    auto slicer = AudioUtil::Slicer::fromMilliseconds(sampleRate, params);
    if (slicer.errorCode() != AudioUtil::SlicerError::Ok) {
        return dstools::Result<dstools::SliceResult>::Error(slicer.errorMessage());
    }

    sf.seek(0, SEEK_SET);
    qint64 frames = sf.frames();
    int channels = sf.channels();

    std::vector<float> interleaved(frames * channels);
    sf.readf(interleaved.data(), frames);

    std::vector<float> monoSamples(frames);
    if (channels == 1) {
        monoSamples = std::move(interleaved);
    } else {
        for (qint64 i = 0; i < frames; ++i) {
            double sum = 0.0;
            for (int ch = 0; ch < channels; ++ch) {
                sum += interleaved[i * channels + ch];
            }
            monoSamples[i] = static_cast<float>(sum / channels);
        }
    }

    auto markers = slicer.slice(monoSamples);

    dstools::SliceResult result;
    result.sampleRate = sampleRate;
    result.chunks.reserve(markers.size());
    for (const auto &marker : markers) {
        result.chunks.emplace_back(marker.first, marker.second);
    }

    return dstools::Result<dstools::SliceResult>::Ok(std::move(result));
}
