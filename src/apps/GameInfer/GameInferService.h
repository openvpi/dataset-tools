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

#include <game-infer/Game.h>

class GameInferService : public QObject {
    Q_OBJECT

public:
    explicit GameInferService(QObject *parent = nullptr);

    // Model lifecycle
    bool loadModel(const std::filesystem::path &modelPath, Game::ExecutionProvider provider, int deviceId,
                   std::string &msg);
    bool isModelOpen() const;

    // Parameter setters (thread-safe, lock m_gameMutex internally)
    void setSegThreshold(float v);
    void setSegRadiusFrames(float v);
    void setEstThreshold(float v);
    void setLanguage(int lang);
    void setD3pmTimesteps(int nSteps);

    // Model info
    int targetSampleRate() const;
    float timestep() const;
    bool hasDur2bd() const;
    std::map<std::string, int> languageMap() const;

    // Config helpers
    void loadLanguagesFromConfig(const std::filesystem::path &modelPath,
                                 std::map<int, std::string> &idToName,
                                 std::map<std::string, int> &nameToId);

    float loadTimestepFromConfig(const std::filesystem::path &modelPath);

    // Operations (blocking, meant for worker threads)
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

    // Sets parameters + runs get_midi + writes MIDI file, all under mutex
    bool exportMidi(const MidiExportParams &params, std::string &msg,
                    const std::function<void(int)> &progress);

    // Sets parameters + runs alignCSV, all under mutex
    bool alignCsv(const AlignCsvParams &params, std::string &msg,
                  const std::function<void(int)> &progress);

private:
    static std::vector<float> generateD3pmTimesteps(int nSteps);
    static bool makeMidiFile(const std::filesystem::path &midiPath, std::vector<Game::GameMidi> midis, float tempo);

    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_gameMutex;
};

#endif // GAMEINFERSERVICE_H
