#pragma once

#include <filesystem>
#include <functional>
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

        bool get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, float tempo,
                      std::string &msg, const std::function<void(int)> &progressChanged, int max_audio_length) const;

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
