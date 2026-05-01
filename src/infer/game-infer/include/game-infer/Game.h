#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <dstools/ExecutionProvider.h>
#include <dstools/IInferenceEngine.h>
#include <dstools/Result.h>

#include "GameGlobal.h"
#include "NoteAlignment.h"
#include "WordParser.h"

namespace Game
{
    using ExecutionProvider = dstools::infer::ExecutionProvider;

    struct GameMidi {
        int note;
        int start;
        int duration;
    };

    struct GameNote {
        float pitch;
        float onset;
        float duration;
        bool voiced;
    };

    struct AlignInput {
        std::filesystem::path wavPath;
        std::vector<std::string> phSeq;
        std::vector<float> phDur;
        std::vector<int> phNum;
    };

    struct AlignOptions {
        std::set<std::string> uvVocab = {"AP", "SP", "br", "sil"};
        UvWordCond uvWordCond = UvWordCond::Lead;
        UvNoteCond uvNoteCond = UvNoteCond::Predict;
        bool useWordBoundary = true;
    };

    class GameModel;

    class GAME_INFER_EXPORT Game : public dstools::infer::IInferenceEngine {
    public:
        Game();
        ~Game();

        dstools::Result<void> load_model(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) const;
        bool is_open() const;

        bool isOpen() const override;
        const char *engineName() const override;
        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

        int get_target_sample_rate() const;
        float get_timestep() const;
        bool has_dur2bd() const;
        const std::map<std::string, int> &get_language_map() const;

        dstools::Result<void> get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, float tempo,
                                       const std::function<void(int)> &progressChanged,
                                       int max_audio_length = 600) const;

        dstools::Result<void> get_notes(const std::filesystem::path &filepath, std::vector<GameNote> &notes,
                                        const std::function<void(int)> &progressChanged, int max_audio_length = 600) const;

        dstools::Result<void> align(const AlignInput &input, const AlignOptions &options, std::vector<AlignedNote> &output) const;

        dstools::Result<void> alignCSV(const std::filesystem::path &csvPath, const std::filesystem::path &savePath,
                                       const std::string &saveFilename, bool overwrite, const AlignOptions &options,
                                       const std::function<void(int)> &progressChanged = nullptr) const;

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
