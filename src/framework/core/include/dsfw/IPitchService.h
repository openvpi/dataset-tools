#pragma once

#include <QString>
#include <dstools/Result.h>
#include <functional>
#include <vector>

namespace dstools {

struct PitchResult {
    std::vector<float> f0;
    double hopMs = 0.0;
    int sampleRate = 0;
};

class IPitchService {
public:
    virtual ~IPitchService() = default;

    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual void unloadModel() = 0;

    virtual Result<PitchResult> extractPitch(const QString &audioPath) = 0;

    virtual Result<void> extractPitchWithProgress(const QString &audioPath,
                                                   std::vector<std::vector<float>> &f0Frames,
                                                   const std::function<void(int)> &progress = nullptr) = 0;
};

} // namespace dstools
