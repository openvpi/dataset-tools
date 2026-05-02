#include "PitchProcessor.h"
#include "gui/DSFile.h"

#include <dstools/CurveTools.h>

#include <cmath>
#include <algorithm>
#include <set>

namespace dstools {
namespace pitchlabeler {
namespace ui {

std::vector<double> PitchProcessor::movingAverage(const std::vector<double> &values, int window) {
    return dstools::movingAverage(values, window, true);
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
    const std::vector<int32_t> &preAdjustF0,
    const std::set<int> &selectedNotes,
    double modulationAmount,
    double driftAmount)
{
    auto &f0 = ds.f0;
    if (f0.timestep <= 0 || preAdjustF0.empty()) return;

    f0.values = preAdjustF0;

    TimePos offset = ds.offset;

    for (int noteIdx : selectedNotes) {
        if (noteIdx < 0 || noteIdx >= static_cast<int>(ds.notes.size())) continue;
        const auto &note = ds.notes[noteIdx];
        if (note.isRest()) continue;
        auto pitch = parseNoteName(note.name);
        if (!pitch.valid) continue;

        int startIdx = std::max(0, static_cast<int>((note.start - offset) / f0.timestep));
        int endIdx = std::min(static_cast<int>(f0.values.size()),
                              static_cast<int>(std::ceil(
                                  static_cast<double>(note.end() - offset) / f0.timestep)));
        if (startIdx >= endIdx) continue;

        int n = endIdx - startIdx;
        std::vector<double> segHz(n);
        bool hasVoiced = false;
        for (int i = 0; i < n; ++i) {
            double hz = mhzToHz(f0.values[startIdx + i]);
            segHz[i] = hz;
            if (hz > 0.0) hasVoiced = true;
        }
        if (!hasVoiced) continue;

        double targetFreq = midiToFreq(pitch.midiNumber);
        if (targetFreq <= 0) continue;

        int window = std::max(5, n / 4);
        if (window % 2 == 0) window++;

        auto centerline = dstools::movingAverage(segHz, window, true);

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

        int crossfade = std::clamp(n / 8, 3, n / 2);
        dstools::smoothstepCrossfade(newVals, segHz, crossfade);

        for (int i = 0; i < n; ++i) {
            if (startIdx + i < static_cast<int>(f0.values.size())) {
                f0.values[startIdx + i] = (newVals[i] > 0.0) ? hzToMhz(newVals[i]) : 0;
            }
        }
    }
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
