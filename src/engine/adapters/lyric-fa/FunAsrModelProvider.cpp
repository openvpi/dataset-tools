#include "FunAsrModelProvider.h"

#include "AsrPipeline.h"

namespace dstools {

FunAsrModelProvider::FunAsrModelProvider(dsfw::ModelTypeId typeId, const QString& name)
    : m_typeId(typeId), m_displayName(name), m_status(dsfw::ModelStatus::Unloaded) {
}

FunAsrModelProvider::~FunAsrModelProvider() = default;

LyricFA::Asr* FunAsrModelProvider::asr() const {
    return m_asr.get();
}

dsfw::ModelTypeId FunAsrModelProvider::type() const {
    return m_typeId;
}

QString FunAsrModelProvider::displayName() const {
    return m_displayName;
}

dsfw::Result<void> FunAsrModelProvider::load(const QString& modelPath, int gpuIndex) {
    m_status = dsfw::ModelStatus::Loading;

    auto provider = dsfw::infer::ExecutionProvider::CPU;
#ifdef ONNXRUNTIME_ENABLE_DML
    if (gpuIndex >= 0)
        provider = dsfw::infer::ExecutionProvider::DML;
#endif

    m_asr = std::make_unique<LyricFA::Asr>(modelPath, provider, gpuIndex);
    if (!m_asr->initialized()) {
        m_asr.reset();
        m_status = dsfw::ModelStatus::Error;
        return dsfw::Result<void>::Error("Failed to initialize FunASR model");
    }

    m_status = dsfw::ModelStatus::Ready;
    return dsfw::Ok();
}

void FunAsrModelProvider::unload() {
    m_asr.reset();
    m_status = dsfw::ModelStatus::Unloaded;
}

dsfw::ModelStatus FunAsrModelProvider::status() const {
    return m_status;
}

int64_t FunAsrModelProvider::estimatedMemoryBytes() const {
    return 0;
}

}  // namespace dstools
