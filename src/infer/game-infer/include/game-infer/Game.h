/// @file Game.h
/// @brief GAME audio-to-MIDI transcription and alignment engine.

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

    /// @brief MIDI note output with quantized timing.
    struct GameMidi {
        int note;      ///< MIDI note number.
        int start;     ///< Start time in MIDI ticks.
        int duration;  ///< Duration in MIDI ticks.
    };

    /// @brief Detected note with pitch, timing, and voicing.
    struct GameNote {
        float pitch;     ///< Pitch as MIDI note number (fractional).
        float onset;     ///< Onset time in seconds.
        float duration;  ///< Duration in seconds.
        bool voiced;     ///< Whether the note is voiced.
    };

    /// @brief Input data for phoneme alignment.
    struct AlignInput {
        std::filesystem::path wavPath;       ///< Path to the audio file.
        std::vector<std::string> phSeq;      ///< Phoneme sequence.
        std::vector<float> phDur;            ///< Phoneme durations in seconds.
        std::vector<int> phNum;              ///< Number of phonemes per word.
    };

    /// @brief Configuration options for phoneme alignment.
    struct AlignOptions {
        std::set<std::string> uvVocab = {"AP", "SP", "br", "sil"};  ///< Unvoiced phoneme vocabulary.
        UvWordCond uvWordCond = UvWordCond::Lead;                    ///< Unvoiced word condition.
        UvNoteCond uvNoteCond = UvNoteCond::Predict;                 ///< Unvoiced note condition.
        bool useWordBoundary = true;                                 ///< Whether to use word boundaries.
    };

    class GameModel;

    /// @brief IInferenceEngine implementation for ONNX-based audio transcription, MIDI export, and phoneme alignment.
    class GAME_INFER_EXPORT Game : public dstools::infer::IInferenceEngine {
    public:
        Game();
        ~Game();

        /// @brief Load the GAME ONNX model.
        /// @param modelPath Path to the model directory.
        /// @param provider Execution provider (CPU, DirectML, etc.).
        /// @param device_id Device index for GPU providers.
        /// @return Result indicating success or failure.
        dstools::Result<void> load_model(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) const;

        /// @brief Check whether a model is loaded.
        /// @return True if a model is currently loaded.
        bool is_open() const;

        bool isOpen() const override;
        const char *engineName() const override;
        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

        /// @brief Get the model's target sample rate.
        /// @return Sample rate in Hz.
        int get_target_sample_rate() const;

        /// @brief Get the model's time step.
        /// @return Time step in seconds.
        float get_timestep() const;

        /// @brief Check whether the model includes a dur2bd sub-model.
        /// @return True if dur2bd is available.
        bool has_dur2bd() const;

        /// @brief Get the supported language map.
        /// @return Map of language name to language ID.
        const std::map<std::string, int> &get_language_map() const;

        /// @brief Transcribe audio to quantized MIDI notes.
        /// @param filepath Path to the input audio file.
        /// @param midis Output MIDI notes.
        /// @param tempo Tempo in BPM for MIDI quantization.
        /// @param progressChanged Progress callback receiving percentage (0–100).
        /// @param max_audio_length Maximum audio length in seconds.
        /// @return Result indicating success or failure.
        dstools::Result<void> get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, float tempo,
                                       const std::function<void(int)> &progressChanged,
                                       int max_audio_length = 600) const;

        /// @brief Transcribe audio to continuous note events.
        /// @param filepath Path to the input audio file.
        /// @param notes Output detected notes.
        /// @param progressChanged Progress callback receiving percentage (0–100).
        /// @param max_audio_length Maximum audio length in seconds.
        /// @return Result indicating success or failure.
        dstools::Result<void> get_notes(const std::filesystem::path &filepath, std::vector<GameNote> &notes,
                                        const std::function<void(int)> &progressChanged, int max_audio_length = 600) const;

        /// @brief Align phonemes to detected notes.
        /// @param input Alignment input data.
        /// @param options Alignment configuration.
        /// @param output Output aligned notes.
        /// @return Result indicating success or failure.
        dstools::Result<void> align(const AlignInput &input, const AlignOptions &options, std::vector<AlignedNote> &output) const;

        /// @brief Align phonemes from a CSV file and save results.
        /// @param csvPath Path to the input CSV file.
        /// @param savePath Output directory.
        /// @param saveFilename Output filename.
        /// @param overwrite Whether to overwrite existing output.
        /// @param options Alignment configuration.
        /// @param progressChanged Progress callback receiving percentage (0–100).
        /// @return Result indicating success or failure.
        dstools::Result<void> alignCSV(const std::filesystem::path &csvPath, const std::filesystem::path &savePath,
                                       const std::string &saveFilename, bool overwrite, const AlignOptions &options,
                                       const std::function<void(int)> &progressChanged = nullptr) const;

        /// @brief Set segmenter threshold.
        /// @param threshold Segmentation threshold value.
        void set_seg_threshold(float threshold) const;

        /// @brief Set segmenter radius in seconds.
        /// @param radius Radius in seconds.
        void set_seg_radius_seconds(float radius) const;

        /// @brief Set segmenter radius in frames.
        /// @param radiusFrames Radius in frames.
        void set_seg_radius_frames(float radiusFrames) const;

        /// @brief Set estimator threshold.
        /// @param threshold Estimation threshold value.
        void set_est_threshold(float threshold) const;

        /// @brief Set D3PM timestep schedule.
        /// @param ts Vector of timestep values.
        void set_d3pm_ts(const std::vector<float> &ts) const;

        /// @brief Set the inference language.
        /// @param language Language ID from the language map.
        void set_language(int language) const;

    private:
        std::unique_ptr<GameModel> m_gameModel;
    };
} // namespace Game
