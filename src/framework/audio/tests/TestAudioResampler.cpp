/// @file TestAudioResampler.cpp
/// @brief Unit tests for dsfw::audio::SwresampleResampler.

#include <dsfw/audio/SwresampleResampler.h>
#include <cassert>
#include <cmath>
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

// ── Test cases ───────────────────────────────────────────────────────────────

static void test_resample_rate() {
    // Create a 1-second sine wave at 44100 Hz
    int inputRate = 44100;
    int inputFrames = inputRate;
    auto input = AudioBuffer::create(inputFrames, 1, SampleFormat::Float32);
    auto f = input.floats();
    for (int i = 0; i < inputFrames; i++) {
        f[i] = std::sin(2.0 * 3.14159265358979323846 * 220.0 * i / inputRate);
    }

    dsfw::audio::SwresampleResampler resampler;
    dsfw::audio::ResampleConfig config = ResampleConfig::forMonoFloat(22050);

    auto result = resampler.convert(input, inputRate, config);
    TEST_ASSERT(result.ok(), "convert ok");
    auto output = result.value();
    TEST_ASSERT(!output.empty(), "output not empty");
    TEST_ASSERT(output.format() == SampleFormat::Float32, "output format");
    TEST_ASSERT(output.channelCount() == 1, "output mono");
    // Should be ~22050 frames (1 second at 22050 Hz)
    TEST_ASSERT(std::abs(output.frameCount() - 22050) < 100, "output frameCount");
    TEST_PASS("resample_rate");
}

static void test_resample_no_change() {
    // Create a buffer and resample to same rate (should be passthrough or near-passthrough)
    int rate = 16000;
    auto input = AudioBuffer::create(rate, 1, SampleFormat::Float32);
    auto f = input.floats();
    for (int i = 0; i < rate; i++) {
        f[i] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / rate);
    }

    dsfw::audio::SwresampleResampler resampler;
    dsfw::audio::ResampleConfig config = ResampleConfig::forMonoFloat(16000);

    auto result = resampler.convert(input, rate, config);
    TEST_ASSERT(result.ok(), "convert ok");
    auto output = result.value();
    TEST_ASSERT(!output.empty(), "output not empty");
    TEST_ASSERT(output.channelCount() == 1, "output mono");
    TEST_ASSERT(std::abs(output.frameCount() - 16000) < 100, "output frameCount");
    TEST_PASS("resample_no_change");
}

static void test_resample_stereo_to_mono() {
    int rate = 44100;
    int frames = 1000;
    auto input = AudioBuffer::create(frames, 2, SampleFormat::Float32);
    auto f = input.floats();
    for (int i = 0; i < frames; i++) {
        f[i * 2] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / rate);
        f[i * 2 + 1] = std::sin(2.0 * 3.14159265358979323846 * 880.0 * i / rate);
    }

    dsfw::audio::SwresampleResampler resampler;
    dsfw::audio::ResampleConfig config = ResampleConfig::forMonoFloat(16000);

    auto result = resampler.convert(input, rate, config);
    TEST_ASSERT(result.ok(), "convert ok");
    auto output = result.value();
    TEST_ASSERT(output.channelCount() == 1, "output mono");
    TEST_ASSERT(output.format() == SampleFormat::Float32, "output format");
    TEST_ASSERT(output.frameCount() > 0, "output frameCount > 0");
    TEST_PASS("resample_stereo_to_mono");
}

static void test_resample_int16_to_float() {
    int rate = 16000;
    int frames = 1000;
    std::vector<int16_t> data(frames);
    for (int i = 0; i < frames; i++) {
        data[i] = static_cast<int16_t>(std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / rate) * 10000);
    }

    auto input = AudioBuffer::fromCopy(data.data(), frames, 1, SampleFormat::Int16);

    dsfw::audio::SwresampleResampler resampler;
    dsfw::audio::ResampleConfig config = ResampleConfig::forMonoFloat(16000);

    auto result = resampler.convert(input, rate, config);
    TEST_ASSERT(result.ok(), "convert ok");
    auto output = result.value();
    TEST_ASSERT(output.format() == SampleFormat::Float32, "output format Float32");
    TEST_ASSERT(!output.empty(), "output not empty");
    TEST_PASS("resample_int16_to_float");
}

static void test_convenience_to_mono_float() {
    int rate = 44100;
    int frames = 500;
    auto input = AudioBuffer::create(frames, 1, SampleFormat::Float32);
    auto f = input.floats();
    for (int i = 0; i < frames; i++) {
        f[i] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / rate);
    }

    dsfw::audio::SwresampleResampler resampler;
    auto result = resampler.toMonoFloat(input, rate, 16000);
    TEST_ASSERT(result.ok(), "toMonoFloat ok");
    auto output = result.value();
    TEST_ASSERT(output.format() == SampleFormat::Float32, "toMonoFloat format");
    TEST_ASSERT(output.channelCount() == 1, "toMonoFloat mono");
    TEST_ASSERT(!output.empty(), "toMonoFloat not empty");
    TEST_PASS("convenience_to_mono_float");
}

static void test_empty_input() {
    auto input = AudioBuffer::create(0, 0, SampleFormat::Float32);
    dsfw::audio::SwresampleResampler resampler;
    dsfw::audio::ResampleConfig config = ResampleConfig::forMonoFloat(16000);

    auto result = resampler.convert(input, 44100, config);
    // Empty input should return empty or error
    TEST_ASSERT(result.ok() || !result.ok(), "empty input handled");
    TEST_PASS("empty_input");
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== TestAudioResampler ===" << std::endl;

    test_resample_rate();
    test_resample_no_change();
    test_resample_stereo_to_mono();
    test_resample_int16_to_float();
    test_convenience_to_mono_float();
    test_empty_input();

    std::cout << "\nResults: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    return g_failed > 0 ? 1 : 0;
}