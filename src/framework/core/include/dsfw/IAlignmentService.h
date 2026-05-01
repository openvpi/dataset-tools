#pragma once

/// @file IAlignmentService.h
/// @brief Phoneme alignment service interface and result types.

#include <QString>
#include <dstools/Result.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <vector>

namespace dstools {

/// @brief A single phoneme with time boundaries.
struct AlignedPhone {
    QString phone;   ///< Phoneme label
    double startSec; ///< Start time in seconds
    double endSec;   ///< End time in seconds
};

/// @brief Full alignment output.
struct AlignmentResult {
    std::vector<AlignedPhone> phones; ///< Aligned phoneme list
    int sampleRate = 0;               ///< Source audio sample rate
};

/// @brief Options for CSV-based batch alignment.
struct AlignCsvOptions {
    std::vector<QString> uvVocab = {"AP", "SP", "br", "sil"}; ///< Unvoiced phoneme vocabulary
    bool useWordBoundary = true;                               ///< Whether to use word boundaries
};

/// @brief Abstract interface for forced alignment backends (e.g. HuBERT-FA).
class IAlignmentService {
public:
    virtual ~IAlignmentService() = default;

    /// @brief Load an alignment model.
    /// @param modelPath Path to the model file.
    /// @param gpuIndex GPU device index, -1 for CPU.
    /// @return Success or error.
    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;

    /// @brief Check if a model is loaded and ready.
    /// @return True if a model is loaded.
    virtual bool isModelLoaded() const = 0;

    /// @brief Release model resources.
    virtual void unloadModel() = 0;

    /// @brief Align audio to a phoneme sequence.
    /// @param audioPath Path to the audio file.
    /// @param phonemes Phoneme sequence to align.
    /// @return Alignment result or error.
    virtual Result<AlignmentResult> align(const QString &audioPath,
                                          const std::vector<QString> &phonemes) = 0;

    /// @brief Batch align entries from a CSV file.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path to save alignment results.
    /// @param options Alignment options.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    virtual Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                  const AlignCsvOptions &options = {},
                                  const std::function<void(int)> &progress = nullptr) = 0;

    /// @brief Return vocabulary metadata as JSON.
    /// @return Vocabulary information.
    virtual nlohmann::json vocabInfo() const = 0;
};

} // namespace dstools
