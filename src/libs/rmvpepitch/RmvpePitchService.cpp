#include "RmvpePitchService.h"

#include <rmvpe-infer/Rmvpe.h>

#include <dstools/ExecutionProvider.h>

namespace dstools {

RmvpePitchService::RmvpePitchService() = default;
RmvpePitchService::~RmvpePitchService() = default;

Result<void> RmvpePitchService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto provider = gpuIndex < 0 ? dstools::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dstools::infer::ExecutionProvider::DML;
#else
                                 : dstools::infer::ExecutionProvider::CPU;
#endif

    m_rmvpe = std::make_unique<Rmvpe::Rmvpe>();
    auto result = m_rmvpe->load(modelPath.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_rmvpe.reset();
        return Err(result.error());
    }
    return Ok();
}

bool RmvpePitchService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rmvpe && m_rmvpe->isOpen();
}

void RmvpePitchService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rmvpe.reset();
}

Result<PitchResult> RmvpePitchService::extractPitch(const QString &audioPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_rmvpe) {
        return Err<PitchResult>("Model not loaded");
    }

    std::vector<Rmvpe::RmvpeRes> res;
    auto result = m_rmvpe->get_f0(audioPath.toStdWString(), 0.03f, res, nullptr);
    if (!result) {
        return Err<PitchResult>(result.error());
    }

    PitchResult out;
    out.sampleRate = 16000;
    out.hopMs = 10.0;
    for (const auto &r : res) {
        out.f0.insert(out.f0.end(), r.f0.begin(), r.f0.end());
    }
    return Ok(std::move(out));
}

Result<void> RmvpePitchService::extractPitchWithProgress(const QString &audioPath,
                                                          std::vector<std::vector<float>> &f0Frames,
                                                          const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_rmvpe) {
        return Err("Model not loaded");
    }

    std::vector<Rmvpe::RmvpeRes> res;
    auto result = m_rmvpe->get_f0(audioPath.toStdWString(), 0.03f, res, progress);
    if (!result) {
        return Err(result.error());
    }

    f0Frames.clear();
    f0Frames.reserve(res.size());
    for (auto &r : res) {
        f0Frames.push_back(std::move(r.f0));
    }
    return Ok();
}

} // namespace dstools
