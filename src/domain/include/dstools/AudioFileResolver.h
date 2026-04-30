#pragma once

#include <QString>
#include <QStringList>

namespace dstools {

/// Unified audio file resolution across all applications.
/// Supports two naming conventions:
///   1. Simple same-name: song.ds → song.wav / song.mp3 / ...
///   2. Suffix-encoded: song_mp3.ds → song.mp3 (legacy SlurCutter format)
class AudioFileResolver {
public:
    /// Find the audio file corresponding to a data file (.json/.lab/.ds/.TextGrid).
    /// Tries simple same-name first, then suffix-encoded matching.
    /// Returns the first existing audio file path, or empty string if not found.
    static QString findAudioFile(const QString &dataFilePath);

    /// Generate a data file path from an audio file path.
    /// Non-wav audio uses suffix encoding: song.mp3 → song_mp3.json
    /// @param audioPath  Full path to the audio file
    /// @param suffix     Target extension without dot, e.g. "json", "lab", "ds"
    static QString audioToDataFile(const QString &audioPath, const QString &suffix);

    /// Supported audio extensions (without dot): {"wav", "mp3", "m4a", "flac", "ogg"}
    static QStringList audioExtensions();
};

} // namespace dstools
