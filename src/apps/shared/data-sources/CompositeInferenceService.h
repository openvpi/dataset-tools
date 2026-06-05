#pragma once
/// @file CompositeInferenceService.h
/// @brief Composite IInferenceService that delegates to specific engines.

#include <dsfw/infer/IInferenceService.h>

namespace HFA {
    class HFA;
}
namespace Rmvpe {
    class Rmvpe;
}
namespace Game {
    class Game;
}

namespace dstools {

    /// @brief IInferenceService implementation that composes HFA, RMVPE, and GAME engines.
    class CompositeInferenceService : public dsfw::infer::IInferenceService {
    public:
        CompositeInferenceService(HFA::HFA *hfa, Rmvpe::Rmvpe *rmvpe, Game::Game *game);

        bool hasForcedAlignment() const override;
        bool hasPitchExtraction() const override;
        bool hasMidiTranscription() const override;

        bool runForcedAlignment(const std::filesystem::path &audioPath, const std::string &language,
                                const std::vector<std::string> &nonSpeechPh, const std::string &lyrics,
                                std::vector<dsfw::infer::AlignedWord> &words) override;

        bool runPitchExtraction(const std::filesystem::path &audioPath, float timestep,
                                dsfw::infer::PitchResult &result) override;

        bool runMidiTranscription(const std::filesystem::path &audioPath,
                                  std::vector<dsfw::infer::MidiNote> &notes) override;

    private:
        HFA::HFA *m_hfa;
        Rmvpe::Rmvpe *m_rmvpe;
        Game::Game *m_game;
    };

} // namespace dstools
