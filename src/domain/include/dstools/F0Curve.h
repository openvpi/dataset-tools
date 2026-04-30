#pragma once

#include <vector>

namespace dstools {

/// F0 curve data — equally-spaced fundamental frequency samples.
/// Values are in Hz or MIDI. 0.0 indicates unvoiced (silence).
struct F0Curve {
    double timestep = 0.0;          ///< Seconds between samples
    std::vector<double> values;      ///< F0 value sequence, 0.0 = unvoiced

    /// Total duration in seconds.
    double totalDuration() const;

    /// Sample count.
    int sampleCount() const { return static_cast<int>(values.size()); }

    /// Get F0 value at a given time (linear interpolation).
    double getValueAtTime(double timeSec) const;

    /// Get F0 values in a time range.
    std::vector<double> getRange(double startTime, double endTime) const;

    /// Set F0 values starting at a given time (edit operation).
    void setRange(double startTime, const std::vector<double> &newValues);

    /// Check if empty.
    bool isEmpty() const { return values.empty(); }
};

} // namespace dstools
