/// @file Slicer.h
/// @brief RMS-based audio slicer for silence-boundary detection.

#pragma once
#include <audio-util/AudioUtilGlobal.h>
#include <cstdint>
#include <string>
#include <vector>

namespace AudioUtil
{
    /// List of sample-range pairs (start, end).
    using MarkerList = std::vector<std::pair<int64_t, int64_t>>;

    /// @brief Configuration parameters for the audio slicer (millisecond-based).
    struct AUDIO_UTIL_EXPORT SlicerParams {
        double threshold = -40.0;  ///< RMS threshold in dB.
        int minLength = 5000;      ///< Minimum slice length in milliseconds.
        int minInterval = 300;     ///< Minimum silence interval in milliseconds.
        int hopSize = 20;          ///< Hop size in milliseconds.
        int maxSilKept = 5000;     ///< Maximum silence kept at boundaries in milliseconds.
    };

    /// @brief Detects silence boundaries using RMS energy analysis and returns sample-range pairs.
    class AUDIO_UTIL_EXPORT Slicer {
    public:
        /// @brief Construct a Slicer with sample-level parameters.
        /// @param sampleRate Audio sample rate in Hz.
        /// @param threshold Linear amplitude threshold (NOT dB).
        /// @param hopSize Hop size in samples.
        /// @param winSize Window size in samples.
        /// @param minLength Minimum slice length in hop-frames.
        /// @param minInterval Minimum silence interval in hop-frames.
        /// @param maxSilKept Maximum silence kept at boundaries in hop-frames.
        Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
               int maxSilKept);

        /// @brief Create a Slicer from millisecond-based parameters.
        /// @param sampleRate Audio sample rate in Hz.
        /// @param params Slicer configuration in milliseconds.
        /// @return Configured Slicer instance.
        static Slicer fromMilliseconds(int sampleRate, const SlicerParams &params);

        /// @brief Slice audio samples into non-silent regions.
        /// @param samples Input audio samples (mono).
        /// @return List of (start, end) sample-range pairs.
        MarkerList slice(const std::vector<float> &samples) const;

    private:
        int m_sampleRate;
        float m_threshold;
        int m_hopSize;
        int m_winSize;
        int m_minLength;
        int m_minInterval;
        int m_maxSilKept;

        /// @brief Compute RMS energy for each frame.
        static std::vector<double> get_rms(const std::vector<float> &samples, int frame_length, int hop_length);
    };
} // namespace AudioUtil
