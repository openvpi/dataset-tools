#include "CompositeInferenceService.h"

#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>

namespace dstools {


CompositeInferenceService::CompositeInferenceService(HFA::HFA *hfa, Rmvpe::Rmvpe *rmvpe, Game::Game *game)
    : m_hfa(hfa), m_rmvpe(rmvpe), m_game(game) {
}

bool CompositeInferenceService::hasForcedAlignment() const {
    return m_hfa && m_hfa->isOpen();
}

bool CompositeInferenceService::hasPitchExtraction() const {
    return m_rmvpe && m_rmvpe->isOpen();
}

bool CompositeInferenceService::hasMidiTranscription() const {
    return m_game && m_game->isOpen();
}

bool CompositeInferenceService::runForcedAlignment(const std::filesystem::path &audioPath,
                                                    const std::string &language,
                                                    const std::vector<std::string> &nonSpeechPh,
                                                    const std::string &lyrics,
                                                    std::vector<dsfw::infer::AlignedWord> &words) {
    if (!m_hfa || !m_hfa->isOpen())
        return false;

    HFA::WordList hfaWords;
    auto result = m_hfa->recognize(audioPath, language, nonSpeechPh, lyrics, hfaWords);
    if (!result)
        return false;

    words.clear();
    words.reserve(hfaWords.size());
    for (const auto &w : hfaWords) {
        dsfw::infer::AlignedWord word;
        word.text = w.text;
        word.phones.reserve(w.phones.size());
        for (const auto &p : w.phones) {
            word.phones.push_back({p.text, p.start, p.end});
        }
        words.push_back(std::move(word));
    }
    return true;
}

bool CompositeInferenceService::runPitchExtraction(const std::filesystem::path &audioPath, float timestep,
                                                    dsfw::infer::PitchResult &result) {
    if (!m_rmvpe || !m_rmvpe->isOpen())
        return false;

    std::vector<Rmvpe::RmvpeRes> results;
    auto rmvpeResult = m_rmvpe->get_f0(audioPath, timestep, results, nullptr);
    if (!rmvpeResult || results.empty())
        return false;

    const auto &res = results[0];
    result.offset = res.offset;
    result.timestep = timestep;
    result.f0 = res.f0;
    result.uv = res.uv;
    return true;
}

bool CompositeInferenceService::runMidiTranscription(const std::filesystem::path &audioPath,
                                                      std::vector<dsfw::infer::MidiNote> &notes) {
    if (!m_game || !m_game->isOpen())
        return false;

    std::vector<Game::GameNote> gameNotes;
    auto gameResult = m_game->getNotes(audioPath, gameNotes, nullptr);
    if (!gameResult || gameNotes.empty())
        return false;

    notes.clear();
    notes.reserve(gameNotes.size());
    for (const auto &gn : gameNotes) {
        notes.push_back({gn.pitch, gn.onset, gn.duration, gn.voiced});
    }
    return true;
}

} // namespace dstools
