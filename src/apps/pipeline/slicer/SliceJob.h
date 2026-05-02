/// @file SliceJob.h
/// @brief Audio slice job parameters and execution.

#pragma once

#include <QString>
#include <cstdint>

#include "Enumerations.h"

/// @brief Parameters for a single audio slicing job.
struct SliceJobParams {
    double threshold;          ///< RMS silence threshold in dB.
    qint64 minLength;          ///< Minimum chunk length in samples.
    qint64 minInterval;        ///< Minimum silence interval in samples.
    qint64 hopSize;            ///< RMS calculation hop size in samples.
    qint64 maxSilKept;         ///< Maximum silence kept at chunk boundaries in samples.
    int outputWaveFormat = WF_INT16_PCM; ///< Output WAV sample format.
    bool saveAudio = true;     ///< Whether to save sliced audio files.
    bool saveMarkers = false;  ///< Whether to save marker CSV files.
    bool loadMarkers = false;  ///< Whether to load existing markers instead of slicing.
    bool overwriteMarkers = false; ///< Whether to overwrite existing marker files.
    int minimumDigits = 3;     ///< Minimum digits for zero-padded file suffixes.
};

/// @brief Result of a slice job execution.
struct SliceJobResult {
    bool success;           ///< Whether the job completed successfully.
    QString errorMessage;   ///< Error description on failure.
    int chunksWritten = 0;  ///< Number of audio chunks written.
};

/// @brief Callback interface for slice job progress messages.
struct ISliceJobSink {
    virtual ~ISliceJobSink() = default;
    /// @brief Called with informational messages.
    virtual void onInfo(const QString &msg) = 0;
    /// @brief Called with error messages.
    virtual void onError(const QString &msg) = 0;
};

/// @brief Executes a single file slicing job.
class SliceJob {
public:
    /// @brief Run a slicing job on a single audio file.
    /// @param filename Path to the input audio file.
    /// @param outPath Output directory for sliced chunks.
    /// @param params Slicing parameters.
    /// @param sink Optional callback sink for progress messages.
    /// @return Result with success flag and chunks written count.
    static SliceJobResult run(const QString &filename, const QString &outPath,
                              const SliceJobParams &params, ISliceJobSink *sink = nullptr);
};
