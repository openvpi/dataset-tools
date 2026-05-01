#pragma once

/// @file AlignmentService.h
/// @brief HuBERT-based forced alignment service implementation.

#include <dsfw/IAlignmentService.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace HFA {
class HFA;
}

/// @brief IAlignmentService implementation using HuBERT-FA for phoneme-level alignment.
class AlignmentService : public dstools::IAlignmentService {
public:
    AlignmentService() = default;
    ~AlignmentService() override;

    /// @brief Load a HuBERT-FA alignment model.
    /// @param modelPath Path to the model file.
    /// @param gpuIndex GPU device index, -1 for CPU.
    /// @return Success or error.
    dstools::Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check if a model is loaded and ready.
    /// @return True if a model is loaded.
    bool isModelLoaded() const override;

    /// @brief Release model resources.
    void unloadModel() override;

    /// @brief Set the target language code for alignment.
    /// @param language Language code (e.g. "zh", "en").
    void setLanguage(const std::string &language);

    /// @brief Configure non-speech phoneme labels.
    /// @param phonemes List of phoneme labels treated as non-speech.
    void setNonSpeechPhonemes(const std::vector<std::string> &phonemes);

    /// @brief Align audio to a phoneme sequence.
    /// @param audioPath Path to the audio file.
    /// @param phonemes Phoneme sequence to align.
    /// @return Alignment result or error.
    dstools::Result<dstools::AlignmentResult> align(const QString &audioPath,
                                            const std::vector<QString> &phonemes) override;

    /// @brief Batch align entries from a CSV file.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path to save alignment results.
    /// @param options Alignment options.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    dstools::Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const dstools::AlignCsvOptions &options = {},
                          const std::function<void(int)> &progress = nullptr) override;

    /// @brief Return vocabulary metadata as JSON.
    /// @return Vocabulary information.
    nlohmann::json vocabInfo() const override;

private:
    std::unique_ptr<HFA::HFA> m_hfa;          ///< HuBERT-FA engine instance
    std::string m_language = "zh";             ///< Target language code
    std::vector<std::string> m_nonSpeechPh;    ///< Non-speech phoneme labels
    mutable std::mutex m_mutex;                ///< Serializes access to the engine
};
