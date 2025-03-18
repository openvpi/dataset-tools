#ifndef AUDIOSLICER_H
#define AUDIOSLICER_H

#include <audio-util/AudioUtilGlobal.h>
#include <cstdint>
#include <vector>

namespace AudioUtil
{
    using MarkerList = std::vector<std::pair<int64_t, int64_t>>;

    class AUDIO_UTIL_EXPORT Slicer {
    public:
        Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
               int maxSilKept);
        MarkerList slice(const std::vector<float> &samples) const;

    private:
        int sample_rate;
        float threshold;
        int hop_size;
        int win_size;
        int min_length;
        int min_interval;
        int max_sil_kept;

        static std::vector<double> get_rms(const std::vector<float> &samples, int frame_length, int hop_length);
    };
} // namespace AudioUtil
#endif // AUDIOSLICER_H
