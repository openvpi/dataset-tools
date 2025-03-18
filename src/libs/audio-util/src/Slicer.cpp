#include <audio-util/Slicer.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <utility>

#ifdef AUDIOUTIL_ENABLE_XSIMD
#include <xsimd/xsimd.hpp>
#endif

namespace AudioUtil
{

    /**
     * @brief Returns the offset from `begin` to the first minimum element
     * @tparam Iterator Forward iterator type
     * @param begin Start of range (inclusive)
     * @param end End of range (exclusive)
     * @return Distance from `begin` to the minimum element
     * @pre `[begin, end)` must be non-empty (assert enforced)
     * @note For equivalent elements, returns the first occurrence
     */
    template <typename Iterator>
    static inline int64_t argmin(const Iterator &begin, const Iterator &end) {
        assert(begin != end && "Empty iterator range");
        return std::distance(begin, std::min_element(begin, end));
    }

    static inline std::vector<double> get_rms_impl_basic(const std::vector<float> &samples, const int frame_length,
                                                         const int hop_length) {
        std::vector<double> output;
        const size_t output_size = samples.size() / hop_length;
        output.reserve(output_size);

        for (size_t i = 0; i < output_size; ++i) {
            const bool is_underflow = i * hop_length < frame_length / 2;
            const size_t start = is_underflow ? 0 : (i * hop_length - frame_length / 2);
            const size_t end = (std::min)(samples.size(), i * hop_length - frame_length / 2 + frame_length);

            const double sum = std::accumulate(samples.begin() + start, samples.begin() + end, 0.0,
                                               [](const double acc, const float value) {
                                                   return acc + value * value;
                                               });
            output.push_back(std::sqrt(sum / frame_length));
        }

        return output;
    }

#ifdef AUDIOUTIL_ENABLE_XSIMD
    static inline double simd_sum(const std::vector<float> &arr, const size_t index_start, const size_t index_end) {
        if (index_start >= index_end) {
            return 0.0;
        }
        double local_sum = 0.0;
        constexpr size_t simd_width = xsimd::batch<float>::size;
        size_t j = index_start;

        // SIMD loop over the range [start, end)
        for (; j + simd_width <= index_end; j += simd_width) {
            // Load the batch of samples into SIMD registers
            xsimd::batch<float> sample_batch = xsimd::load_unaligned(&arr[j]);

            // Square the values
            xsimd::batch<float> squared = sample_batch * sample_batch;

            // Sum the values in the SIMD batch
            local_sum += xsimd::reduce_add(squared);
        }

        // Handle any remaining elements (if any)
        for (; j < index_end; ++j) {
            // Process the remaining scalar values
            local_sum += arr[j] * arr[j];
        }
        return local_sum;
    }

    static inline std::vector<double> get_rms_impl_xsimd(const std::vector<float> &samples, const int frame_length,
                                                         const int hop_length) {
        std::vector<double> output;
        const size_t output_size = samples.size() / hop_length;
        output.reserve(output_size);

        for (size_t i = 0; i < output_size; ++i) {
            const bool is_underflow = i * hop_length < frame_length / 2;
            const size_t start = is_underflow ? 0 : (i * hop_length - frame_length / 2);
            const size_t end = (std::min)(samples.size(), i * hop_length - frame_length / 2 + frame_length);

            const double sum = simd_sum(samples, start, end);

            output.push_back(std::sqrt(sum / frame_length));  // Calculate RMS for the frame
        }

        return output;
    }
#endif

    // https://github.com/stakira/OpenUtau/blob/master/OpenUtau.Core/Analysis/Some.cs
    Slicer::Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
                   int maxSilKept) :
        sample_rate(sampleRate), threshold(threshold), hop_size(hopSize), win_size(winSize), min_length(minLength),
        min_interval(minInterval), max_sil_kept(maxSilKept) {}

    std::vector<double> Slicer::get_rms(const std::vector<float> &samples, const int frame_length,
                                        const int hop_length) {
#ifdef AUDIOUTIL_ENABLE_XSIMD
        return get_rms_impl_xsimd(samples, frame_length, hop_length);
#else
        return get_rms_impl_basic(samples, frame_length, hop_length);
#endif
    }

    MarkerList Slicer::slice(const std::vector<float> &samples) const {
        if ((samples.size() + hop_size - 1) / hop_size <= min_length) {
            return {{0, samples.size()}};
        }

        auto rms_list = get_rms(samples, win_size, hop_size);
        MarkerList sil_tags;
        int64_t silence_start = -1;
        int64_t clip_start = 0;

        for (int64_t i = 0; i < rms_list.size(); ++i) {
            const double rms = rms_list[i];

            if (rms < threshold) {
                if (silence_start < 0) {
                    silence_start = i;
                }
                continue;
            }

            if (silence_start < 0) {
                continue;
            }

            bool is_leading_silence = silence_start == 0 && i > max_sil_kept;
            bool need_slice_middle = i - silence_start >= min_interval && i - clip_start >= min_length;

            if (!is_leading_silence && !need_slice_middle) {
                silence_start = -1;
                continue;
            }

            if (i - silence_start <= max_sil_kept) {
                int64_t pos = argmin(rms_list.begin() + silence_start, rms_list.begin() + i + 1);
                pos += silence_start;
                sil_tags.emplace_back((silence_start == 0 ? 0 : pos), pos);
                clip_start = pos;
            } else {
                int64_t pos_l =
                    argmin(rms_list.begin() + silence_start, rms_list.begin() + silence_start + max_sil_kept + 1);
                int64_t pos_r = argmin(rms_list.begin() + i - max_sil_kept, rms_list.begin() + i + 1);
                pos_l += silence_start;
                pos_r += i - max_sil_kept;

                if (silence_start == 0) {
                    sil_tags.emplace_back(0, pos_r);
                } else {
                    sil_tags.emplace_back(pos_l, pos_r);
                }

                clip_start = pos_r;
            }

            silence_start = -1;
        }

        if (silence_start >= 0 && rms_list.size() - silence_start >= min_interval) {
            const int64_t silence_end = (std::min)(static_cast<int64_t>(rms_list.size() - 1), silence_start + max_sil_kept);
            int64_t pos = argmin(rms_list.begin() + silence_start, rms_list.begin() + silence_end + 1);
            pos += silence_start;
            sil_tags.emplace_back(pos, rms_list.size() + 1);
        }

        if (sil_tags.empty()) {
            return {{0, samples.size()}};
        } else {
            MarkerList chunks;

            if (sil_tags[0].first > 0) {
                chunks.emplace_back(0, sil_tags[0].first * hop_size);
            }

            for (size_t i = 0; i < sil_tags.size() - 1; ++i) {
                chunks.emplace_back(sil_tags[i].second * hop_size, sil_tags[i + 1].first * hop_size);
            }

            if (sil_tags.back().second < rms_list.size()) {
                chunks.emplace_back(sil_tags.back().second * hop_size, rms_list.size() * hop_size);
            }
            return chunks;
        }
    }
} // namespace AudioUtil
