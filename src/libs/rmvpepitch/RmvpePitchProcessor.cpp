#include "RmvpePitchProcessor.h"

#include <rmvpe-infer/Rmvpe.h>

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

#include <nlohmann/json.hpp>

namespace dstools {

RmvpePitchProcessor::RmvpePitchProcessor() = default;
RmvpePitchProcessor::~RmvpePitchProcessor() = default;

QString RmvpePitchProcessor::processorId() const {
    return QStringLiteral("rmvpe");
}

QString RmvpePitchProcessor::displayName() const {
    return QStringLiteral("RMVPE Pitch Extraction");
}

TaskSpec RmvpePitchProcessor::taskSpec() const {
    return {QStringLiteral("pitch_extraction"), {}, {{QStringLiteral("pitch"), QStringLiteral("pitch")}}};
}

Result<void> RmvpePitchProcessor::initialize(IModelManager & /*mm*/,
                                              const ProcessorConfig &modelConfig) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto path = QString::fromStdString(modelConfig.value("path", std::string{}));
    const int gpuIndex = modelConfig.value("deviceId", -1);

    if (path.isEmpty()) {
        return Err("RmvpePitchProcessor: missing 'path' in model config");
    }

    auto provider = gpuIndex < 0 ? dstools::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dstools::infer::ExecutionProvider::DML;
#else
                                 : dstools::infer::ExecutionProvider::CPU;
#endif

    m_rmvpe = std::make_unique<Rmvpe::Rmvpe>();
    auto result = m_rmvpe->load(path.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_rmvpe.reset();
        return Err(result.error());
    }
    return Ok();
}

void RmvpePitchProcessor::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rmvpe.reset();
}

Result<TaskOutput> RmvpePitchProcessor::process(const TaskInput &input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_rmvpe) {
        return Err<TaskOutput>("RmvpePitchProcessor: model not loaded");
    }

    std::vector<Rmvpe::RmvpeRes> res;
    auto result = m_rmvpe->get_f0(input.audioPath.toStdWString(), 0.03f, res, nullptr);
    if (!result) {
        return Err<TaskOutput>(result.error());
    }

    std::vector<float> f0All;
    for (const auto &r : res) {
        f0All.insert(f0All.end(), r.f0.begin(), r.f0.end());
    }

    TaskOutput output;
    output.layers[QStringLiteral("pitch")] = nlohmann::json(f0All);
    return Ok(std::move(output));
}

// Self-registration
static TaskProcessorRegistry::Registrar<RmvpePitchProcessor> s_reg(
    QStringLiteral("pitch_extraction"), QStringLiteral("rmvpe"));

} // namespace dstools
