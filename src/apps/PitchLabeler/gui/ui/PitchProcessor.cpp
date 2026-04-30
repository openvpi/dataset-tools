#include "PitchProcessor.h"
#include "gui/DSFile.h"

#include <cmath>
#include <algorithm>
#include <set>

namespace dstools {
namespace pitchlabeler {
namespace ui {

std::vector<double> PitchProcessor::movingAverage(const std::vector<double> &values, int window) {
    std::vector<double> result(values.size(), 0.0);
    int half = window / 2;
    for (size_t i = 0; i < values.size(); ++i) {
        int lo = std::max(0, static_cast<int>(i) - half);
        int hi = std::min(static_cast<int>(values.size()), static_cast<int>(i) + half + 1);
        double sum = 0.0;
        int count = 0;
        for (int j = lo; j < hi; ++j) {
            if (values[j] > 0.0) { sum += values[j]; count++; }
        }
        result[i] = count > 0 ? sum / count : 0.0;
    }
    return result;
}

double PitchProcessor::getRestMidi(const DSFile &ds, int index) {
    const auto &notes = ds.notes;
    for (int offset = 1; offset < static_cast<int>(notes.size()); ++offset) {
        for (int idx : {index - offset, index + offset}) {
            if (idx >= 0 && idx < static_cast<int>(notes.size())) {
                const auto &n = notes[idx];
                if (!n.isRest()) {
                    auto pitch = parseNoteName(n.name);
                    if (pitch.valid) return pitch.midiNumber;
                }
            }
        }
    }
    return 60.0;
}

void PitchProcessor::applyModulationDriftPreview(
    DSFile &ds,
    const std::vector<double> &preAdjustF0,
    const std::set<int> &selectedNotes,
    double modulationAmount,
    double driftAmount)
{
    auto &f0 = ds.f0;
    if (f0.timestep <= 0 || preAdjustF0.empty()) return;

    // Restore from snapshot
    f0.values = preAdjustF0;

    double offset = ds.offset;

    for (int noteIdx : selectedNotes) {
        if (noteIdx < 0 || noteIdx >= static_cast<int>(ds.notes.size())) continue;
        const auto &note = ds.notes[noteIdx];
        if (note.isRest()) continue;
        auto pitch = parseNoteName(note.name);
        if (!pitch.valid) continue;

        int startIdx = std::max(0, static_cast<int>((note.start - offset) / f0.timestep));
        int endIdx = std::min(static_cast<int>(f0.values.size()),
                              static_cast<int>(std::ceil((note.end() - offset) / f0.timestep)));
        if (startIdx >= endIdx) continue;

        // Convert MIDI segment to Hz for processing (matching Python reference)
        int n = endIdx - startIdx;
        std::vector<double> segHz(n);
        std::vector<double> segMidi(n);
        bool hasVoiced = false;
        for (int i = 0; i < n; ++i) {
            double midi = f0.values[startIdx + i];
            segMidi[i] = midi;
            if (midi > 0.0) {
                segHz[i] = midiToFreq(midi);
                hasVoiced = true;
            } else {
                segHz[i] = 0.0;
            }
        }
        if (!hasVoiced) continue;

        double targetFreq = midiToFreq(pitch.midiNumber);
        if (targetFreq <= 0) continue;

        int window = std::max(5, n / 4);
        if (window % 2 == 0) window++;

        auto centerline = movingAverage(segHz, window);

        std::vector<double> newVals = segHz;
        for (int i = 0; i < n; ++i) {
            if (segHz[i] <= 0) continue;
            double modulationI = segHz[i] - centerline[i];
            double driftDev = centerline[i] - targetFreq;
            double newCenter = targetFreq + driftDev * driftAmount;
            double newMod = modulationI * modulationAmount;
            newVals[i] = std::clamp(newCenter + newMod,
                                    midiToFreq(24.0), midiToFreq(108.0));
        }

        // Smoothstep crossfade at edges
        int crossfade = std::clamp(n / 8, 3, n / 2);
        for (int i = 0; i < crossfade; ++i) {
            if (segHz[i] <= 0) continue;
            double t = static_cast<double>(i + 1) / (crossfade + 1);
            t = t * t * (3 - 2 * t);
            newVals[i] = segHz[i] * (1 - t) + newVals[i] * t;
        }
        for (int i = 0; i < crossfade; ++i) {
            int ri = n - 1 - i;
            if (segHz[ri] <= 0) continue;
            double t = static_cast<double>(i + 1) / (crossfade + 1);
            t = t * t * (3 - 2 * t);
            newVals[ri] = segHz[ri] * (1 - t) + newVals[ri] * t;
        }

        // Convert back to MIDI and write
        for (int i = 0; i < n; ++i) {
            if (startIdx + i < static_cast<int>(f0.values.size())) {
                if (newVals[i] > 0.0)
                    f0.values[startIdx + i] = freqToMidi(newVals[i]);
                else
                    f0.values[startIdx + i] = 0.0;
            }
        }
    }
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
