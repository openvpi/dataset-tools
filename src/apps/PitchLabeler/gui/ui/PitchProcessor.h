#pragma once

#include <dstools/PitchUtils.h>

#include <memory>
#include <set>
#include <vector>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {

using dstools::parseNoteName;
using dstools::freqToMidi;
using dstools::midiToFreq;
using dstools::shiftNoteCents;

/// Pure C++ pitch processing algorithms — no Qt widget dependencies.
/// Operates on DSFile data and returns results.
class PitchProcessor {
public:
    /// Moving-average filter (skips zero/unvoiced values).
    static std::vector<double> movingAverage(const std::vector<double> &values, int window);

    /// Get rest-note MIDI from nearest non-rest neighbor.
    static double getRestMidi(const DSFile &ds, int index);

    /// Apply modulation/drift preview to f0 values in-place.
    /// @param ds           DSFile whose f0.values will be modified
    /// @param preAdjustF0  snapshot taken before drag started
    /// @param selectedNotes set of selected note indices
    /// @param modulationAmount  modulation scale factor
    /// @param driftAmount       drift scale factor
    static void applyModulationDriftPreview(
        DSFile &ds,
        const std::vector<double> &preAdjustF0,
        const std::set<int> &selectedNotes,
        double modulationAmount,
        double driftAmount);
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
