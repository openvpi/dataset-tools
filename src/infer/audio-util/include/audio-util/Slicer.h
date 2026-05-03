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

    /// @brief Error codes returned by the Slicer.
    enum class AUDIO_UTIL_EXPORT SlicerError {
        Ok = 0,            ///< No error.
        InvalidArgument,   ///< Invalid parameter value.
        AudioError         ///< Audio processing failure.
    };

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
        Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
               int maxSilKept);

        /// @brief Create a Slicer from millisecond-based parameters.
        static Slicer fromMilliseconds(int sampleRate, const SlicerParams &params);

        /// @brief Slice audio samples into non-silent regions.
        MarkerList slice(const std::vector<float> &samples) const;

        /// @brief Get the current error code.
        [[nodiscard]] SlicerError errorCode() const { return m_errorCode; }

        /// @brief Get the current error message.
        [[nodiscard]] std::string errorMessage() const { return m_errorMsg; }

    private:
        int m_sampleRate;
        float m_threshold;
        int m_hopSize;
        int m_winSize;
        int m_minLength;
        int m_minInterval;
        int m_maxSilKept;
        SlicerError m_errorCode = SlicerError::Ok;
        std::string m_errorMsg;

        /// @brief Compute RMS energy for each frame.
        static std::vector<double> get_rms(const std::vector<float> &samples, int frame_length, int hop_length);
    };
} // namespace AudioUtil
