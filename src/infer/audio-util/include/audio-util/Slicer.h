/// @file Slicer.h
/// @brief RMS-based audio slicer for silence-boundary detection.

#ifndef AUDIOSLICER_H
#define AUDIOSLICER_H

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

    /// @brief Configuration parameters for the audio slicer.
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
        /// @param threshold RMS threshold in dB.
        /// @param hopSize Hop size in samples.
        /// @param winSize Window size in samples.
        /// @param minLength Minimum slice length in samples.
        /// @param minInterval Minimum silence interval in samples.
        /// @param maxSilKept Maximum silence kept at boundaries in samples.
        Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
               int maxSilKept);

        /// @brief Create a Slicer from millisecond-based parameters.
        /// @param sampleRate Audio sample rate in Hz.
        /// @param params Slicer configuration in milliseconds.
        /// @return Configured Slicer instance.
        static Slicer fromMilliseconds(int sampleRate, const SlicerParams &params);

        /// @brief Slice audio samples into non-silent regions.
        /// @param samples Input audio samples.
        /// @return List of (start, end) sample-range pairs.
        MarkerList slice(const std::vector<float> &samples) const;

        /// @brief Get the current error code.
        /// @return SlicerError code.
        SlicerError errorCode() const;

        /// @brief Get the current error message.
        /// @return Error description string.
        std::string errorMessage() const;

    private:
        int sample_rate;
        double threshold;
        int hop_size;
        int win_size;
        int min_length;
        int min_interval;
        int max_sil_kept;
        SlicerError error_code_;
        std::string error_msg_;

        /// @brief Compute RMS energy for each frame.
        static std::vector<double> get_rms(const std::vector<float> &samples, int frame_length, int hop_length);
    };
} // namespace AudioUtil
#endif // AUDIOSLICER_H
