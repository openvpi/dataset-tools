#pragma once

#include <dsfw/IPitchService.h>

#include <memory>
#include <mutex>

namespace Rmvpe {
class Rmvpe;
}

namespace dstools {

class RmvpePitchService : public IPitchService {
public:
    RmvpePitchService();
    ~RmvpePitchService() override;

    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;
    bool isModelLoaded() const override;
    void unloadModel() override;

    Result<PitchResult> extractPitch(const QString &audioPath) override;

    Result<void> extractPitchWithProgress(const QString &audioPath,
                                           std::vector<std::vector<float>> &f0Frames,
                                           const std::function<void(int)> &progress = nullptr) override;

private:
    std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe;
    mutable std::mutex m_mutex;
};

} // namespace dstools
