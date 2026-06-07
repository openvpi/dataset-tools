#include "PitchExtractionService.h"

#include "InferBridge.h"

#include <dstools/Constants.h>
#include <dstools/DsPitchDocument.h>
#include <dsfw/signal/curve_tools.h>
#include <dstools/PitchUtils.h>
#include <dstools/DsKeys.h>
#include <dsfw/TimePos.h>
#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/PathUtils.h>

namespace dstools {
    using namespace dsfw;

    using dsfw::signal::expectedFrameCount;
    using dsfw::signal::hopSizeToTimestep;
    using dsfw::signal::resampleCurve;

    PitchExtractionResult PitchExtractionService::extractPitch(Rmvpe::Rmvpe *rmvpe, const QString &audioPath) {
        PitchExtractionResult result;
        if (!rmvpe || audioPath.isEmpty())
            return result;

        std::vector<Rmvpe::RmvpeRes> results;
        auto rmvpeResult = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
        if (!rmvpeResult || results.empty())
            return result;

        const auto &res = results[0];
        std::vector<float> mergedF0(res.f0.size());
        for (size_t i = 0; i < res.f0.size(); ++i) {
            mergedF0[i] = res.f0[i] * 1000.0f;
        }

        auto pipeline = dsfw::audio::AudioPipeline::create();
        auto probeResult = pipeline.probe(dsfw::PathUtils::toUtf8(dsfw::PathUtils::toStdPath(audioPath)));
        if (probeResult.ok()) {
            const int sampleRate = probeResult.value().sampleRate;
            const int64_t audioFrames = probeResult.value().totalFrameCount;

            const TimePos dstTimestepUs = hopSizeToTimestep(constants::kDefaultHopSize, sampleRate);
            const int alignLength = expectedFrameCount(audioFrames, constants::kDefaultHopSize);

            result.f0Mhz = resampleCurve(mergedF0, secToUs(0.01), dstTimestepUs, alignLength);
            result.timestep = static_cast<float>(usToSec(dstTimestepUs));
        } else {
            result.f0Mhz = std::move(mergedF0);
            result.timestep = 0.01f;
        }
        return result;
    }

    MidiTranscriptionResult PitchExtractionService::transcribeMidi(Game::Game *game, const QString &audioPath) {
        MidiTranscriptionResult result;
        if (!game || audioPath.isEmpty())
            return result;

        auto gameResult = game->getNotes(audioPath.toStdWString(), result.notes, nullptr);
        if (!gameResult)
            result.notes.clear();
        return result;
    }

    void PitchExtractionService::applyPitchToDocument(DsTextDocument &doc, const std::vector<float> &f0Mhz,
                                                      float timestep) {
        if (f0Mhz.empty())
            return;

        CurveLayer *pitchCurve = nullptr;
        for (auto &curve : doc.curves) {
            if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch)) {
                pitchCurve = &curve;
                break;
            }
        }
        if (!pitchCurve) {
            doc.curves.push_back({});
            pitchCurve = &doc.curves.back();
            pitchCurve->name = QString::fromUtf8(dstools::keys::layers::pitch);
        }
        pitchCurve->values = f0Mhz;
        pitchCurve->timestep = dstools::secToUs(static_cast<double>(timestep));
    }

    void PitchExtractionService::applyMidiToDocument(DsTextDocument &doc, const std::vector<Game::GameNote> &notes) {
        if (notes.empty())
            return;

        IntervalLayer *midiLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
                midiLayer = &layer;
                break;
            }
        }
        if (!midiLayer) {
            doc.layers.push_back({});
            midiLayer = &doc.layers.back();
            midiLayer->name = QString::fromUtf8(dstools::keys::layers::midi);
            midiLayer->type = QString::fromUtf8(dstools::keys::layers::note);
        }

        midiLayer->boundaries.clear();
        int id = 1;
        for (const auto &gn : notes) {
            pitchlabeler::Note n;
            n.start = static_cast<int64_t>(gn.onset * 1000000.0);
            n.duration = static_cast<int64_t>(gn.duration * 1000000.0);
            n.name = gn.voiced ? midiToNoteString(static_cast<double>(gn.pitch)) : QStringLiteral("rest");

            Boundary b;
            b.id = id++;
            b.pos = n.start;
            b.text = pitchlabeler::DsPitchDocument::serializeNote(n);
            midiLayer->boundaries.push_back(std::move(b));
        }
    }

    void PitchExtractionService::applyMidiToDocument(DsTextDocument &doc,
                                                      const std::vector<dsfw::infer::MidiNote> &notes) {
        if (notes.empty())
            return;

        IntervalLayer *midiLayer = nullptr;
        for (auto &layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
                midiLayer = &layer;
                break;
            }
        }
        if (!midiLayer) {
            doc.layers.push_back({});
            midiLayer = &doc.layers.back();
            midiLayer->name = QString::fromUtf8(dstools::keys::layers::midi);
            midiLayer->type = QString::fromUtf8(dstools::keys::layers::note);
        }

        midiLayer->boundaries.clear();
        int id = 1;
        for (const auto &gn : notes) {
            pitchlabeler::Note n;
            n.start = static_cast<int64_t>(gn.onset * 1000000.0);
            n.duration = static_cast<int64_t>(gn.duration * 1000000.0);
            n.name = gn.voiced ? midiToNoteString(static_cast<double>(gn.pitch)) : QStringLiteral("rest");

            Boundary b;
            b.id = id++;
            b.pos = n.start;
            b.text = pitchlabeler::DsPitchDocument::serializeNote(n);
            midiLayer->boundaries.push_back(std::move(b));
        }
    }

} // namespace dstools
