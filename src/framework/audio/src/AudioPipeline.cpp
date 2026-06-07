#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/audio/FfmpegAudioDecoder.h>
#include <dsfw/audio/SwresampleResampler.h>

#include <algorithm>
#include <vector>

namespace dsfw::audio {

    namespace {
        AudioBuffer mergeBuffers(const std::vector<AudioBuffer> &chunks) {
            if (chunks.empty()) {
                return AudioBuffer::create(0, 1, SampleFormat::Float32);
            }

            int channels = chunks[0].channelCount();
            SampleFormat fmt = chunks[0].format();
            int bps = bytesPerSample(fmt);

            int64_t totalFrames = 0;
            for (const auto &chunk : chunks) {
                totalFrames += chunk.frameCount();
            }

            std::vector<uint8_t> merged;
            merged.reserve(static_cast<size_t>(totalFrames * channels * bps));

            for (const auto &chunk : chunks) {
                merged.insert(merged.end(), chunk.rawData(), chunk.rawData() + chunk.byteSize());
            }

            return AudioBuffer::fromVector(std::move(merged), totalFrames, channels, fmt);
        }
    } // anonymous namespace

    AudioPipeline AudioPipeline::create() {
        return AudioPipeline(std::make_unique<FfmpegAudioDecoder>(), std::make_unique<SwresampleResampler>());
    }

    AudioPipeline::AudioPipeline(std::unique_ptr<IAudioDecoder> decoder,
                             std::unique_ptr<IAudioResampler> resampler)
        : m_decoder(std::move(decoder)), m_resampler(std::move(resampler)) {
    }

    dsfw::Result<AudioFormatInfo> AudioPipeline::probe(const std::string &path) {
        return m_decoder->probe(path);
    }

    dsfw::Result<AudioBuffer> AudioPipeline::decodeAndResample(const std::string &path,
                                                                  const ResampleConfig &config) {
        // 1. Probe format
        auto infoResult = m_decoder->probe(path);
        if (!infoResult) {
            return dsfw::Result<AudioBuffer>::Error(infoResult.error());
        }
        auto &info = infoResult.value();

        // 2. Open file
        auto openResult = m_decoder->open(path);
        if (!openResult) {
            return dsfw::Result<AudioBuffer>::Error(openResult.error());
        }

        // 3. Stream decode + resample in chunks
        constexpr int64_t kChunkFrames = 4096;
        std::vector<AudioBuffer> chunks;

        while (true) {
            auto chunkResult = m_decoder->read(kChunkFrames);
            if (!chunkResult) {
                return dsfw::Result<AudioBuffer>::Error(chunkResult.error());
            }
            auto &chunk = chunkResult.value();
            if (chunk.empty()) break; // EOF

            auto resampledResult = m_resampler->convert(chunk, info.sampleRate, config);
            if (!resampledResult) {
                return dsfw::Result<AudioBuffer>::Error(resampledResult.error());
            }
            chunks.push_back(std::move(resampledResult.value()));
        }

        m_decoder->close();

        // 4. Merge all chunks
        if (chunks.empty()) {
            int outCh = config.targetChannelCount > 0 ? config.targetChannelCount : info.channelCount;
            auto outFmt = config.targetFormat != SampleFormat::Unknown ? config.targetFormat : info.sampleFormat;
            return AudioBuffer::create(0, outCh, outFmt);
        }
        return mergeBuffers(chunks);
    }

    dsfw::Result<AudioBuffer> AudioPipeline::decodeSegmentAndResample(const std::string &path, double startSec,
                                                                         double endSec, const ResampleConfig &config) {
        // 1. Probe format
        auto infoResult = m_decoder->probe(path);
        if (!infoResult) {
            return dsfw::Result<AudioBuffer>::Error(infoResult.error());
        }
        auto &info = infoResult.value();

        // 2. Open file
        auto openResult = m_decoder->open(path);
        if (!openResult) {
            return dsfw::Result<AudioBuffer>::Error(openResult.error());
        }

        // 3. Seek to start
        auto seekResult = m_decoder->seekToTime(startSec);
        if (!seekResult) {
            return dsfw::Result<AudioBuffer>::Error(seekResult.error());
        }

        // 4. Decode the segment
        double duration = endSec - startSec;
        int64_t targetFrames = static_cast<int64_t>(duration * info.sampleRate);

        constexpr int64_t kChunkFrames = 4096;
        int64_t remainingFrames = targetFrames;
        std::vector<AudioBuffer> chunks;

        while (remainingFrames > 0) {
            int64_t toRead = std::min(kChunkFrames, remainingFrames);
            auto chunkResult = m_decoder->read(toRead);
            if (!chunkResult) {
                return dsfw::Result<AudioBuffer>::Error(chunkResult.error());
            }
            auto &chunk = chunkResult.value();
            if (chunk.empty()) break;

            auto resampledResult = m_resampler->convert(chunk, info.sampleRate, config);
            if (!resampledResult) {
                return dsfw::Result<AudioBuffer>::Error(resampledResult.error());
            }
            chunks.push_back(std::move(resampledResult.value()));
            remainingFrames -= chunk.frameCount();
        }

        m_decoder->close();

        if (chunks.empty()) {
            int outCh = config.targetChannelCount > 0 ? config.targetChannelCount : info.channelCount;
            auto outFmt = config.targetFormat != SampleFormat::Unknown ? config.targetFormat : info.sampleFormat;
            return AudioBuffer::create(0, outCh, outFmt);
        }
        return mergeBuffers(chunks);
    }

    dsfw::Result<AudioBuffer> AudioPipeline::decodeToMonoFloat(const std::string &path, int targetSampleRate) {
        auto config = ResampleConfig::forMonoFloat(targetSampleRate);
        return decodeAndResample(path, config);
    }

} // namespace dsfw::audio