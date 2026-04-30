#pragma once

/// @file ITranscriptionService.h
/// @brief Audio-to-MIDI transcription service interface.

#include <QString>

#include <dstools/Result.h>

#include <cstdint>
#include <vector>

namespace dstools {

/// @brief A single MIDI note event.
struct NoteEvent {
    int pitch;          ///< MIDI note number (0-127).
    int64_t startFrame; ///< Start position in audio frames.
    int64_t endFrame;   ///< End position in audio frames.
    float velocity;     ///< Note velocity (0.0-1.0).
};

/// @brief Result of an audio-to-MIDI transcription operation.
struct TranscriptionResult {
    std::vector<NoteEvent> notes; ///< Detected MIDI note events.
    int sampleRate = 0;           ///< Sample rate of the source audio.
};

/// @brief Abstract interface for audio-to-MIDI transcription backends.
class ITranscriptionService {
public:
    virtual ~ITranscriptionService() = default;

    /// @brief Transcribe audio to MIDI note events.
    /// @param audioPath Path to the input audio file.
    /// @return TranscriptionResult on success, or an error description.
    virtual Result<TranscriptionResult> transcribe(const QString &audioPath) = 0;
};

} // namespace dstools
