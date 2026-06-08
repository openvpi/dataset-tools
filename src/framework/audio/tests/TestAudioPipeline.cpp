/// @file TestAudioPipeline.cpp
/// @brief Unit tests for dsfw::audio::AudioPipeline.

#include <dsfw/audio/AudioPipeline.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>


// ── Test helpers ────────────────────────────────────────────────────────────

static int g_passed = 0;
static int g_failed = 0;

#define TEST_ASSERT(expr, msg)                                                      \
    do {                                                                            \
        if (!(expr)) {                                                              \
            std::cerr << "  FAIL: " << (msg) << "  (" << #expr << ")" << std::endl; \
            g_failed++;                                                             \
            return;                                                                 \
        }                                                                           \
    } while (0)

#define TEST_PASS(name)                                 \
    do {                                                \
        std::cout << "  PASS: " << (name) << std::endl; \
        g_passed++;                                     \
    } while (0)

// ── WAV helpers ──────────────────────────────────────────────────────────────

static void writeWav(const std::string& path, const std::vector<int16_t>& samples, int sampleRate) {
    std::ofstream f(path, std::ios::binary);
    uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataSize;

    f.write("RIFF", 4);
    f.write(reinterpret_cast<const char*>(&fileSize), 4);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    uint16_t blockAlign = numChannels * bitsPerSample / 8;
    f.write(reinterpret_cast<const char*>(&fmtSize), 4);
    f.write(reinterpret_cast<const char*>(&audioFormat), 2);
    f.write(reinterpret_cast<const char*>(&numChannels), 2);
    f.write(reinterpret_cast<const char*>(&sampleRate), 4);
    f.write(reinterpret_cast<const char*>(&byteRate), 4);
    f.write(reinterpret_cast<const char*>(&blockAlign), 2);
    f.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    f.write("data", 4);
    f.write(reinterpret_cast<const char*>(&dataSize), 4);
    f.write(reinterpret_cast<const char*>(samples.data()), dataSize);
}

static std::vector<int16_t> generateSine(int sampleRate, float freq, float durationSec, float amplitude = 0.5f) {
    int n = static_cast<int>(sampleRate * durationSec);
    std::vector<int16_t> buf(n);
    for (int i = 0; i < n; i++) {
        float val = amplitude * std::sin(2.0 * 3.14159265358979323846 * freq * i / sampleRate);
        buf[i] = static_cast<int16_t>(val * 32767);
    }
    return buf;
}

// ── Test cases ───────────────────────────────────────────────────────────────

static void test_pipeline_create() {
    auto pipeline = AudioPipeline::create();
    TEST_ASSERT(pipeline.decoder() != nullptr, "decoder not null");
    TEST_ASSERT(pipeline.resampler() != nullptr, "resampler not null");
    TEST_PASS("pipeline_create");
}

static void test_pipeline_probe() {
    auto samples = generateSine(44100, 440.0f, 1.0f);
    std::string path = "test_pipeline_probe.wav";
    writeWav(path, samples, 44100);

    auto pipeline = AudioPipeline::create();
    auto result = pipeline.probe(path);
    TEST_ASSERT(result.ok(), "probe ok");
    auto& info = result.value();
    TEST_ASSERT(info.sampleRate == 44100, "sampleRate");
    TEST_ASSERT(info.channelCount == 1, "channelCount");

    std::remove(path.c_str());
    TEST_PASS("pipeline_probe");
}

static void test_pipeline_decode_and_resample() {
    auto samples = generateSine(44100, 440.0f, 1.0f);
    std::string path = "test_pipeline_decode.wav";
    writeWav(path, samples, 44100);

    auto pipeline = AudioPipeline::create();
    auto result = pipeline.decodeAndResample(path, ResampleConfig::forMonoFloat(16000));
    TEST_ASSERT(result.ok(), "decodeAndResample ok");
    auto buf = result.value();
    TEST_ASSERT(!buf.empty(), "output not empty");
    TEST_ASSERT(buf.format() == SampleFormat::Float32, "output format");
    TEST_ASSERT(buf.channelCount() == 1, "output mono");
    // Should be ~16000 frames (1 second at 16000 Hz)
    TEST_ASSERT(std::abs(buf.frameCount() - 16000) < 100, "output frameCount");

    std::remove(path.c_str());
    TEST_PASS("pipeline_decode_and_resample");
}

static void test_pipeline_decode_segment_and_resample() {
    auto samples = generateSine(44100, 440.0f, 2.0f);
    std::string path = "test_pipeline_segment.wav";
    writeWav(path, samples, 44100);

    auto pipeline = AudioPipeline::create();
    auto result = pipeline.decodeSegmentAndResample(path, 0.5, 1.0, ResampleConfig::forMonoFloat(16000));
    TEST_ASSERT(result.ok(), "decodeSegmentAndResample ok");
    auto buf = result.value();
    TEST_ASSERT(!buf.empty(), "output not empty");
    TEST_ASSERT(buf.format() == SampleFormat::Float32, "output format");
    // Should be ~8000 frames (0.5 seconds at 16000 Hz)
    TEST_ASSERT(std::abs(buf.frameCount() - 8000) < 500, "segment frameCount");

    std::remove(path.c_str());
    TEST_PASS("pipeline_decode_segment_and_resample");
}

static void test_pipeline_decode_to_mono_float() {
    auto samples = generateSine(44100, 440.0f, 0.5f);
    std::string path = "test_pipeline_mono_float.wav";
    writeWav(path, samples, 44100);

    auto pipeline = AudioPipeline::create();
    auto result = pipeline.decodeToMonoFloat(path, 16000);
    TEST_ASSERT(result.ok(), "decodeToMonoFloat ok");
    auto buf = result.value();
    TEST_ASSERT(!buf.empty(), "output not empty");
    TEST_ASSERT(buf.format() == SampleFormat::Float32, "output Float32");
    TEST_ASSERT(buf.channelCount() == 1, "output mono");

    std::remove(path.c_str());
    TEST_PASS("pipeline_decode_to_mono_float");
}

static void test_pipeline_nonexistent_file() {
    auto pipeline = AudioPipeline::create();
    auto result = pipeline.decodeAndResample("nonexistent.wav", ResampleConfig::forMonoFloat(16000));
    TEST_ASSERT(!result.ok(), "nonexistent fails");
    TEST_PASS("pipeline_nonexistent_file");
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== TestAudioPipeline ===" << std::endl;

    test_pipeline_create();
    test_pipeline_probe();
    test_pipeline_decode_and_resample();
    test_pipeline_decode_segment_and_resample();
    test_pipeline_decode_to_mono_float();
    test_pipeline_nonexistent_file();

    std::cout << "\nResults: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    return g_failed > 0 ? 1 : 0;
}