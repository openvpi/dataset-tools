#include "GameInferService.h"

#include <iostream>

#include <dsfw/JsonHelper.h>
#include <dstools/ExecutionProvider.h>
#include <game-infer/Game.h>
#include <wolf-midi/MidiFile.h>

GameInferService::GameInferService(QObject *parent) : QObject(parent) {
    m_game = std::make_shared<Game::Game>();
}

dstools::Result<void> GameInferService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_gameMutex);

    auto provider = gpuIndex < 0 ? Game::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : Game::ExecutionProvider::DML;
#else
                                 : Game::ExecutionProvider::CPU;
#endif

    auto result = m_game->load_model(modelPath.toStdWString(), provider, gpuIndex);
    if (!result) {
        return result;
    }

    m_modelInfo.targetSampleRate = m_game->get_target_sample_rate();
    m_modelInfo.timestep = m_game->get_timestep();
    m_modelInfo.hasDur2bd = m_game->has_dur2bd();

    const auto &langMap = m_game->get_language_map();
    for (const auto &[name, id] : langMap) {
        m_modelInfo.languageMap[QString::fromStdString(name)] = id;
    }

    return dstools::Ok();
}

bool GameInferService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    return m_game && m_game->is_open();
}

void GameInferService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    if (m_game) {
        m_game->unload();
    }
    m_modelInfo = {};
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

dstools::Result<dstools::TranscriptionResult> GameInferService::transcribe(const QString &audioPath) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    if (!m_game || !m_game->is_open()) {
        return dstools::Err<dstools::TranscriptionResult>("Model not loaded");
    }

    std::vector<Game::GameNote> notes;
    auto result = m_game->get_notes(audioPath.toStdWString(), notes, nullptr);
    if (!result) {
        return dstools::Err<dstools::TranscriptionResult>(result.error());
    }

    dstools::TranscriptionResult out;
    out.sampleRate = m_modelInfo.targetSampleRate;
    for (const auto &n : notes) {
        out.notes.push_back(dstools::NoteEvent{
            static_cast<int>(std::round(n.pitch)),
            static_cast<int64_t>(n.onset * m_modelInfo.targetSampleRate),
            static_cast<int64_t>((n.onset + n.duration) * m_modelInfo.targetSampleRate),
            n.voiced ? 0.8f : 0.0f,
        });
    }
    return dstools::Ok(std::move(out));
}

dstools::Result<void> GameInferService::exportMidi(const QString &audioPath, const QString &outputPath,
                                                     float tempo,
                                                     const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    if (!m_game || !m_game->is_open()) {
        return dstools::Err("Model not loaded");
    }

    std::vector<Game::GameMidi> midis;
    auto result = m_game->get_midi(audioPath.toStdWString(), midis, tempo, progress);
    if (!result) {
        return dstools::Err(result.error());
    }

    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();
    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    midi.createTrack();
    for (const auto &[note, start, duration] : midis) {
        midi.createNoteOnEvent(1, start, 0, note, 64);
        midi.createNoteOffEvent(1, start + duration, 0, note, 64);
    }

    if (!midi.save(outputPath.toStdWString())) {
        return dstools::Err("Failed to save MIDI file.");
    }
    return dstools::Ok();
}

dstools::Result<void> GameInferService::alignCSV(const QString &csvPath, const QString &savePath,
                                                   const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_gameMutex);
    if (!m_game || !m_game->is_open()) {
        return dstools::Err("Model not loaded");
    }

    Game::AlignOptions opts;
    return m_game->alignCSV(csvPath.toStdWString(), savePath.toStdWString(), "", false, opts, progress);
}

dstools::TranscriptionModelInfo GameInferService::modelInfo() const {
    return m_modelInfo;
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
