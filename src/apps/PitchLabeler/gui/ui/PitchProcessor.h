#pragma once

#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>

#include <cstdint>
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

class PitchProcessor {
public:
    static std::vector<double> movingAverage(const std::vector<double> &values, int window);

    static double getRestMidi(const DSFile &ds, int index);

    static void applyModulationDriftPreview(
        DSFile &ds,
        const std::vector<int32_t> &preAdjustF0,
        const std::set<int> &selectedNotes,
        double modulationAmount,
        double driftAmount);
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
