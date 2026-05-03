/// @file SliceBoundaryModel.cpp
/// @brief Simple single-tier boundary model for slicer pages.

#include "SliceBoundaryModel.h"

#include <algorithm>

namespace dstools {
namespace phonemelabeler {

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
    if (tierIndex != 0 || boundaryIndex < 0 ||
        boundaryIndex >= static_cast<int>(m_pointsSec.size()))
        return 0;
    return static_cast<TimePos>(m_pointsSec[boundaryIndex] * kUsPerSec);
}

void SliceBoundaryModel::moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) {
    if (tierIndex != 0 || boundaryIndex < 0 ||
        boundaryIndex >= static_cast<int>(m_pointsSec.size()))
        return;
    m_pointsSec[boundaryIndex] = static_cast<double>(newTime) / kUsPerSec;
}

TimePos SliceBoundaryModel::totalDuration() const {
    return static_cast<TimePos>(m_durationSec * kUsPerSec);
}

} // namespace phonemelabeler
} // namespace dstools
