#include "FunAsrProcessor.h"

#include "Asr.h"

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

namespace dstools {

static TaskProcessorRegistry::Registrar<FunAsrProcessor> s_reg(
    QStringLiteral("asr"), QStringLiteral("funasr"));

FunAsrProcessor::FunAsrProcessor() = default;
FunAsrProcessor::~FunAsrProcessor() = default;

QString FunAsrProcessor::processorId() const {
    return QStringLiteral("funasr");
}

QString FunAsrProcessor::displayName() const {
    return QStringLiteral("FunASR Speech Recognition");
}

TaskSpec FunAsrProcessor::taskSpec() const {
    return {QStringLiteral("asr"), {}, {{QStringLiteral("text"), QStringLiteral("transcription")}}};
}

Result<void> FunAsrProcessor::initialize(IModelManager & /*mm*/,
                                         const ProcessorConfig &modelConfig) {
    std::lock_guard lock(m_mutex);

    const auto path = QString::fromStdString(modelConfig.value("path", ""));
    const int deviceId = modelConfig.value("deviceId", -1);

    auto provider = deviceId < 0 ? FunAsr::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : FunAsr::ExecutionProvider::DML;
#else
                                 : FunAsr::ExecutionProvider::CPU;
#endif

    m_asr = std::make_unique<LyricFA::Asr>(path, provider, deviceId);
    if (!m_asr->initialized()) {
        m_asr.reset();
        return Result<void>::Error("Failed to initialize FunASR model");
    }
    return Result<void>::Ok();
}

void FunAsrProcessor::release() {
    std::lock_guard lock(m_mutex);
    m_asr.reset();
}

Result<TaskOutput> FunAsrProcessor::process(const TaskInput &input) {
    std::lock_guard lock(m_mutex);

    if (!m_asr || !m_asr->initialized()) {
        return Result<TaskOutput>::Error("FunASR model is not initialized");
    }

    std::string msg;
    const bool ok = m_asr->recognize(input.audioPath.toLocal8Bit().toStdString(), msg);
    if (!ok) {
        return Result<TaskOutput>::Error(msg);
    }

    TaskOutput output;
    output.layers[QStringLiteral("text")] = nlohmann::json{{u8"text", msg}};
    return Result<TaskOutput>::Ok(std::move(output));
}

} // namespace dstools
