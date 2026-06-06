#pragma once

#include <cstdint>
#include <dstools/PitchUtils.h>
#include <dsfw/TimePos.h>
#include <memory>
#include <set>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            using dstools::freqToMidi;
            using dstools::midiToFreq;
            using dstools::parseNoteName;
            using dstools::shiftNoteCents;

            class PitchProcessor {
            public:
                static std::vector<double> movingAverage(const std::vector<double> &values, int window);

                static double getRestMidi(const DsPitchDocument &ds, int index);

                static void applyModulationDriftPreview(DsPitchDocument &ds, const std::vector<float> &preAdjustF0,
                                                        const std::set<int> &selectedNotes, double modulationAmount,
                                                        double driftAmount);
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
