#include <dsfw/audio/AudioBuffer.h>

#include <algorithm>
#include <cassert>
#include <cstring>

namespace dsfw::audio {

    AudioBuffer AudioBuffer::create(int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf;
        buf.m_frameCount = frameCount;
        buf.m_channelCount = channelCount;
        buf.m_format = format;
        buf.m_isView = false;
        auto byteSize = static_cast<size_t>(frameCount * channelCount * bytesPerSample(format));
        if (byteSize > 0) {
            buf.m_data.resize(byteSize);
        }
        return buf;
    }

    AudioBuffer AudioBuffer::fromCopy(const void *data, int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf = create(frameCount, channelCount, format);
        if (!buf.m_data.empty() && data) {
            std::memcpy(buf.m_data.data(), data, buf.m_data.size());
        }
        return buf;
    }

    AudioBuffer AudioBuffer::fromView(const void *data, int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf;
        buf.m_frameCount = frameCount;
        buf.m_channelCount = channelCount;
        buf.m_format = format;
        buf.m_isView = true;
        auto byteSize = static_cast<size_t>(frameCount * channelCount * bytesPerSample(format));
        if (byteSize > 0 && data) {
            const auto *bytes = static_cast<const uint8_t *>(data);
            buf.m_data.assign(bytes, bytes + byteSize);
        }
        return buf;
    }

    AudioBuffer AudioBuffer::fromVector(std::vector<uint8_t> &&data, int64_t frameCount, int channelCount,
                                        SampleFormat format) {
        AudioBuffer buf;
        buf.m_data = std::move(data);
        buf.m_frameCount = frameCount;
        buf.m_channelCount = channelCount;
        buf.m_format = format;
        buf.m_isView = false;
        return buf;
    }

    double AudioBuffer::durationSec(int sampleRate) const {
        if (sampleRate <= 0) return 0.0;
        return static_cast<double>(m_frameCount) / static_cast<double>(sampleRate);
    }

    std::span<const float> AudioBuffer::floats() const {
        assert(m_format == SampleFormat::Float32);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<const float *>(m_data.data()), count};
    }

    std::span<float> AudioBuffer::floats() {
        assert(m_format == SampleFormat::Float32);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<float *>(m_data.data()), count};
    }

    std::span<const int16_t> AudioBuffer::int16s() const {
        assert(m_format == SampleFormat::Int16);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<const int16_t *>(m_data.data()), count};
    }

    std::span<int16_t> AudioBuffer::int16s() {
        assert(m_format == SampleFormat::Int16);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<int16_t *>(m_data.data()), count};
    }

    float AudioBuffer::sampleAt(int64_t frame, int channel) const {
        assert(m_format == SampleFormat::Float32);
        assert(frame >= 0 && frame < m_frameCount);
        assert(channel >= 0 && channel < m_channelCount);
        auto idx = frame * m_channelCount + channel;
        return reinterpret_cast<const float *>(m_data.data())[idx];
    }

    void AudioBuffer::zero() {
        if (!m_data.empty()) {
            std::memset(m_data.data(), 0, m_data.size());
        }
    }

    AudioBuffer AudioBuffer::clone() const {
        return fromCopy(m_data.data(), m_frameCount, m_channelCount, m_format);
    }

    AudioBuffer AudioBuffer::slice(int64_t startFrame, int64_t frameCount) const {
        assert(startFrame >= 0 && startFrame + frameCount <= m_frameCount);
        auto bps = bytesPerSample(m_format);
        auto byteOffset = static_cast<size_t>(startFrame * m_channelCount * bps);
        auto byteLen = static_cast<size_t>(frameCount * m_channelCount * bps);

        if (m_isView) {
            // View mode: create a new view over the sliced range
            AudioBuffer buf;
            buf.m_frameCount = frameCount;
            buf.m_channelCount = m_channelCount;
            buf.m_format = m_format;
            buf.m_isView = true;
            buf.m_data.assign(m_data.begin() + byteOffset, m_data.begin() + byteOffset + byteLen);
            return buf;
        }
        // Owned mode: copy the sliced data
        return fromCopy(m_data.data() + byteOffset, frameCount, m_channelCount, m_format);
    }

} // namespace dsfw::audio