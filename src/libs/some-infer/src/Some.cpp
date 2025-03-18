#include <some-infer/Some.h>

#include <sndfile.hh>

#include <iostream>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>
#include <some-infer/SomeModel.h>

namespace Some
{
    Some::Some(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) {
        m_some = std::make_unique<SomeModel>(modelPath, provider, device_id);

        if (!is_open()) {
            std::cout << "Cannot load SOME Model, there must be files " + modelPath.string() << std::endl;
        }
    }

    Some::~Some() = default;

    bool Some::is_open() const { return m_some && m_some->is_open(); }

    std::vector<double> cumulativeSum(const std::vector<float> &durations) {
        std::vector<double> cumsum(durations.size());
        cumsum[0] = static_cast<double>(durations[0]);
        for (size_t i = 1; i < durations.size(); ++i) {
            cumsum[i] = durations[i] + cumsum[i - 1];
        }
        return cumsum;
    }

    std::vector<int> calculateNoteTicks(const std::vector<float> &note_durations, const float tempo) {
        const std::vector<double> cumsum = cumulativeSum(note_durations);

        std::vector<int> scaled_ticks(cumsum.size());
        for (size_t i = 0; i < cumsum.size(); ++i) {
            scaled_ticks[i] = static_cast<int>(cumsum[i] * tempo * 8);
        }

        std::vector<int> note_ticks(scaled_ticks.size());
        note_ticks[0] = scaled_ticks[0];
        for (size_t i = 1; i < scaled_ticks.size(); ++i) {
            note_ticks[i] = scaled_ticks[i] - scaled_ticks[i - 1];
        }

        return note_ticks;
    }

    static std::vector<Midi> build_midi_note(const int start_tick, const std::vector<float> &note_midi,
                                             const std::vector<float> &note_dur, const std::vector<bool> &note_rest,
                                             const float tempo) {
        std::vector<Midi> midi_data;
        int start_tick_temp = start_tick;
        const std::vector<int> note_ticks = calculateNoteTicks(note_dur, tempo);

        for (size_t i = 0; i < note_midi.size(); ++i) {
            if (note_rest[i]) {
                start_tick_temp += note_ticks[i];
                continue;
            }
            midi_data.push_back(Midi{static_cast<int>(std::round(note_midi[i])), start_tick_temp, note_ticks[i]});
            start_tick_temp += note_ticks[i];
        }

        return midi_data;
    }

    static uint64_t calculateSumOfDifferences(const AudioUtil::MarkerList &markers) {
        uint64_t sum = 0;
        for (const auto &[fst, snd] : markers) {
            sum += (snd - fst);
        }
        return sum;
    }

    bool Some::get_midi(const std::filesystem::path &filepath, std::vector<Midi> &midis, const float tempo,
                        std::string &msg, const std::function<void(int)> &progressChanged) const {
        if (!m_some) {
            return false;
        }

        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, 44100);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                         sf_vio.info.samplerate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(44100, 0.02f, 441, 441 * 4, 500, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        int processedFrames = 0; // To track processed frames
        const auto slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;
            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(static_cast<sf_count_t>(beginFrame), SEEK_SET);
            std::vector<float> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            std::vector<float> temp_midi;
            std::vector<bool> temp_rest;
            std::vector<float> temp_dur;

            const bool success = m_some->forward(tmp, temp_midi, temp_rest, temp_dur, msg);
            if (!success)
                return false;

            const auto start_tick = std::max(static_cast<int>(static_cast<double>(fst) / 44100.0 * tempo * 8),
                                             !midis.empty() ? midis.back().start + midis.back().duration : 0);

            std::vector<Midi> temp_midis = build_midi_note(start_tick, temp_midi, temp_dur, temp_rest, tempo);
            midis.insert(midis.end(), temp_midis.begin(), temp_midis.end());

            // Update the processed frames and calculate progress
            processedFrames += static_cast<int>(frameCount);
            int progress =
                static_cast<int>((static_cast<float>(processedFrames) / static_cast<float>(slicerFrames)) * 100);

            // Call the progress callback with the updated progress
            if (progressChanged) {
                progressChanged(progress); // Trigger the callback with the progress value
            }
        }
        return true;
    }

    void Some::terminate() const { m_some->terminate(); }
} // namespace Some
