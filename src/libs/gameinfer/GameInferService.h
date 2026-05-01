#pragma once

#include <dsfw/ITranscriptionService.h>
#include <dsfw/IAlignmentService.h>

#include <memory>
#include <mutex>

namespace Game {
class Game;
}

namespace dstools {

class GameTranscriptionService : public ITranscriptionService {
public:
    GameTranscriptionService();
    ~GameTranscriptionService() override;

    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;
    bool isModelLoaded() const override;
    void unloadModel() override;

    Result<TranscriptionResult> transcribe(const QString &audioPath) override;

    Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                            float tempo,
                            const std::function<void(int)> &progress = nullptr) override;

    Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const std::function<void(int)> &progress = nullptr) override;

    TranscriptionModelInfo modelInfo() const override;

private:
    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_mutex;
    TranscriptionModelInfo m_modelInfo;
};

class GameAlignmentService : public IAlignmentService {
public:
    GameAlignmentService();
    ~GameAlignmentService() override;

    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;
    bool isModelLoaded() const override;
    void unloadModel() override;

    Result<AlignmentResult> align(const QString &audioPath,
                                  const std::vector<QString> &phonemes) override;

    Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const AlignCsvOptions &options = {},
                          const std::function<void(int)> &progress = nullptr) override;

    nlohmann::json vocabInfo() const override;

private:
    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_mutex;
};

} // namespace dstools
