#ifndef GAMEINFERSERVICE_H
#define GAMEINFERSERVICE_H

#include <QObject>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <dstools/Result.h>
#include <game-infer/Game.h>

class GameInferService : public QObject {
    Q_OBJECT

public:
    explicit GameInferService(QObject *parent = nullptr);

    dstools::Result<void> loadModel(const std::filesystem::path &modelPath, Game::ExecutionProvider provider, int deviceId);
    bool isModelOpen() const;

    void setSegThreshold(float v);
    void setSegRadiusFrames(float v);
    void setEstThreshold(float v);
    void setLanguage(int lang);
    void setD3pmTimesteps(int nSteps);

    int targetSampleRate() const;
    float timestep() const;
    bool hasDur2bd() const;
    std::map<std::string, int> languageMap() const;

    void loadLanguagesFromConfig(const std::filesystem::path &modelPath,
                                 std::map<int, std::string> &idToName,
                                 std::map<std::string, int> &nameToId);

    float loadTimestepFromConfig(const std::filesystem::path &modelPath);

    struct MidiExportParams {
        std::filesystem::path wavPath;
        std::filesystem::path outputMidiPath;
        float tempo;
        int maxAudioSegLength;
    };

    struct AlignCsvParams {
        std::filesystem::path csvPath;
        std::filesystem::path savePath;
        std::string saveFilename;
    };

    dstools::Result<void> exportMidi(const MidiExportParams &params,
                                     const std::function<void(int)> &progress);

    dstools::Result<void> alignCsv(const AlignCsvParams &params,
                                   const std::function<void(int)> &progress);

private:
    static std::vector<float> generateD3pmTimesteps(int nSteps);
    static bool makeMidiFile(const std::filesystem::path &midiPath, std::vector<Game::GameMidi> midis, float tempo);

    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_gameMutex;
};

#endif // GAMEINFERSERVICE_H
