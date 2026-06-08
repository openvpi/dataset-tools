#include "RmvpePitchProcessor.h"

#include <rmvpe-infer/Rmvpe.h>

#include <dsfw/ConfigTypes.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/infer/ExecutionProvider.h>

#include <nlohmann/json.hpp>

namespace dstools {



RmvpePitchProcessor::RmvpePitchProcessor() = default;
RmvpePitchProcessor::~dsfw::RmvpePitchProcessor() = default;

QString RmvpePitchProcessor::processorId() const {
    return QStringLiteral("rmvpe");
}

QString RmvpePitchProcessor::displayName() const {
    return QStringLiteral("RMVPE Pitch Extraction");
}

TaskSpec RmvpePitchProcessor::taskSpec() const {
    return {QStringLiteral("pitch_extraction"), {}, {{QStringLiteral("pitch"), QStringLiteral("pitch")}}};
}

dsfw::Result<void> RmvpePitchProcessor::initialize(dsfw::ModelManager & /*mm*/,
                                              const dsfw::ProcessorConfig &modelConfig) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto path = configValueString(modelConfig, QStringLiteral("path"));
    const int gpuIndex = static_cast<int>(configValueInt(modelConfig, QStringLiteral("deviceId"), -1));

    if (path.isEmpty()) {
        return dsfw::Err("RmvpePitchProcessor: missing 'path' in model config");
    }

    auto provider = gpuIndex < 0 ? dsfw::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dsfw::infer::ExecutionProvider::DML;
#else
                                 : dsfw::infer::ExecutionProvider::CPU;
#endif

    m_rmvpe = std::make_unique<Rmvpe::Rmvpe>();
    auto result = m_rmvpe->load(path.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_rmvpe.reset();
        return dsfw::Err(result.error());
    }
    return dsfw::Ok();
}

void RmvpePitchProcessor::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rmvpe.reset();
}

dsfw::Result<dsfw::TaskOutput> RmvpePitchProcessor::process(const dsfw::TaskInput &input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_rmvpe) {
        return dsfw::Err<dsfw::TaskOutput>("RmvpePitchProcessor: model not loaded");
    }

    std::vector<Rmvpe::RmvpeRes> res;
    auto result = m_rmvpe->get_f0(input.audioPath.toStdWString(), 0.03f, res, nullptr);
    if (!result) {
        return dsfw::Err<dsfw::TaskOutput>(result.error());
    }

    std::vector<float> f0All;
    for (const auto &r : res) {
        f0All.insert(f0All.end(), r.f0.begin(), r.f0.end());
    }

    dsfw::TaskOutput output;
    output.layers[QStringLiteral("pitch")] = LayerData::fromJson(nlohmann::json(f0All));
    return dsfw::Ok(std::move(output));
}

// Self-registration
static TaskProcessorRegistry::Registrar<dsfw::RmvpePitchProcessor> s_reg(
    QStringLiteral("pitch_extraction"), QStringLiteral("rmvpe"));

} // namespace dstools
