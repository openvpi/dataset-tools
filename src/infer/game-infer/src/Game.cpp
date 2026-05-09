#include <game-infer/Game.h>

#include <dstools/PathEncoding.h>
#include <sndfile.hh>

#include <algorithm>
#include <cmath>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>
#include <game-infer/DiffSingerParser.h>
#include <game-infer/GameModel.h>
#include "NoteUtils.h"

namespace Game
{
    Game::Game() { m_gameModel = std::make_unique<GameModel>(); }

    Game::~Game() = default;

    dstools::Result<void> Game::loadModel(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int device_id) const {
        std::string msg;
        m_gameModel->loadModel(modelPath, provider, device_id, msg);
        if (!m_gameModel->isOpen())
            return dstools::Err(msg);
        return dstools::Ok();
    }

    bool Game::isOpen() const { return m_gameModel && m_gameModel->isOpen(); }

    int Game::targetSampleRate() const { return m_gameModel ? m_gameModel->targetSampleRate() : 0; }

    float Game::timestep() const { return m_gameModel ? m_gameModel->timestep() : 0.0f; }

    bool Game::hasDur2bd() const { return m_gameModel && m_gameModel->hasDur2bd(); }

    const std::map<std::string, int> &Game::languageMap() const {
        static const std::map<std::string, int> empty;
        return m_gameModel ? m_gameModel->languageMap() : empty;
    }

    std::vector<double> cumulativeSum(const std::vector<float> &durations) {
        if (durations.empty())
            return {};
        std::vector<double> cumsum(durations.size());
        cumsum[0] = static_cast<double>(durations[0]);
        for (size_t i = 1; i < durations.size(); ++i) {
            cumsum[i] = durations[i] + cumsum[i - 1];
        }
        return cumsum;
    }

    std::vector<int> calculateNoteTicks(const std::vector<float> &note_durations, const float tempo) {
        const std::vector<double> cumsum = cumulativeSum(note_durations);

        std::vector<double> scaled_ticks_double(cumsum.size());
        for (size_t i = 0; i < cumsum.size(); ++i) {
            scaled_ticks_double[i] = cumsum[i] * tempo * 480.0 / 60.0;
        }

        std::vector<int> note_ticks(scaled_ticks_double.size());
        if (!scaled_ticks_double.empty()) {
            note_ticks[0] = static_cast<int>(std::round(scaled_ticks_double[0]));
            for (size_t i = 1; i < scaled_ticks_double.size(); ++i) {
                const double tick_diff = scaled_ticks_double[i] - scaled_ticks_double[i - 1];
                note_ticks[i] = static_cast<int>(std::round(tick_diff));
            }
        }

        return note_ticks;
    }

    static std::vector<GameMidi> build_midi_note(const int start_tick, const std::vector<float> &durations,
                                                 const std::vector<float> &presence, const std::vector<float> &scores,
                                                 const float tempo) {
        std::vector<GameMidi> midi_data;
        int current_tick = start_tick;

        const std::vector<int> note_ticks = calculateNoteTicks(durations, tempo);

        for (size_t i = 0; i < durations.size(); ++i) {
            if (presence[i] > 0.5) {
                const int pitch = static_cast<int>(std::round(scores[i]));

                const int duration_ticks = note_ticks[i];

                midi_data.push_back(GameMidi{pitch, current_tick, duration_ticks});

                current_tick += duration_ticks;
            } else {
                if (i < note_ticks.size()) {
                    current_tick += note_ticks[i];
                }
            }
        }

        return midi_data;
    }

    static uint64_t calculateSumOfDifferences(const AudioUtil::MarkerList &markers) {
        uint64_t sum = 0;
        for (const auto &[fst, snd] : markers) {
            sum += snd - fst;
        }
        return sum;
    }

