#include "SlicerService.h"

#include <audio-util/PathCompat.h>
#include <audio-util/Slicer.h>
#include <dsfw/PathUtils.h>
#include <dstools/AudioDecoder.h>

#include <QFileInfo>

namespace dstools {

Result<SliceResult> SlicerService::slice(const QString &audioPath, double threshold, int minLength,
                                          int minInterval, int hopSize, int maxSilKept) {
    if (!QFileInfo::exists(audioPath)) {
        return Result<SliceResult>::Error("Audio file does not exist: " + audioPath.toStdString());
    }

    std::filesystem::path path = audioPath.toStdWString();
    SndfileHandle sf = AudioUtil::openSndfile(path);
    if (sf.error()) {
        return Result<SliceResult>::Error("Failed to open audio file: " + dsfw::PathUtils::toUtf8(path));
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
        return Result<SliceResult>::Error(slicer.errorMessage());
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

    SliceResult result;
    result.sampleRate = sampleRate;
    result.chunks.reserve(markers.size());
    for (const auto &marker : markers) {
        result.chunks.emplace_back(marker.first, marker.second);
    }

    return Result<SliceResult>::Ok(std::move(result));
}

std::vector<double> SlicerService::computeSlicePoints(const std::vector<float> &samples, int sampleRate,
                                                       const AudioUtil::SlicerParams &params) {
    if (samples.empty() || sampleRate <= 0)
        return {};

    auto slicer = AudioUtil::Slicer::fromMilliseconds(sampleRate, params);
    auto markers = slicer.slice(samples);

    std::vector<double> newPoints;
    for (size_t i = 0; i + 1 < markers.size(); ++i) {
        int64_t boundary = markers[i].second;
        if (markers[i + 1].first > boundary)
            boundary = (boundary + markers[i + 1].first) / 2;
        double cutTime = static_cast<double>(boundary) / sampleRate;
        newPoints.push_back(cutTime);
    }
    return newPoints;
}

std::vector<float> SlicerService::loadAndMixAudio(const QString &filePath, int *outSampleRate) {
    audio::AudioDecoder decoder;
    if (!decoder.open(filePath))
        return {};

    auto fmt = decoder.format();
    int sr = fmt.sampleRate();
    int channels = fmt.channels();

    if (outSampleRate)
        *outSampleRate = sr;

    std::vector<float> allSamples;
    allSamples.reserve(decoder.length() / sizeof(float));
    constexpr int kBufSize = 4096;
    std::vector<float> buffer(kBufSize);
    while (true) {
        int read = decoder.read(buffer.data(), 0, kBufSize);
        if (read <= 0)
            break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    if (channels > 1) {
        size_t numFrames = allSamples.size() / channels;
        std::vector<float> mono(numFrames);
        for (size_t i = 0; i < numFrames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += allSamples[i * channels + c];
            mono[i] = sum / static_cast<float>(channels);
        }
        return mono;
    }
    return allSamples;
}

} // namespace dstools