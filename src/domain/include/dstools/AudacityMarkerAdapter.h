/// @file AudacityMarkerAdapter.h
/// @brief Read/write Audacity label track format (.txt) for slice points.

#pragma once

#include <QString>
#include <dsfw/Result.h>
#include <vector>

namespace dstools {

    /// @brief Serialization/deserialization of Audacity marker (label track) files.
    ///
    /// Format: each line is "start\tend\tlabel\n" (tab-separated).
    /// For point markers: start == end. Times in seconds (decimal).
    class AudacityMarkerAdapter {
    public:
        [[nodiscard]] static Result<std::vector<double>> read(const QString &path);
        [[nodiscard]] static Result<void> write(const QString &path, const std::vector<double> &times);
    };

} // namespace dstools