#pragma once

#include <dsfw/IAsrService.h>

#include <memory>
#include <mutex>

namespace FunAsr {
class Model;
}

namespace LyricFA {
class Asr;
}

class AsrService : public dstools::IAsrService {
public:
    AsrService() = default;
    ~AsrService() override;

    bool loadModel(const QString &modelPath, int gpuIndex = -1);
    bool isModelLoaded() const;
    void unloadModel();

    Result<dstools::AsrResult> recognize(const QString &audioPath) override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    mutable std::mutex m_mutex;
};
