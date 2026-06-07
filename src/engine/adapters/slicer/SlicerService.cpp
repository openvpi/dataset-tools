#include "SlicerService.h"

#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/signal/Slicer.h>
#include <dsfw/PathUtils.h>

#include <QFileInfo>

namespace dstools {

using namespace dsfw;

Result<SliceResult> SlicerService::slice(const QString& audioPath, double threshold, int minLength, int minInterval,
                                         int hopSize, int maxSilKept) {
    if (!QFileInfo::exists(audioPath)) {
        return Result<SliceResult>::Error("Audio file does not exist: " + audioPath.toStdString());
    }

    std::filesystem::path path = audioPath.toStdWString();
    auto pipeline = dsfw::audio::AudioPipeline::create();
    auto probeResult = pipeline.probe(dsfw::PathUtils::toUtf8(path));
    if (!probeResult.ok()) {
        return Result<SliceResult>::Error("Failed to probe audio file: " + probeResult.error());
    }
    int sampleRate = probeResult.value().sampleRate;

    auto decodeResult = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(path), sampleRate);
    if (!decodeResult.ok()) {
        return Result<SliceResult>::Error("Failed to decode audio: " + decodeResult.error());
    }
    auto buffer = decodeResult.value();
    auto floats = buffer.floats();
    std::vector<float> monoSamples(floats.begin(), floats.end());

    dsfw::signal::SlicerParams params;
    params.threshold = threshold;
    params.minLength = minLength;
    params.minInterval = minInterval;
    params.hopSize = hopSize;
    params.maxSilKept = maxSilKept;

    auto slicer = dsfw::signal::Slicer::fromMilliseconds(sampleRate, params);
    if (slicer.errorCode() != dsfw::signal::SlicerError::Ok) {
        return Result<SliceResult>::Error(slicer.errorMessage());
    }

    auto markers = slicer.slice(monoSamples);

    SliceResult result;
    result.sampleRate = sampleRate;
    result.chunks.reserve(markers.size());
    for (const auto& marker : markers) {
        result.chunks.emplace_back(marker.first, marker.second);
    }

    return Result<SliceResult>::Ok(std::move(result));
}

std::vector<double> SlicerService::computeSlicePoints(const std::vector<float>& samples, int sampleRate,
                                                      const dsfw::signal::SlicerParams& params) {
    if (samples.empty() || sampleRate <= 0)
        return {};

    auto slicer = dsfw::signal::Slicer::fromMilliseconds(sampleRate, params);
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

std::vector<float> SlicerService::loadAndMixAudio(const QString& filePath, int* outSampleRate) {
    auto pipeline = dsfw::audio::AudioPipeline::create();
    std::filesystem::path path = filePath.toStdWString();
    auto probeResult = pipeline.probe(dsfw::PathUtils::toUtf8(path));
    if (!probeResult.ok())
        return {};
    int sr = probeResult.value().sampleRate;

    auto decodeResult = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(path), sr);
    if (!decodeResult.ok())
        return {};

    if (outSampleRate)
        *outSampleRate = sr;

    auto buffer = decodeResult.value();
    auto floats = buffer.floats();
    return std::vector<float>(floats.begin(), floats.end());
}

} // namespace dstools