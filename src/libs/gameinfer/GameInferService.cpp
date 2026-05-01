#include "GameInferService.h"

#include <game-infer/Game.h>
#include <wolf-midi/MidiFile.h>

#include <dstools/ExecutionProvider.h>

namespace dstools {

GameTranscriptionService::GameTranscriptionService() = default;
GameTranscriptionService::~GameTranscriptionService() = default;

Result<void> GameTranscriptionService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_game) {
        m_game = std::make_shared<Game::Game>();
    }

    auto provider = gpuIndex < 0 ? Game::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : Game::ExecutionProvider::DML;
#else
                                 : Game::ExecutionProvider::CPU;
#endif

    auto result = m_game->load_model(modelPath.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_game.reset();
        return Err(result.error());
    }

    m_modelInfo.targetSampleRate = m_game->get_target_sample_rate();
    m_modelInfo.timestep = m_game->get_timestep();
    m_modelInfo.hasDur2bd = m_game->has_dur2bd();

    const auto &langMap = m_game->get_language_map();
    for (const auto &[name, id] : langMap) {
        m_modelInfo.languageMap[QString::fromStdString(name)] = id;
    }

    return Ok();
}

bool GameTranscriptionService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_game && m_game->is_open();
}

void GameTranscriptionService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_game) {
        m_game->unload();
    }
    m_modelInfo = {};
}

Result<TranscriptionResult> GameTranscriptionService::transcribe(const QString &audioPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_game || !m_game->is_open()) {
        return Err<TranscriptionResult>("Model not loaded");
    }

    std::vector<Game::GameNote> notes;
    auto result = m_game->get_notes(audioPath.toStdWString(), notes, nullptr);
    if (!result) {
        return Err<TranscriptionResult>(result.error());
    }

    TranscriptionResult out;
    out.sampleRate = m_modelInfo.targetSampleRate;
    for (const auto &n : notes) {
        out.notes.push_back(NoteEvent{
            static_cast<int>(std::round(n.pitch)),
            static_cast<int64_t>(n.onset * m_modelInfo.targetSampleRate),
            static_cast<int64_t>((n.onset + n.duration) * m_modelInfo.targetSampleRate),
            n.voiced ? 0.8f : 0.0f,
        });
    }
    return Ok(std::move(out));
}

Result<void> GameTranscriptionService::exportMidi(const QString &audioPath, const QString &outputPath,
                                                   float tempo,
                                                   const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_game || !m_game->is_open()) {
        return Err("Model not loaded");
    }

    std::vector<Game::GameMidi> midis;
    auto result = m_game->get_midi(audioPath.toStdWString(), midis, tempo, progress);
    if (!result) {
        return Err(result.error());
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
        return Err("Failed to save MIDI file: " + outputPath.toStdString());
    }
    return Ok();
}

Result<void> GameTranscriptionService::alignCSV(const QString &csvPath, const QString &savePath,
                                                  const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_game || !m_game->is_open()) {
        return Err("Model not loaded");
    }

    Game::AlignOptions opts;
    return m_game->alignCSV(csvPath.toStdWString(), savePath.toStdWString(), "", false, opts, progress);
}

TranscriptionModelInfo GameTranscriptionService::modelInfo() const {
    return m_modelInfo;
}

GameAlignmentService::GameAlignmentService() = default;
GameAlignmentService::~GameAlignmentService() = default;

Result<void> GameAlignmentService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_game) {
        m_game = std::make_shared<Game::Game>();
    }

    auto provider = gpuIndex < 0 ? Game::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : Game::ExecutionProvider::DML;
#else
                                 : Game::ExecutionProvider::CPU;
#endif

    auto result = m_game->load_model(modelPath.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_game.reset();
        return Err(result.error());
    }
    return Ok();
}

bool GameAlignmentService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_game && m_game->is_open();
}

void GameAlignmentService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_game) {
        m_game->unload();
    }
}

Result<AlignmentResult> GameAlignmentService::align(const QString &audioPath,
                                                      const std::vector<QString> &phonemes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_game || !m_game->is_open()) {
        return Err<AlignmentResult>("Model not loaded");
    }

    Game::AlignInput input;
    input.wavPath = audioPath.toStdWString();
    for (const auto &ph : phonemes) {
        input.phSeq.push_back(ph.toStdString());
    }

    Game::AlignOptions opts;
    std::vector<Game::AlignedNote> output;
    auto result = m_game->align(input, opts, output);
    if (!result) {
        return Err<AlignmentResult>(result.error());
    }

    AlignmentResult out;
    for (const auto &note : output) {
        AlignedPhone phone;
        phone.phone = QString::fromStdString(note.name);
        phone.startSec = 0;
        phone.endSec = note.duration;
        out.phones.push_back(std::move(phone));
    }
    return Ok(std::move(out));
}

Result<void> GameAlignmentService::alignCSV(const QString &csvPath, const QString &savePath,
                                              const AlignCsvOptions &options,
                                              const std::function<void(int)> &progress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_game || !m_game->is_open()) {
        return Err("Model not loaded");
    }

    Game::AlignOptions opts;
    for (const auto &uv : options.uvVocab) {
        opts.uvVocab.insert(uv.toStdString());
    }
    opts.useWordBoundary = options.useWordBoundary;

    return m_game->alignCSV(csvPath.toStdWString(), savePath.toStdWString(), "", false, opts, progress);
}

nlohmann::json GameAlignmentService::vocabInfo() const {
    return nlohmann::json::object();
}

} // namespace dstools
