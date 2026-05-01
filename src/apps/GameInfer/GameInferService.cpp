#include "GameInferService.h"

#include <iostream>

#include <dsfw/JsonHelper.h>
#include <wolf-midi/MidiFile.h>

GameInferService::GameInferService(QObject *parent) : QObject(parent) {
    m_game = std::make_shared<Game::Game>();
}

bool GameInferService::loadModel(const std::filesystem::path &modelPath, Game::ExecutionProvider provider,
                                 int deviceId, std::string &msg) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->load_model(modelPath, provider, deviceId, msg);
}

bool GameInferService::isModelOpen() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->is_open();
}

void GameInferService::setSegThreshold(float v) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    m_game->set_seg_threshold(v);
}

void GameInferService::setSegRadiusFrames(float v) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    m_game->set_seg_radius_frames(v);
}

void GameInferService::setEstThreshold(float v) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    m_game->set_est_threshold(v);
}

void GameInferService::setLanguage(int lang) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    m_game->set_language(lang);
}

void GameInferService::setD3pmTimesteps(int nSteps) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    if (nSteps > 0) {
        m_game->set_d3pm_ts(generateD3pmTimesteps(nSteps));
    }
}

int GameInferService::targetSampleRate() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->get_target_sample_rate();
}

float GameInferService::timestep() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->get_timestep();
}

bool GameInferService::hasDur2bd() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->has_dur2bd();
}

std::map<std::string, int> GameInferService::languageMap() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game->get_language_map();
}

void GameInferService::loadLanguagesFromConfig(const std::filesystem::path &modelPath,
                                               std::map<int, std::string> &idToName,
                                               std::map<std::string, int> &nameToId) {
    idToName.clear();
    nameToId.clear();

    idToName[0] = "default";
    nameToId["default"] = 0;

    const std::filesystem::path configPath = modelPath / "config.json";
    std::string jsonErr;
    auto config = dstools::JsonHelper::loadFile(configPath, jsonErr);

    if (jsonErr.empty()) {
        auto languages = dstools::JsonHelper::getObject(config, "languages");
        if (languages.is_object()) {
            for (auto &[key, value] : languages.items()) {
                if (value.is_number_integer()) {
                    int id = value.get<int>();
                    idToName[id] = key;
                    nameToId[key] = id;
                }
            }
        }
    }
}

float GameInferService::loadTimestepFromConfig(const std::filesystem::path &modelPath) {
    const std::filesystem::path configPath = modelPath / "config.json";
    std::string jsonErr;
    auto config = dstools::JsonHelper::loadFile(configPath, jsonErr);

    if (jsonErr.empty()) {
        float ts = dstools::JsonHelper::get(config, "timestep", 0.01f);
        if (ts > 0)
            return ts;
    }
    return 0.01f;
}

bool GameInferService::exportMidi(const MidiExportParams &params, std::string &msg,
                                  const std::function<void(int)> &progress) {
    std::vector<Game::GameMidi> midis;

    {
        std::lock_guard<std::mutex> lock(m_gameMutex);
        const bool success =
            m_game->get_midi(params.wavPath, midis, params.tempo, msg, progress, params.maxAudioSegLength);
        if (!success)
            return false;
    }

    if (!makeMidiFile(params.outputMidiPath, std::move(midis), params.tempo)) {
        msg = "Failed to save MIDI file.";
        return false;
    }
    return true;
}

bool GameInferService::alignCsv(const AlignCsvParams &params, std::string &msg,
                                const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    Game::AlignOptions options;
    return m_game->alignCSV(params.csvPath, params.savePath, params.saveFilename, true, options, msg, progress);
}

std::vector<float> GameInferService::generateD3pmTimesteps(int nSteps) {
    std::vector<float> ts;
    if (nSteps <= 0)
        return ts;
    constexpr float t0 = 0.0f;
    const float step = (1.0f - t0) / nSteps;
    ts.reserve(nSteps);
    for (int i = 0; i < nSteps; ++i) {
        ts.push_back(t0 + i * step);
    }
    return ts;
}

bool GameInferService::makeMidiFile(const std::filesystem::path &midiPath, std::vector<Game::GameMidi> midis,
                                    float tempo) {
    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();
    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    std::vector<char> trackName;
    std::string str = "Game";
    trackName.insert(trackName.end(), str.begin(), str.end());

    midi.createTrack();
    midi.createMetaEvent(1, 0, Midi::MidiEvent::MetaNumbers::TrackName, trackName);

    for (const auto &[note, start, duration] : midis) {
        midi.createNoteOnEvent(1, start, 0, note, 64);
        midi.createNoteOffEvent(1, start + duration, 0, note, 64);
    }

    return midi.save(midiPath);
}
