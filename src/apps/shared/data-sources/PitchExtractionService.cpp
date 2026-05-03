#include "PitchExtractionService.h"

#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>
#include <dstools/TimePos.h>

namespace dstools {

PitchExtractionResult PitchExtractionService::extractPitch(Rmvpe::Rmvpe *rmvpe, const QString &audioPath) {
    PitchExtractionResult result;
    if (!rmvpe || audioPath.isEmpty())
        return result;

    std::vector<Rmvpe::RmvpeRes> results;
    auto rmvpeResult = rmvpe->get_f0(audioPath.toStdWString(), 0.03f, results, nullptr);
    if (!rmvpeResult || results.empty())
        return result;

    const auto &res = results[0];
    result.f0Mhz.resize(res.f0.size());
    for (size_t i = 0; i < res.f0.size(); ++i) {
        result.f0Mhz[i] = static_cast<int32_t>(res.f0[i] * 1000.0f);
    }
    result.timestep = (res.f0.size() > 1 && res.offset >= 0) ? (1.0f / 16000.0f) : 0.005f;
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

void PitchExtractionService::applyPitchToDocument(DsTextDocument &doc, const std::vector<int32_t> &f0Mhz,
                                                    float timestep) {
    if (f0Mhz.empty())
        return;

    CurveLayer *pitchCurve = nullptr;
    for (auto &curve : doc.curves) {
        if (curve.name == QStringLiteral("pitch")) {
            pitchCurve = &curve;
            break;
        }
    }
    if (!pitchCurve) {
        doc.curves.push_back({});
        pitchCurve = &doc.curves.back();
        pitchCurve->name = QStringLiteral("pitch");
    }
    pitchCurve->values = f0Mhz;
    pitchCurve->timestep = dstools::secToUs(static_cast<double>(timestep));
}

void PitchExtractionService::applyMidiToDocument(DsTextDocument &doc,
                                                   const std::vector<Game::GameNote> &notes) {
    if (notes.empty())
        return;

    IntervalLayer *midiLayer = nullptr;
    for (auto &layer : doc.layers) {
        if (layer.name == QStringLiteral("midi")) {
            midiLayer = &layer;
            break;
        }
    }
    if (!midiLayer) {
        doc.layers.push_back({});
        midiLayer = &doc.layers.back();
        midiLayer->name = QStringLiteral("midi");
        midiLayer->type = QStringLiteral("note");
    }

    midiLayer->boundaries.clear();
    int id = 1;
    for (const auto &note : notes) {
        Boundary b;
        b.id = id++;
        b.pos = static_cast<int64_t>(note.onset * 1000000.0);
        int midiNote = static_cast<int>(note.pitch + 0.5);
        int octave = midiNote / 12 - 1;
        int pc = midiNote % 12;
        static const char *pcNames[] = {"C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B"};
        b.text = QStringLiteral("%1%2").arg(pcNames[pc]).arg(octave);
        midiLayer->boundaries.push_back(std::move(b));
    }
}

} // namespace dstools
