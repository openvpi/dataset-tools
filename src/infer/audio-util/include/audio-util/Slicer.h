#ifndef AUDIOSLICER_H
#define AUDIOSLICER_H

#include <audio-util/AudioUtilGlobal.h>
#include <cstdint>
#include <string>
#include <vector>

namespace AudioUtil
{
    using MarkerList = std::vector<std::pair<int64_t, int64_t>>;

    enum class AUDIO_UTIL_EXPORT SlicerError {
        Ok = 0,
        InvalidArgument,
        AudioError
    };

    struct AUDIO_UTIL_EXPORT SlicerParams {
        double threshold = -40.0;
        int minLength = 5000;
        int minInterval = 300;
        int hopSize = 20;
        int maxSilKept = 5000;
    };

    class AUDIO_UTIL_EXPORT Slicer {
    public:
        Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
               int maxSilKept);

        static Slicer fromMilliseconds(int sampleRate, const SlicerParams &params);

        MarkerList slice(const std::vector<float> &samples) const;

        SlicerError errorCode() const;
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

        static std::vector<double> get_rms(const std::vector<float> &samples, int frame_length, int hop_length);
    };
} // namespace AudioUtil
#endif // AUDIOSLICER_H
