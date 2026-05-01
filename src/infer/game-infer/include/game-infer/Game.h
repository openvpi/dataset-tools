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
        float pitch; // Floating-point MIDI pitch (A4 = 69.0, cents as decimals)
        float onset; // Onset time in seconds
        float duration; // Duration in seconds
        bool voiced; // true = voiced note, false = rest/unvoiced
    };

    /** Input for a single align operation */
    struct AlignInput {
        std::filesystem::path wavPath;
        std::vector<std::string> phSeq;
        std::vector<float> phDur;
        std::vector<int> phNum;
    };

    /** Options controlling align mode behavior */
    struct AlignOptions {
        std::set<std::string> uvVocab = {"AP", "SP", "br", "sil"};
        UvWordCond uvWordCond = UvWordCond::Lead;
        UvNoteCond uvNoteCond = UvNoteCond::Predict;
        bool useWordBoundary = true; // false = no word-note alignment (--no-wb)
    };

    class GameModel; // Forward declaration

    class GAME_INFER_EXPORT Game : public dstools::infer::IInferenceEngine {
    public:
        Game();
        ~Game();

        bool load_model(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id,
                        std::string &msg) const;
        bool is_open() const;

        // IInferenceEngine overrides
        bool isOpen() const override;
        const char *engineName() const override;
        bool load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId,
                  std::string &errorMsg) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

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

        /**
         * Align a single audio file with phoneme transcription.
         * Returns aligned notes with slur flags, matching Python infer.py align mode.
         * Requires dur2bd.onnx to be present in the model directory.
         */
        bool align(const AlignInput &input, const AlignOptions &options, std::vector<AlignedNote> &output,
                   std::string &msg) const;

        /**
         * Process a DiffSinger transcription CSV file.
         * Reads the CSV, runs align on each item, writes updated CSV with note_seq/note_dur/note_slur.
         * @param csvPath Input CSV path
         * @param savePath Output CSV path (if empty, determined by saveFilename relative to csvPath)
         * @param saveFilename Output filename (used when savePath is empty)
         * @param overwrite If true and savePath is empty and saveFilename is empty, overwrite input CSV
         * @param options Align options
         * @param progressChanged Progress callback (0-100)
         */
        bool alignCSV(const std::filesystem::path &csvPath, const std::filesystem::path &savePath,
                      const std::string &saveFilename, bool overwrite, const AlignOptions &options, std::string &msg,
                      const std::function<void(int)> &progressChanged = nullptr) const;

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
