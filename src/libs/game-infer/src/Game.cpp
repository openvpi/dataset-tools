#include <game-infer/Game.h>

#include <sndfile.hh>

#include <algorithm>
#include <cmath>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>
#include <game-infer/GameModel.h>

namespace Game
{
    Game::Game(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) {
        m_game = std::make_unique<GameModel>(modelPath, provider, device_id);

        if (!is_open()) {
        }
    }

    Game::~Game() = default;

    bool Game::is_open() const { return m_game && m_game->is_open(); }

    std::vector<double> cumulativeSum(const std::vector<float> &durations) {
        std::vector<double> cumsum(durations.size());
        cumsum[0] = static_cast<double>(durations[0]);
        for (size_t i = 1; i < durations.size(); ++i) {
            cumsum[i] = durations[i] + cumsum[i - 1];
        }
        return cumsum;
    }

    std::vector<int> calculateNoteTicks(const std::vector<float> &note_durations, const float tempo) {
        const std::vector<double> cumsum = cumulativeSum(note_durations);

        std::vector<int> scaled_ticks(cumsum.size());
        for (size_t i = 0; i < cumsum.size(); ++i) {
            scaled_ticks[i] = static_cast<int>(cumsum[i] * tempo * 480 / 60);
        }

        std::vector<int> note_ticks(scaled_ticks.size());
        if (!scaled_ticks.empty()) {
            note_ticks[0] = scaled_ticks[0];
            for (size_t i = 1; i < scaled_ticks.size(); ++i) {
                note_ticks[i] = scaled_ticks[i] - scaled_ticks[i - 1];
            }
        }

        return note_ticks;
    }

    static std::vector<GameMidi> build_midi_note(const int start_tick, const std::vector<float> &durations,
                                                 const std::vector<float> &presence, const std::vector<float> &scores,
                                                 const float tempo) {
        std::vector<GameMidi> midi_data;
        int start_tick_temp = start_tick;

        const std::vector<int> note_ticks = calculateNoteTicks(durations, tempo);

        for (size_t i = 0; i < durations.size(); ++i) {
            if (presence[i] > 0.5) {
                const int pitch = static_cast<int>(std::round(scores[i]));

                const int duration_ticks = note_ticks[i];

                midi_data.push_back(GameMidi{pitch, start_tick_temp, duration_ticks});

                start_tick_temp += duration_ticks;
            } else {
                if (i < note_ticks.size()) {
                    start_tick_temp += note_ticks[i];
                }
            }
        }

        return midi_data;
    }

    static uint64_t calculateSumOfDifferences(const AudioUtil::MarkerList &markers) {
        uint64_t sum = 0;
        for (const auto &[fst, snd] : markers) {
            sum += snd - fst;
        }
        return sum;
    }

    bool Game::get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, const float tempo,
                        std::string &msg, const std::function<void(int)> &progressChanged) const {
        if (!m_game) {
            return false;
        }

        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, 44100);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(44100, 0.02f, 441, 441 * 4, 500, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        int processedFrames = 0;
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;
            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(beginFrame, SEEK_SET);
            std::vector<float> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            std::vector<bool> boundaries;
            std::vector<float> durations;
            std::vector<float> presence;
            std::vector<float> scores;

            const bool success = m_game->forward(tmp, boundaries, durations, presence, scores, msg);
            if (!success)
                return false;

            const auto start_tick = std::max(static_cast<int>(static_cast<double>(fst) / 44100.0 * tempo * 480 / 60),
                                             !midis.empty() ? midis.back().start + midis.back().duration : 0);

            std::vector<GameMidi> temp_midis = build_midi_note(start_tick, durations, presence, scores, tempo);
            midis.insert(midis.end(), temp_midis.begin(), temp_midis.end());

            processedFrames += static_cast<int>(frameCount);
            int progress =
                static_cast<int>(static_cast<float>(processedFrames) / static_cast<float>(slicerFrames) * 100);

            if (progressChanged) {
                progressChanged(progress);
            }
        }
        return true;
    }

    void Game::terminate() const { m_game->terminate(); }

    // Implementation of parameter setting methods
    void Game::set_seg_threshold(const float threshold) const {
        if (m_game) {
            m_game->set_seg_threshold(threshold);
        }
    }

    void Game::set_seg_radius_seconds(const float radius) const {
        if (m_game) {
            m_game->set_seg_radius_seconds(radius);
        }
    }

    void Game::set_est_threshold(const float threshold) const {
        if (m_game) {
            m_game->set_est_threshold(threshold);
        }
    }

    void Game::set_d3pm_ts(const std::vector<float> &ts) const {
        if (m_game) {
            m_game->set_d3pm_ts(ts);
        }
    }

    void Game::set_language(const int language) const {
        if (m_game) {
            m_game->set_language(language);
        }
    }
} // namespace Game
