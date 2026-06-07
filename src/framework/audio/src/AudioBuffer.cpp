#include <dsfw/audio/AudioBuffer.h>

#include <algorithm>
#include <cassert>
#include <cstring>

namespace dsfw::audio {

    // -- Private helpers --

    const uint8_t *AudioBuffer::dataPtr() const {
        return std::visit(
            [](const auto &storage) -> const uint8_t * {
                return storage.data();
            },
            m_data);
    }

    uint8_t *AudioBuffer::dataPtrMutable() {
        assert(!isView() && "Cannot mutate a view buffer");
        if (isView()) return nullptr;
        auto *vec = std::get_if<std::vector<uint8_t>>(&m_data);
        return vec ? vec->data() : nullptr;
    }

    // -- Factory methods --

    AudioBuffer AudioBuffer::create(int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf;
        buf.m_frameCount = frameCount;
        buf.m_channelCount = channelCount;
        buf.m_format = format;
        auto byteSize = static_cast<size_t>(frameCount * channelCount * bytesPerSample(format));
        if (byteSize > 0) {
            buf.m_data = std::vector<uint8_t>(byteSize);
        } else {
            buf.m_data = std::vector<uint8_t>{};
        }
        return buf;
    }

    AudioBuffer AudioBuffer::fromCopy(const void *data, int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf = create(frameCount, channelCount, format);
        auto *vec = std::get_if<std::vector<uint8_t>>(&buf.m_data);
        if (vec && !vec->empty() && data) {
            std::memcpy(vec->data(), data, vec->size());
        }
        return buf;
    }

    AudioBuffer AudioBuffer::fromView(const void *data, int64_t frameCount, int channelCount, SampleFormat format) {
        AudioBuffer buf;
        buf.m_frameCount = frameCount;
        buf.m_channelCount = channelCount;
        buf.m_format = format;
        auto byteSize = static_cast<size_t>(frameCount * channelCount * bytesPerSample(format));
        if (byteSize > 0 && data) {
            buf.m_data = std::span<const uint8_t>(static_cast<const uint8_t *>(data), byteSize);
        } else {
            buf.m_data = std::span<const uint8_t>{};
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
        return buf;
    }

    // -- Accessors --

    size_t AudioBuffer::byteSize() const {
        return std::visit(
            [](const auto &storage) -> size_t { return storage.size(); },
            m_data);
    }

    bool AudioBuffer::isView() const {
        return std::holds_alternative<std::span<const uint8_t>>(m_data);
    }

    double AudioBuffer::durationSec(int sampleRate) const {
        if (sampleRate <= 0) return 0.0;
        return static_cast<double>(m_frameCount) / static_cast<double>(sampleRate);
    }

    // -- Const data access --

    const uint8_t *AudioBuffer::rawData() const {
        return dataPtr();
    }

    std::span<const float> AudioBuffer::floats() const {
        assert(m_format == SampleFormat::Float32);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<const float *>(dataPtr()), count};
    }

    std::span<const int16_t> AudioBuffer::int16s() const {
        assert(m_format == SampleFormat::Int16);
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<const int16_t *>(dataPtr()), count};
    }

    float AudioBuffer::sampleAt(int64_t frame, int channel) const {
        assert(m_format == SampleFormat::Float32);
        assert(frame >= 0 && frame < m_frameCount);
        assert(channel >= 0 && channel < m_channelCount);
        auto idx = static_cast<size_t>(frame * m_channelCount + channel);
        return reinterpret_cast<const float *>(dataPtr())[idx];
    }

    // -- Mutable data access --

    uint8_t *AudioBuffer::rawData() {
        return dataPtrMutable();
    }

    std::span<float> AudioBuffer::floats() {
        assert(m_format == SampleFormat::Float32);
        auto *ptr = dataPtrMutable();
        if (!ptr) return {};
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<float *>(ptr), count};
    }

    std::span<int16_t> AudioBuffer::int16s() {
        assert(m_format == SampleFormat::Int16);
        auto *ptr = dataPtrMutable();
        if (!ptr) return {};
        auto count = static_cast<size_t>(m_frameCount * m_channelCount);
        return {reinterpret_cast<int16_t *>(ptr), count};
    }

    // -- Utility --

    void AudioBuffer::zero() {
        auto *vec = std::get_if<std::vector<uint8_t>>(&m_data);
        assert(vec && "Cannot zero a view buffer");
        if (vec && !vec->empty()) {
            std::memset(vec->data(), 0, vec->size());
        }
    }

    AudioBuffer AudioBuffer::clone() const {
        return fromCopy(dataPtr(), m_frameCount, m_channelCount, m_format);
    }

    AudioBuffer AudioBuffer::slice(int64_t startFrame, int64_t frameCount) const {
        assert(startFrame >= 0 && startFrame < m_frameCount);
        // Clamp to available frames
        auto availFrames = m_frameCount - startFrame;
        frameCount = std::min(frameCount, availFrames);
        if (frameCount <= 0) return AudioBuffer{};

        auto bps = bytesPerSample(m_format);
        auto byteOffset = static_cast<size_t>(startFrame * m_channelCount * bps);
        auto byteLen = static_cast<size_t>(frameCount * m_channelCount * bps);

        if (isView()) {
            // View mode: create a sub-span over the same external data (zero-copy)
            AudioBuffer buf;
            buf.m_frameCount = frameCount;
            buf.m_channelCount = m_channelCount;
            buf.m_format = m_format;
            auto &span = std::get<std::span<const uint8_t>>(m_data);
            buf.m_data = span.subspan(byteOffset, byteLen);
            return buf;
        }
        // Owned mode: copy the sliced data
        return fromCopy(dataPtr() + byteOffset, frameCount, m_channelCount, m_format);
    }

} // namespace dsfw::audio