#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "GameGlobal.h"

namespace Game
{

    struct GameMidi {
        int note;
        int start;
        int duration;
    };

    struct GameNote {
        float pitch;    // Floating-point MIDI pitch (A4 = 69.0, cents as decimals)
        float onset;    // Onset time in seconds
        float duration; // Duration in seconds
        bool voiced;    // true = voiced note, false = rest/unvoiced
    };

    enum class ExecutionProvider { CPU, CUDA, DML };

    class GameModel; // Forward declaration

    class GAME_INFER_EXPORT Game {
    public:
        Game();
        ~Game();

        bool load_model(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id,
                        std::string &msg) const;
        bool is_open() const;
        void terminate() const;

        int get_target_sample_rate() const;
        float get_timestep() const;
        bool has_dur2bd() const;
        const std::map<std::string, int> &get_language_map() const;

        /**
         * Extract MIDI notes from audio file (tick-quantized, integer pitch).
         * Kept for backward compatibility with GameInfer GUI.
         */
        bool get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, float tempo,
                      std::string &msg, const std::function<void(int)> &progressChanged,
                      int max_audio_length = 600) const;

        /**
         * Extract notes from audio file with floating-point pitch and second-based timing.
         * This is the primary extract API matching Python infer.py extract mode.
         */
        bool get_notes(const std::filesystem::path &filepath, std::vector<GameNote> &notes, std::string &msg,
                       const std::function<void(int)> &progressChanged, int max_audio_length = 600) const;

        // Methods to update model parameters
        void set_seg_threshold(float threshold) const;
        void set_seg_radius_seconds(float radius) const;
        void set_seg_radius_frames(float radiusFrames) const;
        void set_est_threshold(float threshold) const;
        void set_d3pm_ts(const std::vector<float> &ts) const;
        void set_language(int language) const;

    private:
        std::unique_ptr<GameModel> m_gameModel;
    };
} // namespace Game
