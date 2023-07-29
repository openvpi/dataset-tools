#include <QString>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "slicer.h"

template<class T>
inline T divIntRound(T n, T d);

template<class T>
inline qint64 argmin_range_view(const std::vector<T>& v, qint64 begin, qint64 end);

template<class T>
inline std::vector<T> multichannel_to_mono(const std::vector<T>& v, unsigned int channels);

template<class T>
inline std::vector<double> get_rms(const std::vector<T>& arr, qint64 frame_length = 2048, qint64 hop_length = 512);

class MovingRMS {
private:
    qint64 m_windowSize;
    qint64 m_numElements;
    double m_squareSum;
    std::queue<double> m_queue;
public:
    MovingRMS(qint64 windowSize);
    void push(double num);
    double rms();
    qint64 capacity();
    qint64 size();
};

MovingRMS::MovingRMS(qint64 windowSize) {
    m_windowSize = windowSize;
    m_numElements = 0;
    m_squareSum = 0.0;
}

void MovingRMS::push(double num) {
    double frontItem = 0.0;
    double numSquared = num * num;
    if (m_numElements < m_windowSize) {
        m_queue.push(numSquared);
        ++m_numElements;
    } else {
        frontItem = m_queue.front();
        m_queue.pop();
        m_queue.push(numSquared);
    }
    m_squareSum += numSquared - frontItem;
}

double MovingRMS::rms() {
    if ((m_windowSize == 0) || (m_squareSum < 0)) {
        return 0.0;
    }
    return std::sqrt(std::max(0.0, (double) m_squareSum / (double) m_windowSize));
}

qint64 MovingRMS::capacity() {
    return m_windowSize;
}

qint64 MovingRMS::size() {
    return m_numElements;
}

Slicer::Slicer(QsApi::IAudioDecoder *decoder, double threshold, qint64 minLength, qint64 minInterval, qint64 hopSize, qint64 maxSilKept)
{
    m_errCode = SlicerErrorCode::SLICER_OK;
    if ((!((minLength >= minInterval) && (minInterval >= hopSize))) || (maxSilKept < hopSize))
    {
        // The following condition must be satisfied: m_minLength >= m_minInterval >= m_hopSize
        // The following condition must be satisfied: m_maxSilKept >= m_hopSize
        m_errCode = SlicerErrorCode::SLICER_INVALID_ARGUMENT;
    }

    m_decoder = decoder;
    m_decoder->SetPosition(0);
    int sr = m_decoder->Format().SampleRate();
    m_threshold = std::pow(10, threshold / 20.0);
    m_hopSize = divIntRound<qint64>(hopSize * (qint64)sr, (qint64)1000);
    m_winSize = std::min(divIntRound<qint64>(minInterval * (qint64)sr, (qint64)1000), (qint64)4 * m_hopSize);
    m_minLength = divIntRound<qint64>(minLength * (qint64)sr, (qint64)1000 * m_hopSize);
    m_minInterval = divIntRound<qint64>(minInterval * (qint64)sr, (qint64)1000 * m_hopSize);
    m_maxSilKept = divIntRound<qint64>(maxSilKept * (qint64)sr, (qint64)1000 * m_hopSize);
}


