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
    template <typename Iterator>
    static inline int64_t argmin(const Iterator &begin, const Iterator &end) {
        assert(begin != end && "Empty iterator range");
        return std::distance(begin, std::min_element(begin, end));
    }

    template <typename T>
    static inline T divIntRound(T n, T d) {
        return ((n < 0) ^ (d < 0)) ? ((n - (d / 2)) / d) : ((n + (d / 2)) / d);
    }

    static inline std::vector<double> get_rms_impl_basic(const std::vector<float> &samples, const int frame_length,
                                                         const int hop_length) {
        std::vector<double> output;
        const size_t output_size = samples.size() / hop_length;
        output.reserve(output_size);

        for (size_t i = 0; i < output_size; ++i) {
            const bool is_underflow = static_cast<int>(i * hop_length) < frame_length / 2;
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

        for (; j + simd_width <= index_end; j += simd_width) {
            xsimd::batch<float> sample_batch = xsimd::load_unaligned(&arr[j]);
            xsimd::batch<float> squared = sample_batch * sample_batch;
            local_sum += xsimd::reduce_add(squared);
        }

        for (; j < index_end; ++j) {
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
            const bool is_underflow = static_cast<int>(i * hop_length) < frame_length / 2;
            const size_t start = is_underflow ? 0 : (i * hop_length - frame_length / 2);
            const size_t end = (std::min)(samples.size(), i * hop_length - frame_length / 2 + frame_length);

            const double sum = simd_sum(samples, start, end);

            output.push_back(std::sqrt(sum / frame_length));
        }

        return output;
    }
#endif

    Slicer::Slicer(int sampleRate, float threshold, int hopSize, int winSize, int minLength, int minInterval,
                   int maxSilKept) :
        m_sampleRate(sampleRate), m_threshold(threshold), m_hopSize(hopSize), m_winSize(winSize), m_minLength(minLength),
        m_minInterval(minInterval), m_maxSilKept(maxSilKept), m_errorCode(SlicerError::Ok) {}

    Slicer Slicer::fromMilliseconds(int sampleRate, const SlicerParams &params) {
        Slicer s(sampleRate, 0.0f, 0, 0, 0, 0, 0);
        s.m_errorCode = SlicerError::Ok;

        if (!((params.minLength >= params.minInterval) && (params.minInterval >= params.hopSize)) ||
            (params.maxSilKept < params.hopSize)) {
            s.m_errorCode = SlicerError::InvalidArgument;
            s.m_errorMsg = "ValueError: The following conditions must be satisfied: "
                           "(m_minLength >= m_minInterval >= m_hopSize) and (m_maxSilKept >= m_hopSize).";
            return s;
        }

        if (sampleRate <= 0) {
            s.m_errorCode = SlicerError::AudioError;
            s.m_errorMsg = "Invalid sample rate!";
            return s;
        }

        s.m_sampleRate = sampleRate;
        s.m_threshold = std::pow(10, params.threshold / 20.0);
        s.m_hopSize = divIntRound<int64_t>(static_cast<int64_t>(params.hopSize) * sampleRate, 1000LL);
        s.m_winSize = static_cast<int>(std::min(
            divIntRound<int64_t>(static_cast<int64_t>(params.minInterval) * sampleRate, 1000LL),
            static_cast<int64_t>(4) * s.m_hopSize));
        s.m_minLength = static_cast<int>(divIntRound<int64_t>(
            static_cast<int64_t>(params.minLength) * sampleRate, 1000LL * s.m_hopSize));
        s.m_minInterval = static_cast<int>(divIntRound<int64_t>(
            static_cast<int64_t>(params.minInterval) * sampleRate, 1000LL * s.m_hopSize));
        s.m_maxSilKept = static_cast<int>(divIntRound<int64_t>(
            static_cast<int64_t>(params.maxSilKept) * sampleRate, 1000LL * s.m_hopSize));

        return s;
    }

    std::vector<double> Slicer::get_rms(const std::vector<float> &samples, const int frame_length,
                                        const int hop_length) {
#ifdef AUDIOUTIL_ENABLE_XSIMD
        return get_rms_impl_xsimd(samples, frame_length, hop_length);
#else
        return get_rms_impl_basic(samples, frame_length, hop_length);
#endif
    }

    MarkerList Slicer::slice(const std::vector<float> &samples) const {
        if (m_errorCode != SlicerError::Ok) {
            return {};
        }

        if ((samples.size() + m_hopSize - 1) / m_hopSize <= static_cast<size_t>(m_minLength)) {
            return {{0, static_cast<int64_t>(samples.size())}};
        }

        auto rms_list = get_rms(samples, m_winSize, m_hopSize);
        MarkerList sil_tags;
        int64_t silence_start = -1;
        int64_t clip_start = 0;

        for (int64_t i = 0; i < static_cast<int64_t>(rms_list.size()); ++i) {
            const double rms = rms_list[i];

            if (rms < m_threshold) {
                if (silence_start < 0) {
                    silence_start = i;
                }
                continue;
            }

            if (silence_start < 0) {
                continue;
            }

            bool is_leading_silence = silence_start == 0 && i > m_maxSilKept;
            bool need_slice_middle = i - silence_start >= m_minInterval && i - clip_start >= m_minLength;

            if (!is_leading_silence && !need_slice_middle) {
                silence_start = -1;
                continue;
            }

            if (i - silence_start <= m_maxSilKept) {
                int64_t pos = argmin(rms_list.begin() + silence_start, rms_list.begin() + i + 1);
                pos += silence_start;
                sil_tags.emplace_back((silence_start == 0 ? 0 : pos), pos);
                clip_start = pos;
            } else if (i - silence_start <= m_maxSilKept * 2) {
                int64_t overlap_start = std::max(silence_start, i - m_maxSilKept);
                int64_t pos = argmin(rms_list.begin() + overlap_start,
                                     rms_list.begin() + std::min(static_cast<int64_t>(rms_list.size()),
                                                                  silence_start + m_maxSilKept + 1));
                pos += overlap_start;
                int64_t pos_l =
                    argmin(rms_list.begin() + silence_start, rms_list.begin() + silence_start + m_maxSilKept + 1);
                int64_t pos_r = argmin(rms_list.begin() + i - m_maxSilKept, rms_list.begin() + i + 1);
                pos_l += silence_start;
                pos_r += i - m_maxSilKept;

                if (silence_start == 0) {
                    clip_start = pos_r;
                    sil_tags.emplace_back(0, clip_start);
                } else {
                    clip_start = std::max(pos_r, pos);
                    sil_tags.emplace_back(std::min(pos_l, pos), clip_start);
                }
            } else {
                int64_t pos_l =
                    argmin(rms_list.begin() + silence_start, rms_list.begin() + silence_start + m_maxSilKept + 1);
                int64_t pos_r = argmin(rms_list.begin() + i - m_maxSilKept, rms_list.begin() + i + 1);
                pos_l += silence_start;
                pos_r += i - m_maxSilKept;

                if (silence_start == 0) {
                    sil_tags.emplace_back(0, pos_r);
                } else {
                    sil_tags.emplace_back(pos_l, pos_r);
                }

                clip_start = pos_r;
            }

            silence_start = -1;
        }

        if (silence_start >= 0 && static_cast<int64_t>(rms_list.size()) - silence_start >= m_minInterval) {
            const int64_t silence_end =
                (std::min)(static_cast<int64_t>(rms_list.size() - 1), silence_start + m_maxSilKept);
            int64_t pos = argmin(rms_list.begin() + silence_start, rms_list.begin() + silence_end + 1);
            pos += silence_start;
            sil_tags.emplace_back(pos, rms_list.size() + 1);
        }

        if (sil_tags.empty()) {
            return {{0, static_cast<int64_t>(samples.size())}};
        } else {
            MarkerList chunks;

            if (sil_tags[0].first > 0) {
                chunks.emplace_back(0, sil_tags[0].first * m_hopSize);
            }

            for (size_t i = 0; i < sil_tags.size() - 1; ++i) {
                chunks.emplace_back(sil_tags[i].second * m_hopSize, sil_tags[i + 1].first * m_hopSize);
            }

            if (sil_tags.back().second < static_cast<int64_t>(rms_list.size())) {
                chunks.emplace_back(sil_tags.back().second * m_hopSize, static_cast<int64_t>(rms_list.size()) * m_hopSize);
            }
            return chunks;
        }
    }

    SlicerError Slicer::errorCode() const { return m_errorCode; }

    std::string Slicer::errorMessage() const { return m_errorMsg; }
} // namespace AudioUtil
