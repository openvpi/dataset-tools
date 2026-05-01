/// @file GameInferService.h
/// @brief GameInfer application-level transcription service with configurable GAME parameters.

#pragma once

#include <QObject>

#include <dsfw/ITranscriptionService.h>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Game {
class Game;
}

/// @brief ITranscriptionService implementation with GAME parameter tuning.
///
/// Wraps the GAME inference engine and exposes segmentation threshold, radius,
/// estimation threshold, language, and D3PM timestep configuration.
class GameInferService : public QObject, public dstools::ITranscriptionService {
    Q_OBJECT

public:
    /// @param parent Optional parent QObject.
    explicit GameInferService(QObject *parent = nullptr);

    /// @brief Load a GAME model from disk.
    /// @param modelPath Path to the model directory.
    /// @param gpuIndex GPU device index, or -1 for CPU.
    /// @return Success or error result.
    dstools::Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check whether a model is currently loaded.
    bool isModelLoaded() const override;

    /// @brief Unload the current model and free resources.
    void unloadModel() override;

    /// @brief Transcribe an audio file to note events.
    /// @param audioPath Path to the input audio file.
    /// @return Transcription result or error.
    dstools::Result<dstools::TranscriptionResult> transcribe(const QString &audioPath) override;

    /// @brief Export transcription as a MIDI file.
    /// @param audioPath Path to the input audio file.
    /// @param outputPath Path for the output MIDI file.
    /// @param tempo Tempo in BPM for MIDI export.
    /// @param progress Optional progress callback receiving percentage (0-100).
    /// @return Success or error result.
    dstools::Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                                     float tempo,
                                     const std::function<void(int)> &progress = nullptr) override;

    /// @brief Align a CSV file with audio transcription results.
    /// @param csvPath Path to the input CSV file.
    /// @param savePath Path for the aligned output CSV.
    /// @param progress Optional progress callback receiving percentage (0-100).
    /// @return Success or error result.
    dstools::Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                   const std::function<void(int)> &progress = nullptr) override;

    /// @brief Get information about the currently loaded model.
    dstools::TranscriptionModelInfo modelInfo() const override;

    /// @brief Set the segmentation threshold.
    /// @param v Threshold value.
    void setSegThreshold(float v);

    /// @brief Set the segmentation radius in frames.
    /// @param v Radius value in frames.
    void setSegRadiusFrames(float v);

    /// @brief Set the estimation threshold.
    /// @param v Threshold value.
    void setEstThreshold(float v);

    /// @brief Set the language for transcription.
    /// @param lang Language identifier.
    void setLanguage(int lang);

    /// @brief Set the number of D3PM diffusion timesteps.
    /// @param nSteps Number of timesteps.
    void setD3pmTimesteps(int nSteps);

private:
    /// @brief Generate a vector of D3PM timestep values.
    /// @param nSteps Number of timesteps to generate.
    /// @return Vector of timestep values.
    static std::vector<float> generateD3pmTimesteps(int nSteps);

    std::shared_ptr<Game::Game> m_game;  ///< GAME inference engine instance.
    mutable std::mutex m_gameMutex;      ///< Mutex protecting m_game access.
    dstools::TranscriptionModelInfo m_modelInfo; ///< Cached model information.
};