std::vector<std::tuple<qint64, qint64>>
Slicer::slice()
{
    if (m_errCode == SlicerErrorCode::SLICER_INVALID_ARGUMENT)
    {
        return {};
    }

    auto frames = m_decoder->TotalSamples();
    auto channels = m_decoder->Format().Channels();

    if ((frames + m_hopSize - 1) / m_hopSize <= m_minLength)
    {
        return {{ 0, frames }};
    }

    auto rms_size = frames / m_hopSize + 1;
    std::vector<double> rms_list(rms_size);
    qint64 rms_index = 0;
    
    MovingRMS movingRms((int)m_winSize);
    auto padding = m_winSize / 2;
    for (auto i = 0; i < padding; i++) {
        movingRms.push(0.0);
    }

    {
        std::vector<float> initialBuffer(padding * channels);
        int samplesRead = m_decoder->Read(initialBuffer.data(), 0, initialBuffer.size());
        int framesRead = samplesRead / channels;
        for (int i = 0; i < framesRead; i++) {
            float monoSample = 0.0f;
            for (int j = 0; j < channels; j++) {
                monoSample += initialBuffer[i * channels + j] / (float)channels;
            }
            movingRms.push(monoSample);
        }
    }

    rms_list[rms_index++] = movingRms.rms();

    std::vector<float> buffer(m_hopSize * channels);

    int samplesRead = 0;
    do {
        samplesRead = m_decoder->Read(buffer.data(), 0, buffer.size());
        if (samplesRead == 0) {
            break;
        }
        int framesRead = samplesRead / channels;
        for (int i = 0; i < framesRead; i++) {
            float monoSample = 0.0f;
            for (int j = 0; j < channels; j++) {
                monoSample += buffer[i * channels + j] / (float)channels;
            }
            movingRms.push(monoSample);
        }
        for (qint64 i = framesRead; i < buffer.size() / channels; i++) {
            movingRms.push(0.0);
        }
        rms_list[rms_index++] = movingRms.rms();
        //qDebug() << samplesRead << "<->" << rms_list[rms_index - 1];
    } while (rms_index < rms_list.size());

    while (rms_index < rms_list.size()) {
        for (qint64 i = 0; i < m_hopSize; i++) {
            movingRms.push(0.0);
        }
        rms_list[rms_index++] = movingRms.rms();
    }

    //qint64 frames = waveform.size() / channels;
    //std::vector<float> samples = multichannel_to_mono<float>(waveform, channels);


    //std::vector<double> rms_list = get_rms<float>(samples, (qint64) m_winSize, (qint64) m_hopSize);

    std::vector<std::tuple<qint64, qint64>> sil_tags;
    qint64 silence_start = 0;
    bool has_silence_start = false;
    qint64 clip_start = 0;

    qint64 pos = 0, pos_l = 0, pos_r = 0;

    for (qint64 i = 0; i < rms_list.size(); i++)
    {
        double rms = rms_list[i];
        // Keep looping while frame is silent.
        if (rms < m_threshold)
        {
            // Record start of silent frames.
            if (!has_silence_start)
            {
                silence_start = i;
                has_silence_start = true;
            }
            continue;
        }
        // Keep looping while frame is not silent and silence start has not been recorded.
        if (!has_silence_start)
        {
            continue;
        }
        // Clear recorded silence start if interval is not enough or clip is too short
        bool is_leading_silence = ((silence_start == 0) && (i > m_maxSilKept));
        bool need_slice_middle = (
                ( (i - silence_start) >= m_minInterval) &&
                ( (i - clip_start) >= m_minLength) );
        if ((!is_leading_silence) && (!need_slice_middle))
        {
            has_silence_start = false;
            continue;
        }

        // Need slicing. Record the range of silent frames to be removed.
        if ((i - silence_start) <= m_maxSilKept)
        {
            pos = argmin_range_view<double>(rms_list, silence_start, i + 1) + silence_start;
            if (silence_start == 0)
            {
                sil_tags.emplace_back(0, pos);
            }
            else
            {
                sil_tags.emplace_back(pos, pos);
            }
            clip_start = pos;
        }
        else if ((i - silence_start) <= (m_maxSilKept * 2))
        {
            pos = argmin_range_view<double>(rms_list, i - m_maxSilKept, silence_start + m_maxSilKept + 1);
            pos += i - m_maxSilKept;
            pos_l = argmin_range_view<double>(rms_list, silence_start, silence_start + m_maxSilKept + 1) + silence_start;
            pos_r = argmin_range_view<double>(rms_list, i - m_maxSilKept, i + 1) + i - m_maxSilKept;
            if (silence_start == 0)
            {
                clip_start = pos_r;
                sil_tags.emplace_back(0, clip_start);
            }
            else
            {
                clip_start = std::max(pos_r, pos);
                sil_tags.emplace_back(std::min(pos_l, pos), clip_start);
            }
        }
        else {
            pos_l = argmin_range_view<double>(rms_list, silence_start, silence_start + m_maxSilKept + 1) + silence_start;
            pos_r = argmin_range_view<double>(rms_list, i - m_maxSilKept, i + 1) + i - m_maxSilKept;
            if (silence_start == 0) {
                sil_tags.emplace_back(0, pos_r);
            }
            else {
                sil_tags.emplace_back(pos_l, pos_r);
            }
            clip_start = pos_r;
        }
        has_silence_start = false;
    }
    // Deal with trailing silence.
    qint64 total_frames = rms_list.size();
    if (has_silence_start && ((total_frames - silence_start) >= m_minInterval)) {
        qint64 silence_end = std::min(total_frames - 1, silence_start + m_maxSilKept);
        pos = argmin_range_view<double>(rms_list, silence_start, silence_end + 1) + silence_start;
        sil_tags.emplace_back(pos, total_frames + 1);
    }
    // Apply and return slices.
    if (sil_tags.empty()) {
        return {{ 0, frames }};
    }
    else {
        std::vector<std::tuple<qint64, qint64>> chunks;
        qint64 begin = 0, end = 0;
        qint64 s0 = std::get<0>(sil_tags[0]);
        if (s0 > 0) {
            begin = 0;
            end = s0;
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        for (auto i = 0; i < sil_tags.size() - 1; i++) {
            begin = std::get<1>(sil_tags[i]);
            end = std::get<0>(sil_tags[i + 1]);
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        if (std::get<1>(sil_tags.back()) < total_frames) {
            begin = std::get<1>(sil_tags.back());
            end = total_frames;
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        return chunks;
    }
}

SlicerErrorCode Slicer::getErrorCode() {
    return m_errCode;
}

template<class T>
inline std::vector<double> get_rms(const std::vector<T>& arr, qint64 frame_length, qint64 hop_length) {
    qint64 arr_length = arr.size();

    qint64 padding = frame_length / 2;

    qint64 rms_size = arr_length / hop_length + 1;

    std::vector<double> rms = std::vector<double>(rms_size);

    qint64 left = 0;
    qint64 right = 0;
    qint64 hop_count = 0;

    qint64 rms_index = 0;
    double val = 0;

    // Initial condition: the frame is at the beginning of padded array
    while ((right < padding) && (right < arr_length)) {
        val += (double)arr[right] * arr[right];
        right++;
    }
    rms[rms_index++] = (std::sqrt(std::max(0.0, (double)val / (double)frame_length)));

    // Left side or right side of the frame has not touched the sides of original array
    while ((right < frame_length) && (right < arr_length) && (rms_index < rms_size)) {
        val += (double)arr[right] * arr[right];
        hop_count++;
        if (hop_count == hop_length) {
            rms[rms_index++] = (std::sqrt(std::max(0.0, (double)val / (double)frame_length)));
            hop_count = 0;
        }
        right++;  // Move right 1 step at a time.
    }

    if (frame_length < arr_length) {
        while ((right < arr_length) && (rms_index < rms_size)) {
            val += (double)arr[right] * arr[right] - (double)arr[left] * arr[left];
            hop_count++;
            if (hop_count == hop_length) {
                rms[rms_index++] = (std::sqrt(std::max(0.0, (double)val / (double)frame_length)));
                hop_count = 0;
            }
            left++;
            right++;
        }
    }
    else {
        while ((right < frame_length) && (rms_index < rms_size)) {
            hop_count++;
            if (hop_count == hop_length) {
                rms[rms_index++] = (std::sqrt(std::max(0.0, (double)val / (double)frame_length)));
                hop_count = 0;
            }
            right++;
        }
    }

    while ((left < arr_length) && (rms_index < rms_size)/* && (right < arr_length + padding)*/) {
        val -= (double)arr[left] * arr[left];
        hop_count++;
        if (hop_count == hop_length) {
            rms[rms_index++] = (std::sqrt(std::max(0.0, (double)val / (double)frame_length)));
            hop_count = 0;
        }
        left++;
        right++;
    }

    return rms;
}

template<class T>
inline T divIntRound(T n, T d) {
    /*
     * Integer division rounding to the closest integer, without converting to floating point numbers.
     */
    // T should be an integer type (int, int64_t, qint64, ...)
    return ((n < 0) ^ (d < 0)) ? \
        ((n - (d / 2)) / d) : \
        ((n + (d / 2)) / d);
}

template<class T>
inline qint64 argmin_range_view(const std::vector<T>& v, qint64 begin, qint64 end) {
    // Ensure vector access is not out of bound
    auto size = v.size();
    if (begin > size)  begin = size;
    if (end > size)    end = size;
    if (begin >= end)  return 0;

    auto min_it = std::min_element(v.begin() + begin, v.begin() + end);
    return std::distance(v.begin() + begin, min_it);
}

template<class T>
inline std::vector<T> multichannel_to_mono(const std::vector<T>& v, unsigned int channels) {
    qint64 frames = v.size() / channels;
    std::vector<T> out(frames);

    for (qint64 i = 0; i < frames; i++) {
        T s = 0;
        for (unsigned int j = 0; j < channels; j++) {
            s += (T)v[i * channels + j] / (T)channels;
        }
        out[i] = s;
    }

    return out;
}