#include "AsrService.h"

#include "Asr.h"

#include <dstools/ExecutionProvider.h>

AsrService::~AsrService() = default;

bool AsrService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto provider = gpuIndex < 0 ? FunAsr::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : FunAsr::ExecutionProvider::DML;
#else
                                 : FunAsr::ExecutionProvider::CPU;
#endif

    m_asr = std::make_unique<LyricFA::Asr>(modelPath, provider, gpuIndex);
    if (!m_asr->initialized()) {
        m_asr.reset();
        return false;
    }
    return true;
}

bool AsrService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_asr && m_asr->initialized();
}

void AsrService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_asr.reset();
}

Result<dstools::AsrResult> AsrService::recognize(const QString &audioPath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_asr || !m_asr->initialized()) {
        return Result<dstools::AsrResult>::Error("ASR model is not loaded");
    }

    std::string msg;
    bool ok = m_asr->recognize(audioPath.toLocal8Bit().toStdString(), msg);
    if (!ok) {
        return Result<dstools::AsrResult>::Error(msg);
    }

    dstools::AsrResult result;
    result.text = QString::fromStdString(msg);
    return Result<dstools::AsrResult>::Ok(std::move(result));
}
