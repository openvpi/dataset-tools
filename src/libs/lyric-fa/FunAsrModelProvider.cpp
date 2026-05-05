#include "FunAsrModelProvider.h"

#include "AsrPipeline.h"

#include <dstools/ExecutionProvider.h>

namespace dstools {

FunAsrModelProvider::FunAsrModelProvider(ModelTypeId typeId, const QString &name)
    : m_typeId(typeId), m_displayName(name), m_status(ModelStatus::Unloaded) {}

FunAsrModelProvider::~FunAsrModelProvider() = default;

LyricFA::Asr *FunAsrModelProvider::asr() const {
    return m_asr.get();
}

ModelTypeId FunAsrModelProvider::type() const {
    return m_typeId;
}

QString FunAsrModelProvider::displayName() const {
    return m_displayName;
}

Result<void> FunAsrModelProvider::load(const QString &modelPath, int gpuIndex) {
    m_status = ModelStatus::Loading;

    auto provider = infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (gpuIndex >= 0)
        provider = infer::ExecutionProvider::DML;
#endif

    m_asr = std::make_unique<LyricFA::Asr>(modelPath, provider, gpuIndex);
    if (!m_asr->initialized()) {
        m_asr.reset();
        m_status = ModelStatus::Error;
        return Result<void>::Error("Failed to initialize FunASR model");
    }

    m_status = ModelStatus::Ready;
    return Ok();
}

void FunAsrModelProvider::unload() {
    m_asr.reset();
    m_status = ModelStatus::Unloaded;
}

ModelStatus FunAsrModelProvider::status() const {
    return m_status;
}

int64_t FunAsrModelProvider::estimatedMemoryBytes() const {
    return 0;
}

} // namespace dstools
