#include "RmvpePitchProcessor.h"

#include <rmvpe-infer/Rmvpe.h>

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

#include <QDir>
#include <QFileInfoList>

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

Result<BatchOutput> RmvpePitchProcessor::processBatch(const BatchInput &input,
                                                       ProgressCallback progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_rmvpe) {
        return Err<BatchOutput>("RmvpePitchProcessor: model not loaded");
    }

    QDir dir(input.workingDir);
    const auto wavFiles =
        dir.entryInfoList({QStringLiteral("*.wav")}, QDir::Files, QDir::Name);
    const int total = wavFiles.size();

    BatchOutput output;
    output.outputPath = input.workingDir;

    for (int i = 0; i < total; ++i) {
        const auto &fi = wavFiles[i];
        if (progress) {
            progress(i, total, fi.fileName());
        }

        std::vector<Rmvpe::RmvpeRes> res;
        auto result =
            m_rmvpe->get_f0(fi.absoluteFilePath().toStdWString(), 0.03f, res, nullptr);
        if (!result) {
            ++output.failedCount;
            continue;
        }

        std::vector<float> f0All;
        for (const auto &r : res) {
            f0All.insert(f0All.end(), r.f0.begin(), r.f0.end());
        }

        // Write pitch JSON next to the WAV file.
        const auto outPath =
            dir.filePath(fi.completeBaseName() + QStringLiteral(".pitch.json"));
        QFile outFile(outPath);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            const auto json = nlohmann::json(f0All).dump();
            outFile.write(QByteArray::fromStdString(json));
            ++output.processedCount;
        } else {
            ++output.failedCount;
        }
    }

    if (progress && total > 0) {
        progress(total, total, QStringLiteral("Done"));
    }
    return Ok(std::move(output));
}

// Self-registration
static TaskProcessorRegistry::Registrar<RmvpePitchProcessor> s_reg(
    QStringLiteral("pitch_extraction"), QStringLiteral("rmvpe"));

} // namespace dstools
