#include <QString>
#include <algorithm>
#include <cmath>
#include <queue>
#include <tuple>
#include <vector>

#include <sndfile.hh>

#include "mathutils.h"
#include "slicer.h"

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
    explicit MovingRMS(qint64 windowSize);
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

Slicer::Slicer(SndfileHandle *decoder, double threshold, qint64 maxLength, qint64 minInterval, qint64 hopSize, qint64 maxSilKept)
{
    m_errCode = SlicerErrorCode::SLICER_OK;
    if (!((minInterval >= hopSize) && (maxSilKept >= hopSize) && (maxLength >= maxSilKept * 2)))
    {
        // The following condition must be satisfied: (m_minInterval >= m_hopSize) and (m_maxLength / 2 >= m_maxSilKept >= m_hopSize)
        m_errCode = SlicerErrorCode::SLICER_INVALID_ARGUMENT;
        m_errMsg = "ValueError: The following conditions must be satisfied: "
                   "(min_interval >= hop_size) and (max_length / 2 >= max_sil_kept >= hop_size).";
        return;
    }

    m_decoder = decoder;
    if (!m_decoder) {
        m_errCode = SlicerErrorCode::SLICER_AUDIO_ERROR;
        m_errMsg = "Invalid audio decoder!";
        return;
    }

    m_decoder->seek(0, SEEK_SET);
    int sr = m_decoder->samplerate();
    if (sr <= 0) {
        m_errCode = SlicerErrorCode::SLICER_AUDIO_ERROR;
        m_errMsg = "Invalid audio file!";
        return;
    }

    m_threshold = std::pow(10, threshold / 20.0);
    m_hopSize = divIntRound<qint64>(hopSize * (qint64)sr, (qint64)1000);
    m_winSize = std::min(divIntRound<qint64>(minInterval * (qint64)sr, (qint64)1000), (qint64)4 * m_hopSize);
    m_maxLength = divIntRound<qint64>(maxLength * (qint64)sr, (qint64)1000 * m_hopSize);
    m_minInterval = divIntRound<qint64>(minInterval * (qint64)sr, (qint64)1000 * m_hopSize);
    m_maxSilKept = divIntRound<qint64>(maxSilKept * (qint64)sr, (qint64)1000 * m_hopSize);
}


MarkerList Slicer::slice()
{
    if (m_errCode == SlicerErrorCode::SLICER_INVALID_ARGUMENT)
    {
        return {};
    }

    qint64 frames = m_decoder->frames();
    int channels = m_decoder->channels();

    if ((frames + m_hopSize - 1) / m_hopSize <= m_maxLength)
    {
        return {{ 0, frames }};
    }

    qint64 rms_size = frames / m_hopSize + 1;
    std::vector<double> rms_list(rms_size);
    qint64 rms_index = 0;
    
    MovingRMS movingRms(m_winSize);
    qint64 padding = m_winSize / 2;
    for (qint64 i = 0; i < padding; i++) {
        movingRms.push(0.0);
    }

    qint64 samplesRead = 0;
    {
        std::vector<double> initialBuffer(padding * channels);
        samplesRead = m_decoder->read(initialBuffer.data(), padding * channels);
        qint64 framesRead = samplesRead / channels;
        for (qint64 i = 0; i < framesRead; i++) {
            double monoSample = 0.0;
            for (int j = 0; j < channels; j++) {
                monoSample += initialBuffer[i * channels + j] / static_cast<double>(channels);
            }
            movingRms.push(monoSample);
        }
    }

    rms_list[rms_index++] = movingRms.rms();

    std::vector<double> buffer(m_hopSize * channels);

    do {
        samplesRead = m_decoder->read(buffer.data(), m_hopSize * channels);
        if (samplesRead == 0) {
            break;
        }
        qint64 framesRead = samplesRead / channels;
        for (qint64 i = 0; i < framesRead; i++) {
            double monoSample = 0.0;
            for (int j = 0; j < channels; j++) {
                monoSample += buffer[i * channels + j] / static_cast<double>(channels);
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

    // Scan for silence

    MarkerList silences;
    qint64 silence_start = -1<<30;
    bool was_silent = true; // If 0 is not silent, it detects an inf-length silence [-inf, 0) as opening guard.

    for (qint64 i = 0; i < rms_list.size(); i++)
    {
        if (rms_list[i] < m_threshold)
        { // This frame is silent.
            if (!was_silent)
            { // Start of silence.
                was_silent = true;
                silence_start = i;
            }
        }
        else
        { // This frame is not silent.
            if (was_silent)
            { // End of silence range [silence_start, i).
                was_silent = false;
                if (silence_start < 0)
                { // Prepend inf-length silence as left boundary guard.
                    qint64 pos_r = argmin_range_view<double>(rms_list, i - m_maxSilKept, i + 1);
                    silences.emplace_back(-1<<30, pos_r);
                }
                else
                {
                    // Find break points in this silence.
                    qint64 pos_l = argmin_range_view<double>(rms_list, silence_start, std::min(silence_start + m_maxSilKept, i) + 1);
                    qint64 pos_r = argmin_range_view<double>(rms_list, std::max(silence_start, i - m_maxSilKept), i + 1);
                    // assert(pos_l <= pos_r); // by simple mathematics
                    silences.emplace_back(pos_l, pos_r);
                }
            }
        }

    if (silences.empty()) {
        return {}; // entirely silent
    }
    // Append inf-length silence as right boundary guard.
    if (!was_silent)
    {
        silence_start = rms_list.size();
    }
    qint64 pos_l = argmin_range_view<double>(rms_list, silence_start, silence_start + m_maxSilKept + 1);
    silences.emplace_back(pos_l, 1<<30);

    std::vector<std::pair<qint64, qint64>> order(silences.size());
    std::vector<qint64> prev(silences.size()), next(silences.size()); // doubly linked list of silences
    for (qint64 i = 0; i < silences.size(); i++)
    {
        auto sil = silence[i];
        order[i] = std::make_pair(std::max(sil.second - sil.first, 0), i);
        prev[i] = i - 1;
        next[i] = i + 1;
    }
    std::sort(order.start(), order.end()); // sort silences from short to long

    for (auto o : order)
    {
        if (o.first >> 29) // length is inf
        {
            break;
        }
        qint64 i = o.second;
        qint64 pr = prev[i], nx = next[i];
        if (silences[nx].first - silences[pr].second <= m_maxLength)
        { // If possible, remove this short silence.
            next[pr] = nx;
            prev[nx] = pr;
        }
    }

    MarkerList chunks;
    for (qint64 i = 0; next[i] < silences.size(); i = next[i])
    {
        chunks.emplace_back(silences[i].second * m_hopSize, std::min(frames, silences[next[i]].first * m_hopSize));
    }
    return chunks;
}

SlicerErrorCode Slicer::getErrorCode() {
    return m_errCode;
}

QString Slicer::getErrorMsg() {
    return m_errMsg;
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
inline qint64 argmin_range_view(const std::vector<T>& v, qint64 begin, qint64 end) {
    // Return the absolute index in v, which is in range [begin, end) if possible
    // Ensure vector access is not out of bound
    auto size = v.size();
    if (begin < 0)     begin = 0;
    if (end > size)    end = size;
    if (begin >= end)  return begin;

    auto min_it = std::min_element(v.begin() + begin, v.begin() + end);
    return std::distance(v.begin(), min_it);
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