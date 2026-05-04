/// @file AudacityMarkerIO.h
/// @brief Read/write Audacity label track format (.txt) for slice points.

#pragma once

#include <QString>
#include <vector>

namespace dstools {

/// @brief Serialization/deserialization of Audacity marker (label track) files.
///
/// Format: each line is "start\tend\tlabel\n" (tab-separated).
/// For point markers: start == end. Times in seconds (decimal).
namespace AudacityMarkerIO {

/// Read marker times from an Audacity label file.
/// @return sorted list of marker times in seconds.
std::vector<double> read(const QString &path, QString &error);

/// Write marker times to an Audacity label file.
/// Labels are generated as "001", "002", etc.
bool write(const QString &path, const std::vector<double> &times, QString &error);

} // namespace AudacityMarkerIO
} // namespace dstools
