#include <game-infer/Game.h>

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

    bool Game::load_model(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int device_id,
                          std::string &msg) const {
        m_gameModel->load_model(modelPath, provider, device_id, msg);
        return m_gameModel->is_open();
    }

    bool Game::is_open() const { return m_gameModel && m_gameModel->is_open(); }

    int Game::get_target_sample_rate() const { return m_gameModel ? m_gameModel->get_target_sample_rate() : 0; }

    float Game::get_timestep() const { return m_gameModel ? m_gameModel->get_timestep() : 0.0f; }

    bool Game::has_dur2bd() const { return m_gameModel && m_gameModel->has_dur2bd(); }

    const std::map<std::string, int> &Game::get_language_map() const {
        static const std::map<std::string, int> empty;
        return m_gameModel ? m_gameModel->get_language_map() : empty;
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

        // 使用更高精度的时间计算
        std::vector<double> scaled_ticks_double(cumsum.size());
        for (size_t i = 0; i < cumsum.size(); ++i) {
            // 使用double类型保持精度
            scaled_ticks_double[i] = cumsum[i] * tempo * 480.0 / 60.0;
        }

        // 将差值转换为ticks，避免累积误差
        std::vector<int> note_ticks(scaled_ticks_double.size());
        if (!scaled_ticks_double.empty()) {
            note_ticks[0] = static_cast<int>(std::round(scaled_ticks_double[0]));
            for (size_t i = 1; i < scaled_ticks_double.size(); ++i) {
                // 计算相邻时间点的差值，这样可以保留更多精度
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

    bool Game::get_midi(const std::filesystem::path &filepath, std::vector<GameMidi> &midis, const float tempo,
                        std::string &msg, const std::function<void(int)> &progressChanged, int max_audio_length) const {
        if (!m_gameModel) {
            return false;
        }

        const auto tar_sr = m_gameModel->get_target_sample_rate();
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, tar_sr);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(tar_sr, 0.02f, 441, 441 * 4, 200, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        int processedFrames = 0;
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;

            double sliceDuration = static_cast<double>(frameCount) / tar_sr;

            if (sliceDuration > max_audio_length) {
                msg = "Slice duration exceeds " + std::to_string(max_audio_length) +
                    " seconds: " + std::to_string(sliceDuration) +
                    "s.\nPlease check whether the accompaniment has been removed from the current audio.";
                return false;
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
                return false;

            // 改进开始tick的计算，使用更高精度
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
        return true;
    }

    bool Game::get_notes(const std::filesystem::path &filepath, std::vector<GameNote> &notes, std::string &msg,
                         const std::function<void(int)> &progressChanged, const int max_audio_length) const {
        if (!m_gameModel) {
            msg = "Model not loaded";
            return false;
        }

        const auto tar_sr = m_gameModel->get_target_sample_rate();
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, tar_sr);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(tar_sr, 0.02f, 441, 441 * 4, 200, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        int processedFrames = 0;
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;

            const double sliceDuration = static_cast<double>(frameCount) / tar_sr;

            if (sliceDuration > max_audio_length) {
                msg = "Slice duration exceeds " + std::to_string(max_audio_length) +
                    " seconds: " + std::to_string(sliceDuration) +
                    "s.\nPlease check whether the accompaniment has been removed from the current audio.";
                return false;
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
                return false;

            // Build notes with second-based timing, matching Python callback logic
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

        // Sort and fix overlaps (matching Python SaveCombinedFileCallback.save_file logic)
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
                continue; // skip zero-duration
            notes[writeIdx] = note;
            lastTime = offset;
            writeIdx++;
        }
        notes.resize(writeIdx);

        return true;
    }

    bool Game::isOpen() const { return is_open(); }

    const char *Game::engineName() const { return "GAME"; }

    bool Game::load(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int deviceId,
                    std::string &errorMsg) {
        return load_model(modelPath, provider, deviceId, errorMsg);
    }

    void Game::unload() {
        m_gameModel = std::make_unique<GameModel>();
    }

    int64_t Game::estimatedMemoryBytes() const {
        return is_open() ? 500 * 1024 * 1024LL : 0;
    }

    bool Game::align(const AlignInput &input, const AlignOptions &options, std::vector<AlignedNote> &output,
                     std::string &msg) const {
        if (!m_gameModel) {
            msg = "Model not loaded";
            return false;
        }
        if (!m_gameModel->has_dur2bd()) {
            msg = "dur2bd.onnx not loaded. Align mode requires dur2bd.onnx in model directory.";
            return false;
        }

        // Validate phonemes
        auto [valid, errMsg] = validatePhones(input.phSeq, input.phDur, input.phNum);
        if (!valid) {
            msg = "Invalid phoneme data for '" + input.wavPath.filename().string() + "': " + errMsg;
            return false;
        }

        // Parse words with UV merging for align mode
        std::vector<WordInfo> words;
        if (options.useWordBoundary) {
            words = parseWords(input.phSeq, input.phDur, input.phNum, options.uvVocab, options.uvWordCond,
                               true /* merge consecutive UV for known_durations */);
        } else {
            // No word boundary: single word spanning entire duration
            float totalDur = 0.0f;
            for (const auto d : input.phDur)
                totalDur += d;
            words.push_back(WordInfo{totalDur, true});
        }

        const auto knownDurations = wordDurations(words);

        // Load audio
        const auto tarSr = m_gameModel->get_target_sample_rate();
        std::string audioMsg;
        auto sfVio = AudioUtil::resample_to_vio(input.wavPath, audioMsg, 1, tarSr);
        if (!audioMsg.empty() && sfVio.data.byteArray.empty()) {
            msg = "Failed to load audio '" + input.wavPath.string() + "': " + audioMsg;
            return false;
        }

        SndfileHandle sf(sfVio.vio, &sfVio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sfVio.info.channels,
                         sfVio.info.samplerate);
        const auto totalSize = sf.frames();
        std::vector<float> waveform(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(waveform.data(), static_cast<sf_count_t>(waveform.size()));

        // Run inference with known durations
        std::vector<float> durations, presence, scores;
        if (!m_gameModel->forwardWithKnownDurations(waveform, knownDurations, durations, presence, scores, msg)) {
            return false;
        }

        // Filter valid notes (duration > 0)
        std::vector<float> validDur, validScores, validPresence;
        for (size_t i = 0; i < durations.size(); ++i) {
            if (durations[i] > 0.0f) {
                validDur.push_back(durations[i]);
                validScores.push_back(scores[i]);
                validPresence.push_back(presence[i]);
            }
        }

        if (options.useWordBoundary) {
            // Build note_seq based on uv_note_cond
            std::vector<std::string> noteSeq;
            for (size_t i = 0; i < validScores.size(); ++i) {
                if (options.uvNoteCond == UvNoteCond::Follow) {
                    // Defer v/uv to alignment; use raw pitch for all
                    noteSeq.push_back(game_infer::midiToNoteString(static_cast<double>(validScores[i])));
                } else {
                    // Predict: use presence to decide rest
                    if (validPresence[i] > 0.5f) {
                        noteSeq.push_back(game_infer::midiToNoteString(static_cast<double>(validScores[i])));
                    } else {
                        noteSeq.push_back("rest");
                    }
                }
            }

            // Re-parse words without merging for alignment (matching Python behavior)
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
            // No alignment: apply presence-based rest directly
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

        return true;
    }

    bool Game::alignCSV(const std::filesystem::path &csvPath, const std::filesystem::path &savePath,
                        const std::string &saveFilename, const bool overwrite, const AlignOptions &options,
                        std::string &msg, const std::function<void(int)> &progressChanged) const {
        // Parse CSV
        std::vector<DiffSingerItem> items;
        try {
            items = parseDiffSingerCSV(csvPath);
        }
        catch (const std::exception &e) {
            msg = "Failed to parse CSV: " + std::string(e.what());
            return false;
        }

        // Determine output path
        std::filesystem::path outputPath;
        if (!savePath.empty()) {
            outputPath = savePath;
        } else if (!saveFilename.empty()) {
            outputPath = csvPath.parent_path() / saveFilename;
        } else if (overwrite) {
            outputPath = csvPath;
        } else {
            msg = "No output path specified and overwrite is false";
            return false;
        }

        // Check existing files if not overwriting
        if (!overwrite && std::filesystem::exists(outputPath)) {
            msg = "Output file already exists: " + outputPath.string();
            return false;
        }

        // Run align on each item
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
            if (!align(alignInput, options, result, msg)) {
                msg = "Failed on item '" + item.name + "': " + msg;
                return false;
            }
            allResults.push_back(std::move(result));

            if (progressChanged) {
                progressChanged(static_cast<int>((i + 1) * 100 / items.size()));
            }
        }

        // Write output CSV
        try {
            writeDiffSingerCSV(outputPath, items, allResults);
        }
        catch (const std::exception &e) {
            msg = "Failed to write output CSV: " + std::string(e.what());
            return false;
        }

        return true;
    }

    // Implementation of parameter setting methods
    void Game::set_seg_threshold(const float threshold) const {
        if (m_gameModel) {
            m_gameModel->set_seg_threshold(threshold);
        }
    }

    void Game::set_seg_radius_seconds(const float radius) const {
        if (m_gameModel) {
            m_gameModel->set_seg_radius_seconds(radius);
        }
    }
    void Game::set_seg_radius_frames(const float radiusFrames) const {
        if (m_gameModel) {
            m_gameModel->set_seg_radius_frames(radiusFrames);
        }
    }

    void Game::set_est_threshold(const float threshold) const {
        if (m_gameModel) {
            m_gameModel->set_est_threshold(threshold);
        }
    }

    void Game::set_d3pm_ts(const std::vector<float> &ts) const {
        if (m_gameModel) {
            m_gameModel->set_d3pm_ts(ts);
        }
    }

    void Game::set_language(const int language) const {
        if (m_gameModel) {
            m_gameModel->set_language(language);
        }
    }
} // namespace Game
