#pragma once
/// @file IInferenceService.h
/// @brief High-level inference service interface abstracting specific engines.

#include <dsfw/Result.h>
#include <filesystem>
#include <string>
#include <vector>

namespace dsfw::infer {

    /// @brief Forced alignment result for a single phoneme.
    struct AlignedPhoneme {
        std::string text; ///< Phoneme label.
        double start = 0.0; ///< Start time in seconds.
        double end = 0.0; ///< End time in seconds.
    };

    /// @brief Forced alignment result for a word.
    struct AlignedWord {
        std::string text; ///< Word text.
        std::vector<AlignedPhoneme> phones; ///< Phoneme-level alignment.
    };

    /// @brief Pitch extraction result.
    struct PitchResult {
        float offset = 0.0f; ///< Time offset of the first frame in seconds.
        float timestep = 0.01f; ///< Frame timestep in seconds.
        std::vector<float> f0; ///< F0 frequency values per frame (Hz, 0 = unvoiced).
        std::vector<bool> uv; ///< Voicing flags (true = unvoiced).
    };

    /// @brief MIDI note detection result.
    struct MidiNote {
        float pitch = 0.0f; ///< Pitch as MIDI note number (fractional).
        float onset = 0.0f; ///< Onset time in seconds.
        float duration = 0.0f; ///< Duration in seconds.
        bool voiced = false; ///< Whether the note is voiced.
    };

    /// @brief High-level inference service interface.
    /// Abstracts specific engine implementations (HFA, RMVPE, GAME)
    /// behind a unified API for forced alignment, pitch extraction, and MIDI transcription.
    class IInferenceService {
    public:
        virtual ~IInferenceService() = default;

        /// @brief Check if forced alignment is available.
        virtual bool hasForcedAlignment() const = 0;

        /// @brief Check if pitch extraction is available.
        virtual bool hasPitchExtraction() const = 0;

        /// @brief Check if MIDI transcription is available.
        virtual bool hasMidiTranscription() const = 0;

        /// @brief Run forced alignment on an audio file.
        /// @param audioPath Path to the audio file.
        /// @param language Language code (e.g., "zh", "en").
        /// @param nonSpeechPh Non-speech phoneme labels (e.g., {"AP", "SP"}).
        /// @param lyrics Optional lyrics text for guided alignment.
        /// @param words Output aligned words.
        /// @return True on success.
        virtual bool runForcedAlignment(const std::filesystem::path &audioPath, const std::string &language,
                                        const std::vector<std::string> &nonSpeechPh, const std::string &lyrics,
                                        std::vector<AlignedWord> &words) = 0;

        /// @brief Run pitch extraction on an audio file.
        /// @param audioPath Path to the audio file.
        /// @param timestep Frame timestep in seconds.
        /// @param result Output pitch result.
        /// @return True on success.
        virtual bool runPitchExtraction(const std::filesystem::path &audioPath, float timestep,
                                        PitchResult &result) = 0;

        /// @brief Run MIDI transcription on an audio file.
        /// @param audioPath Path to the audio file.
        /// @param notes Output detected notes.
        /// @return True on success.
        virtual bool runMidiTranscription(const std::filesystem::path &audioPath,
                                          std::vector<MidiNote> &notes) = 0;
    };

} // namespace dsfw::infer
