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

class GameInferService : public QObject, public dstools::ITranscriptionService {
    Q_OBJECT

public:
    explicit GameInferService(QObject *parent = nullptr);

    dstools::Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;
    bool isModelLoaded() const override;
    void unloadModel() override;

    dstools::Result<dstools::TranscriptionResult> transcribe(const QString &audioPath) override;

    dstools::Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                                     float tempo,
                                     const std::function<void(int)> &progress = nullptr) override;

    dstools::Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                   const std::function<void(int)> &progress = nullptr) override;

    dstools::TranscriptionModelInfo modelInfo() const override;

    void setSegThreshold(float v);
    void setSegRadiusFrames(float v);
    void setEstThreshold(float v);
    void setLanguage(int lang);
    void setD3pmTimesteps(int nSteps);

private:
    static std::vector<float> generateD3pmTimesteps(int nSteps);

    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_gameMutex;
    dstools::TranscriptionModelInfo m_modelInfo;
};
