#pragma once

/// @file GameInferService.h
/// @brief GAME-based audio transcription and alignment service implementations.

#include <dsfw/ITranscriptionService.h>
#include <dsfw/IAlignmentService.h>

#include <memory>
#include <mutex>

namespace Game {
class Game;
}

namespace dstools {

/// @brief ITranscriptionService implementation using the GAME audio-to-MIDI engine.
class GameTranscriptionService : public ITranscriptionService {
public:
    GameTranscriptionService();
    ~GameTranscriptionService() override;

    /// @brief Load a GAME transcription model.
    /// @param modelPath Path to the model directory.
    /// @param gpuIndex GPU device index, -1 for CPU.
    /// @return Success or error.
    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check if a model is loaded and ready.
    /// @return True if a model is loaded.
    bool isModelLoaded() const override;

    /// @brief Release model resources.
    void unloadModel() override;

    /// @brief Transcribe audio to note events.
    /// @param audioPath Path to the audio file.
    /// @return Transcription result or error.
    Result<TranscriptionResult> transcribe(const QString &audioPath) override;

    /// @brief Export transcription as a MIDI file.
    /// @param audioPath Path to the source audio file.
    /// @param outputPath Path to the output MIDI file.
    /// @param tempo Tempo in BPM for the MIDI export.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                            float tempo,
                            const std::function<void(int)> &progress = nullptr) override;

    /// @brief Batch transcribe and align entries from a CSV file.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path to save results.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const std::function<void(int)> &progress = nullptr) override;

    /// @brief Return metadata about the loaded model.
    /// @return Model information struct.
    TranscriptionModelInfo modelInfo() const override;

private:
    std::shared_ptr<Game::Game> m_game; ///< GAME engine instance
    mutable std::mutex m_mutex;         ///< Serializes access to the engine
    TranscriptionModelInfo m_modelInfo; ///< Cached model metadata
};

/// @brief IAlignmentService implementation using the GAME engine for note-level alignment.
class GameAlignmentService : public IAlignmentService {
public:
    GameAlignmentService();
    ~GameAlignmentService() override;

    /// @brief Load a GAME alignment model.
    /// @param modelPath Path to the model directory.
    /// @param gpuIndex GPU device index, -1 for CPU.
    /// @return Success or error.
    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check if a model is loaded and ready.
    /// @return True if a model is loaded.
    bool isModelLoaded() const override;

    /// @brief Release model resources.
    void unloadModel() override;

    /// @brief Align audio to a phoneme sequence.
    /// @param audioPath Path to the audio file.
    /// @param phonemes Phoneme sequence to align.
    /// @return Alignment result or error.
    Result<AlignmentResult> align(const QString &audioPath,
                                  const std::vector<QString> &phonemes) override;

    /// @brief Batch align entries from a CSV file.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path to save alignment results.
    /// @param options Alignment options.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const AlignCsvOptions &options = {},
                          const std::function<void(int)> &progress = nullptr) override;

    /// @brief Return vocabulary metadata as JSON.
    /// @return Vocabulary information.
    nlohmann::json vocabInfo() const override;

private:
    std::shared_ptr<Game::Game> m_game; ///< GAME engine instance
    mutable std::mutex m_mutex;         ///< Serializes access to the engine
};

} // namespace dstools
