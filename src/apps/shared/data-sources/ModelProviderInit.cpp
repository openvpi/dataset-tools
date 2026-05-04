#include "ModelProviderInit.h"

#include <dsfw/IModelManager.h>
#include <dsfw/IModelProvider.h>
#include <dsfw/InferenceModelProvider.h>
#include <dstools/ModelManager.h>

#include <FunAsrModelProvider.h>
#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>

namespace dstools {

void registerModelProviders(IModelManager &mm) {
    auto *modelMgr = dynamic_cast<ModelManager *>(&mm);
    if (!modelMgr)
        return;

    auto asrType = registerModelType("asr");
    modelMgr->registerProvider(
        asrType, std::make_unique<FunAsrModelProvider>(asrType, QStringLiteral("FunASR")));

    auto faType = registerModelType("phoneme_alignment");
    modelMgr->registerProvider(
        faType, std::make_unique<InferenceModelProvider<HFA::HFA>>(faType, QStringLiteral("HuBERT-FA")));

    auto pitchType = registerModelType("pitch_extraction");
    modelMgr->registerProvider(
        pitchType, std::make_unique<InferenceModelProvider<Rmvpe::Rmvpe>>(pitchType, QStringLiteral("RMVPE")));

    auto midiType = registerModelType("midi_transcription");
    modelMgr->registerProvider(
        midiType, std::make_unique<InferenceModelProvider<Game::Game>>(midiType, QStringLiteral("GAME")));
}

} // namespace dstools
