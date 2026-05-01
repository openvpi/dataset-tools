#pragma once

/// @file ITranscriptionService.h
/// @brief Audio-to-MIDI transcription service interface and result types.

#include <QString>
#include <dstools/Result.h>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace dstools {

/// @brief A detected musical note event.
struct NoteEvent {
    int pitch;          ///< MIDI note number.
    int64_t startFrame; ///< Start time boundary in frames.
    int64_t endFrame;   ///< End time boundary in frames.
    float velocity;     ///< Note velocity.
};

/// @brief Full transcription output.
struct TranscriptionResult {
    std::vector<NoteEvent> notes; ///< Detected note events.
    int sampleRate = 0;           ///< Source audio sample rate in Hz.
};

/// @brief Simplified MIDI note for export.
struct MidiNote {
    int note;     ///< MIDI note number.
    int start;    ///< Tick position.
    int duration; ///< Tick length.
};

/// @brief Model capability metadata.
struct TranscriptionModelInfo {
    int targetSampleRate = 0;          ///< Expected input sample rate in Hz.
    float timestep = 0.01f;            ///< Time step between frames in seconds.
    bool hasDur2bd = false;            ///< Whether the model includes a dur2bd sub-model.
    std::map<QString, int> languageMap; ///< Supported language name to ID mapping.
};

/// @brief Abstract interface for audio-to-MIDI transcription (e.g. GAME).
class ITranscriptionService {
public:
    virtual ~ITranscriptionService() = default;

    /// @brief Load the transcription model.
    /// @param modelPath Path to the model directory.
    /// @param gpuIndex GPU device index, or -1 for CPU.
    /// @return Ok on success, Err with description on failure.
    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;

    /// @brief Check whether a model is currently loaded.
    /// @return True if a model is loaded and ready.
    virtual bool isModelLoaded() const = 0;

    /// @brief Unload the current model and free resources.
    virtual void unloadModel() = 0;

    /// @brief Transcribe audio to note events.
    /// @param audioPath Path to the input audio file.
    /// @return TranscriptionResult on success, Err on failure.
    virtual Result<TranscriptionResult> transcribe(const QString &audioPath) = 0;

    /// @brief Export transcription as a MIDI file.
    /// @param audioPath Path to the input audio file.
    /// @param outputPath Path for the output MIDI file.
    /// @param tempo Tempo in BPM for MIDI timing.
    /// @param progress Optional progress callback receiving percentage (0-100).
    /// @return Ok on success, Err on failure.
    virtual Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                                    float tempo,
                                    const std::function<void(int)> &progress = nullptr) = 0;

    /// @brief Align CSV annotations to audio.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path for the aligned output file.
    /// @param progress Optional progress callback receiving percentage (0-100).
    /// @return Ok on success, Err on failure.
    virtual Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                  const std::function<void(int)> &progress = nullptr) = 0;

    /// @brief Query model capabilities and metadata.
    /// @return TranscriptionModelInfo describing the loaded model.
    virtual TranscriptionModelInfo modelInfo() const = 0;
};

} // namespace dstools
