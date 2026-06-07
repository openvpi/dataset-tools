#include "MoeCurveProcessor.h"

#include <dsfw/infer/InferenceModelProvider.h>

#include <dsfw/PathUtils.h>
#include <dsfw/audio/AudioPipeline.h>
#include <QtConcurrent/QtConcurrent>
#include <dsfw/infer/ExecutionProvider.h>
#include <moe-infer/Moe.h>

namespace dstools {

using namespace dsfw;

MoeCurveProcessor::MoeCurveProcessor(QObject* parent) : QObject(parent) {
}

MoeCurveProcessor::~MoeCurveProcessor() = default;

void MoeCurveProcessor::setModelPath(const std::filesystem::path& path) {
    m_modelPath = path;
    m_engine.reset();
}

bool MoeCurveProcessor::isModelLoaded() const {
    return m_engine && m_engine->isOpen();
}

void MoeCurveProcessor::runInference(const std::filesystem::path& audioPath, std::shared_ptr<std::atomic<bool>> alive) {
    if (!*alive)
        return;

    if (!m_engine) {
        m_engine = std::make_unique<Moe::Moe>();
        auto result = m_engine->load(m_modelPath, dsfw::infer::ExecutionProvider::CPU, 0);
        if (!result) {
            emit errorOccurred(QString::fromStdString(result.error()));
            if (onError)
                onError(QString::fromStdString(result.error()));
            return;
        }
    }

    if (!*alive)
        return;

    QPointer<MoeCurveProcessor> self(this);

    QtConcurrent::run([=]() {
        if (!self || !*alive)
            return;

        auto pipeline = dsfw::audio::AudioPipeline::create();
        auto decodeResult = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(audioPath), 44100);
        if (!decodeResult.ok()) {
            auto error = QStringLiteral("Failed to decode audio: %1").arg(QString::fromStdString(decodeResult.error()));
            if (self)
                emit self->errorOccurred(error);
            if (onError)
                onError(error);
            return;
        }

        auto buffer = decodeResult.value();
        auto floats = buffer.floats();
        std::vector<float> mono44100(floats.begin(), floats.end());

        if (mono44100.empty()) {
            auto error = QStringLiteral("Audio file has no samples.");
            if (self)
                emit self->errorOccurred(error);
            if (onError)
                onError(error);
            return;
        }

        if (!self || !*alive)
            return;

        auto predictResult = self->m_engine->predict(mono44100);
        if (!predictResult) {
            auto error = QString::fromStdString(predictResult.error());
            if (self)
                emit self->errorOccurred(error);
            if (onError)
                onError(error);
            return;
        }

        if (!self || !*alive)
            return;

        MouthCurve curve;
        curve.timestep = static_cast<TimePos>(self->m_engine->frameTimestep() * 1e6);
        curve.values = std::move(predictResult.value().curve);

        if (self)
            emit self->curveReady(curve);
        if (onCurveReady)
            onCurveReady(curve);
    });
}

} // namespace dstools
