/// @file AudioFileLoader.h
/// @brief Audio file loader with format conversion support.

#pragma once

#include <QString>
#include <vector>

/// @brief Raw audio samples with metadata.
struct AudioData {
    std::vector<double> samples; ///< Interleaved audio sample data.
    int sampleRate = 0;          ///< Sample rate in Hz.
    int channels = 0;            ///< Number of audio channels.
    qint64 frames = 0;           ///< Number of audio frames.

    /// @brief Get total sample count (frames * channels).
    qint64 totalSize() const { return frames * channels; }
};

/// @brief Result of an audio file load operation.
struct AudioLoadResult {
    bool success = false;     ///< Whether the load succeeded.
    QString errorMessage;     ///< Error description on failure.
    AudioData audio;          ///< Loaded audio data.
    QString sndfilePath;      ///< Path usable with sndfile (original or temp WAV).
    QString tempFilePath;     ///< Non-empty if a temp WAV was created (caller must clean up).
};

/// @brief Loads audio files, converting non-WAV formats via temporary files.
class AudioFileLoader {
public:
    /// @brief Load an audio file from disk.
    /// @param filePath Path to the audio file.
    /// @return Load result with audio data or error information.
    static AudioLoadResult load(const QString &filePath);
};
