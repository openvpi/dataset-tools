/// @file AudioChunkWriter.h
/// @brief Audio chunk file writer for sliced audio output.

#pragma once

#include <QString>
#include <utility>
#include <vector>

struct AudioData;

using MarkerList = std::vector<std::pair<qint64, qint64>>;

/// @brief Result of a chunk writing operation.
struct ChunkWriteResult {
    bool success = false;       ///< Whether the write operation succeeded.
    int chunksWritten = 0;      ///< Number of audio chunks written.
    QString errorMessage;       ///< Error message if the operation failed.
};

/// @brief Static utility for writing sliced audio chunks to individual files.
class AudioChunkWriter {
public:
    /// @brief Write audio chunks to separate files in the output directory.
    /// @param audio Source audio data.
    /// @param chunks List of (start, end) sample pairs defining each chunk.
    /// @param outputDir Directory to write chunk files into.
    /// @param fileBaseName Base name for output files.
    /// @param outputWaveFormat Output WAV format (e.g., INT16_PCM).
    /// @param minimumDigits Minimum digits for zero-padded file suffixes.
    /// @return Result with success flag and number of chunks written.
    static ChunkWriteResult writeChunks(const AudioData &audio, const MarkerList &chunks,
                                        const QString &outputDir, const QString &fileBaseName,
                                        int outputWaveFormat, int minimumDigits);
};
