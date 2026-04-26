#include "MoeCurveProcessor.h"

#include "dsfw/InferenceModelProvider.h"

#include <dsfw/PathUtils.h>
#include <QFileInfo>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>
#include <dstools/AudioDecoder.h>
#include <dstools/ExecutionProvider.h>
#include <moe-infer/Moe.h>
#include <soxr.h>

namespace dstools {

    MoeCurveProcessor::MoeCurveProcessor(QObject *parent) : QObject(parent) {
    }

    MoeCurveProcessor::~MoeCurveProcessor() = default;

    void MoeCurveProcessor::setModelPath(const std::filesystem::path &path) {
        m_modelPath = path;
        m_engine.reset();
    }

    bool MoeCurveProcessor::isModelLoaded() const {
        return m_engine && m_engine->isOpen();
    }

    void MoeCurveProcessor::runInference(const std::filesystem::path &audioPath,
                                         std::shared_ptr<std::atomic<bool>> alive) {
        if (!*alive)
            return;

        if (!m_engine) {
            m_engine = std::make_unique<Moe::Moe>();
            auto result = m_engine->load(m_modelPath, dstools::ExecutionProvider::CPU, 0);
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

            dstools::audio::AudioDecoder decoder;
            if (!decoder.open(dsfw::PathUtils::fromStdPath(audioPath))) {
                auto error =
                    QStringLiteral("Failed to open audio file: %1").arg(dsfw::PathUtils::fromStdPath(audioPath));
                if (self)
                    emit self->errorOccurred(error);
                if (onError)
                    onError(error);
                return;
            }

            auto format = decoder.format();
            int srcSampleRate = format.sampleRate();
            int srcChannels = format.channels();
            qint64 totalBytes = decoder.length();
            if (totalBytes <= 0 || srcChannels <= 0) {
                auto error = QStringLiteral("Audio file has no samples.");
                if (self)
                    emit self->errorOccurred(error);
                if (onError)
                    onError(error);
                return;
            }
            int64_t totalSamples = totalBytes / static_cast<qint64>(sizeof(float)) / srcChannels;
            if (totalSamples <= 0) {
                auto error = QStringLiteral("Audio file has no samples.");
                if (self)
                    emit self->errorOccurred(error);
                if (onError)
                    onError(error);
                return;
            }

            if (!self || !*alive)
                return;

            std::vector<float> rawSamples(static_cast<size_t>(totalSamples * srcChannels));
            int samplesRead = decoder.read(rawSamples.data(), 0, static_cast<int>(rawSamples.size()));
            decoder.close();

            if (samplesRead < 0) {
                auto error = QStringLiteral("Failed to read audio samples.");
                if (self)
                    emit self->errorOccurred(error);
                if (onError)
                    onError(error);
                return;
            }

            int actualSamples = srcChannels > 0 ? samplesRead / srcChannels : samplesRead;

            if (actualSamples <= 0) {
                auto error = QStringLiteral("Failed to read audio samples.");
                if (self)
                    emit self->errorOccurred(error);
                if (onError)
                    onError(error);
                return;
            }

            if (!self || !*alive)
                return;

            std::vector<float> monoInput(static_cast<size_t>(actualSamples));
            if (srcChannels > 1) {
                for (int i = 0; i < actualSamples; ++i) {
                    float sum = 0.0f;
                    for (int ch = 0; ch < srcChannels; ++ch) {
                        sum += rawSamples[static_cast<size_t>(i * srcChannels + ch)];
                    }
                    monoInput[static_cast<size_t>(i)] = sum / static_cast<float>(srcChannels);
                }
            } else {
                for (int i = 0; i < actualSamples; ++i) {
                    monoInput[static_cast<size_t>(i)] = rawSamples[static_cast<size_t>(i)];
                }
            }

            std::vector<float> mono44100;
            if (srcSampleRate == 44100) {
                mono44100 = std::move(monoInput);
            } else {
                double ratio = 44100.0 / srcSampleRate;
                size_t estimatedOutFrames = static_cast<size_t>(actualSamples * ratio + 0.5);

                soxr_error_t error = nullptr;
                soxr_io_spec_t ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
                auto qualitySpec = soxr_quality_spec(SOXR_HQ, 0);
                auto runtimeSpec = soxr_runtime_spec(1);

                soxr_t resampler = soxr_create(srcSampleRate, 44100.0, 1, &error, &ioSpec, &qualitySpec, &runtimeSpec);

                if (!resampler || error) {
                    if (self)
                        emit self->errorOccurred(QStringLiteral("Failed to create resampler."));
                    return;
                }

                mono44100.resize(estimatedOutFrames + 4096);
                size_t inputDone = 0;
                size_t outputDone = 0;

                error = soxr_process(resampler, monoInput.data(), monoInput.size(), &inputDone, mono44100.data(),
                                     mono44100.size(), &outputDone);

                if (error) {
                    soxr_delete(resampler);
                    if (self)
                        emit self->errorOccurred(QStringLiteral("Resampling error: %1").arg(soxr_strerror(error)));
                    return;
                }

                size_t flushDone = 0;
                size_t remaining = mono44100.size() - outputDone;
                if (remaining > 0) {
                    error = soxr_process(resampler, nullptr, 0, nullptr, mono44100.data() + outputDone, remaining,
                                         &flushDone);
                }

                soxr_delete(resampler);
                mono44100.resize(outputDone + flushDone);
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
