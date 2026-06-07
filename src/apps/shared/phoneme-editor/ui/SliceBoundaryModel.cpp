/// @file SliceBoundaryModel.cpp
/// @brief Simple single-tier boundary model for slicer pages.

#include "SliceBoundaryModel.h"

#include <algorithm>

namespace dstools {
    namespace phonemelabeler {
        using namespace dsfw;

        static constexpr int64_t kUsPerSec = 1000000;

        void SliceBoundaryModel::setSlicePoints(const std::vector<double> &pointsSec) {
            m_pointsSec = pointsSec;
            std::sort(m_pointsSec.begin(), m_pointsSec.end());
        }

        void SliceBoundaryModel::setDuration(double durationSec) {
            m_durationSec = durationSec;
        }

        int SliceBoundaryModel::boundaryCount(int tierIndex) const {
            if (tierIndex != 0)
                return 0;
            return static_cast<int>(m_pointsSec.size());
        }

        TimePos SliceBoundaryModel::boundaryTime(int tierIndex, int boundaryIndex) const {
            if (tierIndex != 0 || boundaryIndex < 0 || boundaryIndex >= static_cast<int>(m_pointsSec.size()))
                return 0;
            return static_cast<TimePos>(m_pointsSec[boundaryIndex] * kUsPerSec);
        }

        void SliceBoundaryModel::moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) {
            if (tierIndex != 0 || boundaryIndex < 0 || boundaryIndex >= static_cast<int>(m_pointsSec.size()))
                return;
            m_pointsSec[boundaryIndex] = static_cast<double>(newTime) / kUsPerSec;
        }

        TimePos SliceBoundaryModel::totalDuration() const {
            return static_cast<TimePos>(m_durationSec * kUsPerSec);
        }

        TimePos SliceBoundaryModel::clampBoundaryTime(int tierIndex, int boundaryIndex, TimePos proposedTime) const {
            if (tierIndex != 0 || boundaryIndex < 0 || boundaryIndex >= static_cast<int>(m_pointsSec.size()))
                return proposedTime;

            double proposedSec = static_cast<double>(proposedTime) / kUsPerSec;

            if (m_allowOverlap) {
                double clamped = std::clamp(proposedSec, 0.0, m_durationSec);
                return static_cast<TimePos>(clamped * kUsPerSec);
            }

            double prevBoundary = (boundaryIndex > 0) ? m_pointsSec[boundaryIndex - 1] : 0.0;
            double nextBoundary = (boundaryIndex + 1 < static_cast<int>(m_pointsSec.size()))
                                      ? m_pointsSec[boundaryIndex + 1]
                                      : m_durationSec;

            // Boundary minimum gap: 1e-5 seconds = 0.01ms, prevents floating-point overlap
            static constexpr double kMinBoundaryGapSec = 1e-5;

            double minClamp = (boundaryIndex == 0) ? prevBoundary : prevBoundary + kMinBoundaryGapSec;
            double maxClamp =
                (boundaryIndex + 1 < static_cast<int>(m_pointsSec.size())) ? nextBoundary - kMinBoundaryGapSec : nextBoundary;
            double clamped = std::clamp(proposedSec, minClamp, maxClamp);
            return static_cast<TimePos>(clamped * kUsPerSec);
        }

        TimePos SliceBoundaryModel::snapToNearestBoundary(int tierIndex, TimePos proposedTime,
                                                          TimePos snapThreshold) const {
            if (tierIndex != 0)
                return proposedTime;

            double proposedSec = static_cast<double>(proposedTime) / kUsPerSec;
            double thresholdSec = static_cast<double>(snapThreshold) / kUsPerSec;

            double bestDist = thresholdSec;
            double bestTime = proposedSec;

            for (double bTime : m_pointsSec) {
                double dist = std::abs(bTime - proposedSec);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTime = bTime;
                }
            }

            return static_cast<TimePos>(bestTime * kUsPerSec);
        }

        TimePos SliceBoundaryModel::snapToNearestBoundaryPixels(int tierIndex, TimePos proposedTime,
                                                                double pixelsPerSecond, double pixelThreshold) const {
            if (tierIndex != 0 || pixelsPerSecond <= 0)
                return proposedTime;

            double proposedSec = static_cast<double>(proposedTime) / kUsPerSec;
            double thresholdSec = pixelThreshold / pixelsPerSecond;

            double bestDist = thresholdSec;
            double bestTime = proposedSec;

            for (double bTime : m_pointsSec) {
                double dist = std::abs(bTime - proposedSec);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestTime = bTime;
                }
            }

            return static_cast<TimePos>(bestTime * kUsPerSec);
        }

        std::vector<OutOfBoundsRange> SliceBoundaryModel::getOutOfBoundsRanges(int tierIndex) const {
            if (tierIndex != 0 || m_pointsSec.size() < 2)
                return {};

            std::vector<OutOfBoundsRange> ranges;
            double prev = 0.0;
            for (size_t i = 0; i < m_pointsSec.size(); ++i) {
                if (m_pointsSec[i] < prev) {
                    ranges.push_back({m_pointsSec[i], prev});
                }
                prev = m_pointsSec[i];
            }
            return ranges;
        }

    } // namespace phonemelabeler
} // namespace dstools
