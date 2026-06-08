#include "ModelProviderInit.h"

#include <dstools/ModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/infer/InferenceModelProvider.h>

#include <FunAsrModelProvider.h>
#include <game-infer/Game.h>
#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>

namespace dstools {


void registerModelProviders(dsfw::ModelManager &mm) {
    auto asrType = registerModelType("asr");
    mm.registerProvider(
        asrType, std::make_unique<FunAsrModelProvider>(asrType, QStringLiteral("FunASR")));

    auto faType = registerModelType("phoneme_alignment");
    mm.registerProvider(
        faType, std::make_unique<dsfw::InferenceModelProvider<HFA::HFA>>(faType, QStringLiteral("HuBERT-FA")));

    auto pitchType = registerModelType("pitch_extraction");
    mm.registerProvider(
        pitchType, std::make_unique<dsfw::InferenceModelProvider<Rmvpe::Rmvpe>>(pitchType, QStringLiteral("RMVPE")));

    auto midiType = registerModelType("midi_transcription");
    mm.registerProvider(
        midiType, std::make_unique<dsfw::InferenceModelProvider<Game::Game>>(midiType, QStringLiteral("GAME")));
}

} // namespace dstools