    dstools::Result<void> Game::getMidi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, const float tempo,
                                          const std::function<void(int)> &progressChanged, int max_audio_length) const {
        if (!m_gameModel) {
            return dstools::Err("Model not loaded");
        }

        const auto tar_sr = m_gameModel->targetSampleRate();
        std::string msg;
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, tar_sr);

        if (!msg.empty()) {
            return dstools::Err("Failed to resample audio for Game inference: " + msg);
        }

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        if (!sf) {
            return dstools::Err("Failed to open resampled audio for Game inference");
        }
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(tar_sr, 0.02f, 441, 441 * 4, 200, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            return dstools::Err("slicer: no audio chunks for output!");
        }

        int processedFrames = 0;
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;

            double sliceDuration = static_cast<double>(frameCount) / tar_sr;

            if (sliceDuration > max_audio_length) {
                return dstools::Err("Slice duration exceeds " + std::to_string(max_audio_length) +
                    " seconds: " + std::to_string(sliceDuration) +
                    "s.\nPlease check whether the accompaniment has been removed from the current audio.");
            }

            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(beginFrame, SEEK_SET);
            std::vector<float> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            std::vector<bool> boundaries;
            std::vector<float> durations;
            std::vector<float> presence;
            std::vector<float> scores;

            const bool success = m_gameModel->forward(tmp, boundaries, durations, presence, scores, msg);
            if (!success)
                return dstools::Err(msg);

            int previous_end_tick = !midis.empty() ? midis.back().start + midis.back().duration : 0;
            int chunk_start_time_ticks =
                static_cast<int>(std::round(static_cast<double>(fst) / tar_sr * tempo * 480.0 / 60.0));
            const auto start_tick = std::max(chunk_start_time_ticks, previous_end_tick);

            std::vector<GameMidi> temp_midis = build_midi_note(start_tick, durations, presence, scores, tempo);
            midis.insert(midis.end(), temp_midis.begin(), temp_midis.end());

            processedFrames += static_cast<int>(frameCount);
            int progress =
                static_cast<int>(static_cast<float>(processedFrames) / static_cast<float>(slicerFrames) * 100);

            if (progressChanged) {
                progressChanged(progress);
            }
        }
        return dstools::Ok();
    }

    dstools::Result<void> Game::getNotes(const std::filesystem::path &filepath, std::vector<GameNote> &notes,
                                           const std::function<void(int)> &progressChanged, const int max_audio_length) const {
        if (!m_gameModel) {
            return dstools::Err("Model not loaded");
        }

        const auto tar_sr = m_gameModel->targetSampleRate();
        std::string msg;
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, tar_sr);

        if (!msg.empty()) {
            return dstools::Err("Failed to resample audio for Game inference: " + msg);
        }

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        if (!sf) {
            return dstools::Err("Failed to open resampled audio for Game inference");
        }
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(tar_sr, 0.02f, 441, 441 * 4, 200, 30, 50);
        const auto chunks = slicer.slice(audio);
        if (chunks.empty()) {
            return dstools::Err("slicer: no audio chunks for output!");
        }

        int processedFrames = 0;
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;

            const double sliceDuration = static_cast<double>(frameCount) / tar_sr;

            if (sliceDuration > max_audio_length) {
                return dstools::Err("Slice duration exceeds " + std::to_string(max_audio_length) +
                    " seconds: " + std::to_string(sliceDuration) +
                    "s.\nPlease check whether the accompaniment has been removed from the current audio.");
            }

            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(beginFrame, SEEK_SET);
            std::vector<float> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            std::vector<bool> boundaries;
            std::vector<float> durations;
            std::vector<float> presence;
            std::vector<float> scores;

            const bool success = m_gameModel->forward(tmp, boundaries, durations, presence, scores, msg);
            if (!success)
                return dstools::Err(msg);

            const double chunkOffset = static_cast<double>(fst) / tar_sr;
            double noteOnset = 0.0;
            for (size_t i = 0; i < durations.size(); ++i) {
                const float dur = durations[i];
                if (dur <= 0.0f)
                    continue;
                const bool voiced = presence[i] > 0.5f;
                if (voiced) {
                    notes.push_back(GameNote{
                        scores[i],
                        static_cast<float>(chunkOffset + noteOnset),
                        dur,
                        true,
                    });
                }
                noteOnset += dur;
            }

            processedFrames += static_cast<int>(frameCount);
            const int progress =
                static_cast<int>(static_cast<float>(processedFrames) / static_cast<float>(slicerFrames) * 100);

            if (progressChanged) {
                progressChanged(progress);
            }
        }

        std::sort(notes.begin(), notes.end(),
                  [](const GameNote &a, const GameNote &b)
                  {
                      if (a.onset != b.onset)
                          return a.onset < b.onset;
                      return (a.onset + a.duration) < (b.onset + b.duration);
                  });
        float lastTime = 0.0f;
        size_t writeIdx = 0;
        for (size_t i = 0; i < notes.size(); ++i) {
            auto &note = notes[i];
            note.onset = std::max(note.onset, lastTime);
            const float offset = note.onset + note.duration;
            if (offset <= note.onset)
                continue;
            notes[writeIdx] = note;
            lastTime = offset;
            writeIdx++;
        }
        notes.resize(writeIdx);

        return dstools::Ok();
    }

    const char *Game::engineName() const { return "GAME"; }

    dstools::Result<void> Game::load(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int deviceId) {
        return loadModel(modelPath, provider, deviceId);
    }

    void Game::unload() {
        m_gameModel = std::make_unique<GameModel>();
    }

    int64_t Game::estimatedMemoryBytes() const {
        return isOpen() ? 500 * 1024 * 1024LL : 0;
    }

    dstools::Result<void> Game::align(const AlignInput &input, const AlignOptions &options, std::vector<AlignedNote> &output) const {
        if (!m_gameModel) {
            return dstools::Err("Model not loaded");
        }
        if (!m_gameModel->hasDur2bd()) {
            return dstools::Err("dur2bd.onnx not loaded. Align mode requires dur2bd.onnx in model directory.");
        }

        auto [valid, errMsg] = validatePhones(input.phSeq, input.phDur, input.phNum);
        if (!valid) {
            return dstools::Err("Invalid phoneme data for '" + input.wavPath.filename().string() + "': " + errMsg);
        }

        std::vector<WordInfo> words;
        if (options.useWordBoundary) {
            words = parseWords(input.phSeq, input.phDur, input.phNum, options.uvVocab, options.uvWordCond,
                               true);
        } else {
            float totalDur = 0.0f;
            for (const auto d : input.phDur)
                totalDur += d;
            words.push_back(WordInfo{totalDur, true});
        }

        const auto knownDurations = wordDurations(words);

        const auto tarSr = m_gameModel->targetSampleRate();
        std::string audioMsg;
        auto sfVio = AudioUtil::resample_to_vio(input.wavPath, audioMsg, 1, tarSr);
        if (!audioMsg.empty() && sfVio.data.byteArray.empty()) {
            return dstools::Err("Failed to load audio '" + dstools::pathToUtf8(input.wavPath) + "': " + audioMsg);
        }

        SndfileHandle sf(sfVio.vio, &sfVio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sfVio.info.channels,
                         sfVio.info.samplerate);
        if (!sf) {
            return dstools::Err("Failed to open resampled audio for Game inference: " + audioMsg);
        }
        const auto totalSize = sf.frames();
        std::vector<float> waveform(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(waveform.data(), static_cast<sf_count_t>(waveform.size()));

        std::vector<float> durations, presence, scores;
        std::string msg;
        if (!m_gameModel->forwardWithKnownDurations(waveform, knownDurations, durations, presence, scores, msg)) {
            return dstools::Err(msg);
        }

        std::vector<float> validDur, validScores, validPresence;
        for (size_t i = 0; i < durations.size(); ++i) {
            if (durations[i] > 0.0f) {
                validDur.push_back(durations[i]);
                validScores.push_back(scores[i]);
                validPresence.push_back(presence[i]);
            }
        }

        if (options.useWordBoundary) {
            std::vector<std::string> noteSeq;
            for (size_t i = 0; i < validScores.size(); ++i) {
                if (options.uvNoteCond == UvNoteCond::Follow) {
                    noteSeq.push_back(game_infer::midiToNoteString(static_cast<double>(validScores[i])));
                } else {
                    if (validPresence[i] > 0.5f) {
                        noteSeq.push_back(game_infer::midiToNoteString(static_cast<double>(validScores[i])));
                    } else {
                        noteSeq.push_back("rest");
                    }
                }
            }

            std::vector<WordInfo> alignWords;
            if (options.uvNoteCond == UvNoteCond::Follow) {
                alignWords =
                    parseWords(input.phSeq, input.phDur, input.phNum, options.uvVocab, options.uvWordCond, false);
            } else {
                alignWords =
                    parseWords(input.phSeq, input.phDur, input.phNum, options.uvVocab, options.uvWordCond, false);
            }

            output = alignNotesToWords(alignWords, noteSeq, validDur, 0.01f, options.uvNoteCond == UvNoteCond::Follow);
        } else {
            for (size_t i = 0; i < validScores.size(); ++i) {
                std::string name;
                if (validPresence[i] > 0.5f) {
                    name = game_infer::midiToNoteString(static_cast<double>(validScores[i]));
                } else {
                    name = "rest";
                }
                output.push_back(AlignedNote{name, validDur[i], 0});
            }
        }

        return dstools::Ok();
    }

    dstools::Result<void> Game::alignCSV(const std::filesystem::path &csvPath, const std::filesystem::path &savePath,
                                          const std::string &saveFilename, const bool overwrite, const AlignOptions &options,
                                          const std::function<void(int)> &progressChanged) const {
        std::vector<DiffSingerItem> items;
        try {
            items = parseDiffSingerCSV(csvPath);
        }
        catch (const std::exception &e) {
            return dstools::Err("Failed to parse CSV: " + std::string(e.what()));
        }

        std::filesystem::path outputPath;
        if (!savePath.empty()) {
            outputPath = savePath;
        } else if (!saveFilename.empty()) {
            outputPath = csvPath.parent_path() / saveFilename;
        } else if (overwrite) {
            outputPath = csvPath;
        } else {
            return dstools::Err("No output path specified and overwrite is false");
        }

        if (!overwrite && std::filesystem::exists(outputPath)) {
            return dstools::Err("Output file already exists: " + dstools::pathToUtf8(outputPath));
        }

        std::vector<std::vector<AlignedNote>> allResults;
        allResults.reserve(items.size());

        for (size_t i = 0; i < items.size(); ++i) {
            const auto &item = items[i];

            AlignInput alignInput;
            alignInput.wavPath = item.wavPath;
            alignInput.phSeq = item.phSeq;
            alignInput.phDur = item.phDur;
            alignInput.phNum = item.phNum;

            std::vector<AlignedNote> result;
            auto alignResult = align(alignInput, options, result);
            if (!alignResult) {
                return dstools::Err("Failed on item '" + item.name + "': " + alignResult.error());
            }
            allResults.push_back(std::move(result));

            if (progressChanged) {
                progressChanged(static_cast<int>((i + 1) * 100 / items.size()));
            }
        }

        try {
            writeDiffSingerCSV(outputPath, items, allResults);
        }
        catch (const std::exception &e) {
            return dstools::Err("Failed to write output CSV: " + std::string(e.what()));
        }

        return dstools::Ok();
    }

    void Game::setSegThreshold(const float threshold) const {
        if (m_gameModel) {
            m_gameModel->setSegThreshold(threshold);
        }
    }

    void Game::setSegRadiusSeconds(const float radius) const {
        if (m_gameModel) {
            m_gameModel->setSegRadiusSeconds(radius);
        }
    }
    void Game::setSegRadiusFrames(const float radiusFrames) const {
        if (m_gameModel) {
            m_gameModel->setSegRadiusFrames(radiusFrames);
        }
    }

    void Game::setEstThreshold(const float threshold) const {
        if (m_gameModel) {
            m_gameModel->setEstThreshold(threshold);
        }
    }

    void Game::setD3pmTs(const std::vector<float> &ts) const {
        if (m_gameModel) {
            m_gameModel->setD3pmTs(ts);
        }
    }

    void Game::setLanguage(const int language) const {
        if (m_gameModel) {
            m_gameModel->setLanguage(language);
        }
    }
} // namespace Game
