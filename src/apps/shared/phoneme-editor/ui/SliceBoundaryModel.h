/// @file SliceBoundaryModel.h
/// @brief Simple single-tier boundary model for slicer pages.

#pragma once

#include "IBoundaryModel.h"

using ::dstools::IBoundaryModel;
using ::dstools::OutOfBoundsRange;

#include <vector>

namespace dstools {
    namespace phonemelabeler {
        using namespace dsfw;

        /// @brief Single-tier boundary model backed by a sorted vector of time points.
        ///
        /// Used by DsSlicerPage to drive the same WaveformChartPanel/SpectrogramChartPanel
        /// as PhonemeEditor without any TextGrid or binding dependency.
        class SliceBoundaryModel : public IBoundaryModel {
        public:
            SliceBoundaryModel() = default;

            void setAllowOverlap(bool allow) {
                m_allowOverlap = allow;
            }
            [[nodiscard]] bool allowOverlap() const {
                return m_allowOverlap;
            }

            /// @brief Set slice boundary times (in seconds). Converted to microseconds internally.
            void setSlicePoints(const std::vector<double> &pointsSec);

            /// @brief Get slice boundary times in seconds.
            [[nodiscard]] const std::vector<double> &slicePointsSec() const {
                return m_pointsSec;
            }

            /// @brief Set total audio duration in seconds.
            void setDuration(double durationSec);

            // IBoundaryModel
            [[nodiscard]] int tierCount() const override {
                return 1;
            }
            [[nodiscard]] int activeTierIndex() const override {
                return 0;
            }
            [[nodiscard]] int boundaryCount(int tierIndex) const override;
            [[nodiscard]] TimePos boundaryTime(int tierIndex, int boundaryIndex) const override;
            void moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) override;
            [[nodiscard]] TimePos totalDuration() const override;
            [[nodiscard]] bool supportsBinding() const override {
                return false;
            }
            [[nodiscard]] bool supportsInsert() const override {
                return true;
            }

            [[nodiscard]] TimePos clampBoundaryTime(int tierIndex, int boundaryIndex,
                                                    TimePos proposedTime) const override;
            [[nodiscard]] std::vector<OutOfBoundsRange> getOutOfBoundsRanges(
                int tierIndex) const override;
            [[nodiscard]] TimePos snapToNearestBoundary(int tierIndex, TimePos proposedTime,
                                                        TimePos snapThreshold) const override;
            [[nodiscard]] TimePos snapToNearestBoundaryPixels(int tierIndex, TimePos proposedTime,
                                                              double pixelsPerSecond,
                                                              double pixelThreshold) const override;

        private:
            std::vector<double> m_pointsSec; ///< Sorted slice times in seconds.
            double m_durationSec = 0.0;
            bool m_allowOverlap = false;
        };

    } // namespace phonemelabeler
} // namespace dstools
