/// @file TestAudioBuffer.cpp
/// @brief Unit tests for dsfw::audio::AudioBuffer.

#include <dsfw/audio/AudioBuffer.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

using namespace dsfw::audio;

// ── Test helpers ────────────────────────────────────────────────────────────

static int g_passed = 0;
static int g_failed = 0;

#define TEST_ASSERT(expr, msg)                                                                 \
    do {                                                                                       \
        if (!(expr)) {                                                                         \
            std::cerr << "  FAIL: " << (msg) << "  (" << #expr << ")" << std::endl;            \
            g_failed++;                                                                        \
            return;                                                                            \
        }                                                                                      \
    } while (0)

#define TEST_PASS(name)                                                                        \
    do {                                                                                       \
        std::cout << "  PASS: " << (name) << std::endl;                                        \
        g_passed++;                                                                            \
    } while (0)

// ── Test cases ───────────────────────────────────────────────────────────────

static void test_create_zero() {
    auto buf = AudioBuffer::create(100, 2, SampleFormat::Float32);
    TEST_ASSERT(buf.frameCount() == 100, "frameCount");
    TEST_ASSERT(buf.channelCount() == 2, "channelCount");
    TEST_ASSERT(buf.format() == SampleFormat::Float32, "format");
    TEST_ASSERT(buf.sampleCount() == 200, "sampleCount");
    TEST_ASSERT(buf.byteSize() == 100 * 2 * 4, "byteSize");
    TEST_ASSERT(!buf.isView(), "isView");
    TEST_ASSERT(!buf.empty(), "empty");
    TEST_PASS("create_zero");
}

static void test_create_empty() {
    auto buf = AudioBuffer::create(0, 0, SampleFormat::Float32);
    TEST_ASSERT(buf.empty(), "empty");
    TEST_ASSERT(buf.frameCount() == 0, "frameCount");
    TEST_ASSERT(buf.byteSize() == 0, "byteSize");
    TEST_PASS("create_empty");
}

static void test_from_copy() {
    std::vector<float> data = {1.0f, 0.5f, -0.5f, -1.0f, 0.0f, 0.3f, -0.3f, 0.0f};
    auto buf = AudioBuffer::fromCopy(data.data(), 4, 2, SampleFormat::Float32);
    TEST_ASSERT(buf.frameCount() == 4, "frameCount");
    TEST_ASSERT(!buf.isView(), "isView");
    TEST_ASSERT(!buf.empty(), "empty");

    auto floats = buf.floats();
    TEST_ASSERT(floats.size() == 8, "floats.size");
    TEST_ASSERT(std::abs(floats[0] - 1.0f) < 0.001f, "floats[0]");
    TEST_ASSERT(std::abs(floats[3] - (-1.0f)) < 0.001f, "floats[3]");
    TEST_PASS("from_copy");
}

static void test_from_copy_nullptr() {
    auto buf = AudioBuffer::fromCopy(nullptr, 0, 0, SampleFormat::Float32);
    TEST_ASSERT(buf.empty(), "nullptr empty");
    TEST_PASS("from_copy_nullptr");
}

static void test_from_view() {
    float data[] = {0.1f, 0.2f, 0.3f, 0.4f};
    auto buf = AudioBuffer::fromView(data, 2, 2, SampleFormat::Float32);
    TEST_ASSERT(buf.frameCount() == 2, "frameCount");
    TEST_ASSERT(buf.isView(), "isView");
    TEST_ASSERT(!buf.empty(), "empty");

    auto floats = buf.floats();
    TEST_ASSERT(std::abs(floats[0] - 0.1f) < 0.001f, "floats[0]");
    TEST_PASS("from_view");
}

static void test_from_vector() {
    std::vector<uint8_t> v = {0, 1, 2, 3, 4, 5, 6, 7};
    auto buf = AudioBuffer::fromVector(std::move(v), 2, 1, SampleFormat::Float32);
    TEST_ASSERT(buf.frameCount() == 2, "frameCount");
    TEST_ASSERT(buf.byteSize() == 8, "byteSize");
    TEST_ASSERT(!buf.isView(), "isView");
    TEST_PASS("from_vector");
}

static void test_duration_sec() {
    auto buf = AudioBuffer::create(44100, 1, SampleFormat::Float32);
    double dur = buf.durationSec(44100);
    TEST_ASSERT(std::abs(dur - 1.0) < 0.001, "duration 1 sec");
    TEST_ASSERT(std::abs(buf.durationSec(0) - 0.0) < 0.001, "duration zero rate");
    TEST_PASS("duration_sec");
}

static void test_zero() {
    auto buf = AudioBuffer::create(10, 1, SampleFormat::Float32);
    auto f = buf.floats();
    f[0] = 1.0f;
    buf.zero();
    for (size_t i = 0; i < f.size(); i++) {
        TEST_ASSERT(f[i] == 0.0f, "zeroed");
    }
    TEST_PASS("zero");
}

static void test_clone() {
    auto buf = AudioBuffer::create(5, 1, SampleFormat::Float32);
    buf.floats()[0] = 0.5f;
    auto copy = buf.clone();
    TEST_ASSERT(copy.frameCount() == 5, "clone frameCount");
    TEST_ASSERT(!copy.isView(), "clone isView");
    TEST_ASSERT(std::abs(copy.floats()[0] - 0.5f) < 0.001f, "clone data");
    // Modify original, clone should be independent
    buf.floats()[0] = 1.0f;
    TEST_ASSERT(std::abs(copy.floats()[0] - 0.5f) < 0.001f, "clone independent");
    TEST_PASS("clone");
}

static void test_slice() {
    auto buf = AudioBuffer::create(10, 1, SampleFormat::Float32);
    for (int i = 0; i < 10; i++) buf.floats()[i] = static_cast<float>(i);
    auto sliced = buf.slice(3, 4);
    TEST_ASSERT(sliced.frameCount() == 4, "slice frameCount");
    TEST_ASSERT(std::abs(sliced.floats()[0] - 3.0f) < 0.001f, "slice[0]");
    TEST_ASSERT(std::abs(sliced.floats()[3] - 6.0f) < 0.001f, "slice[3]");
    TEST_PASS("slice");
}

static void test_slice_out_of_bounds() {
    auto buf = AudioBuffer::create(10, 1, SampleFormat::Float32);
    for (int i = 0; i < 10; i++) buf.floats()[i] = static_cast<float>(i);
    auto sliced = buf.slice(8, 5);
    TEST_ASSERT(sliced.frameCount() == 2, "slice oob frameCount");
    TEST_PASS("slice_out_of_bounds");
}

static void test_sample_at() {
    auto buf = AudioBuffer::create(3, 2, SampleFormat::Float32);
    auto f = buf.floats();
    f[0] = 0.1f; f[1] = 0.2f;   // frame 0
    f[2] = 0.3f; f[3] = 0.4f;   // frame 1
    f[4] = 0.5f; f[5] = 0.6f;   // frame 2
    TEST_ASSERT(std::abs(buf.sampleAt(0, 0) - 0.1f) < 0.001f, "sampleAt(0,0)");
    TEST_ASSERT(std::abs(buf.sampleAt(0, 1) - 0.2f) < 0.001f, "sampleAt(0,1)");
    TEST_ASSERT(std::abs(buf.sampleAt(2, 1) - 0.6f) < 0.001f, "sampleAt(2,1)");
    TEST_PASS("sample_at");
}

static void test_int16_format() {
    std::vector<int16_t> data = {0, 100, 200, -100};
    auto buf = AudioBuffer::fromCopy(data.data(), 2, 2, SampleFormat::Int16);
    TEST_ASSERT(buf.format() == SampleFormat::Int16, "int16 format");
    TEST_ASSERT(buf.byteSize() == 2 * 2 * 2, "int16 byteSize");
    auto i16 = buf.int16s();
    TEST_ASSERT(i16[0] == 0, "int16[0]");
    TEST_ASSERT(i16[1] == 100, "int16[1]");
    TEST_ASSERT(i16[2] == 200, "int16[2]");
    TEST_ASSERT(i16[3] == -100, "int16[3]");
    TEST_PASS("int16_format");
}

static void test_bytes_per_sample() {
    TEST_ASSERT(bytesPerSample(SampleFormat::Float32) == 4, "Float32");
    TEST_ASSERT(bytesPerSample(SampleFormat::Int16) == 2, "Int16");
    TEST_ASSERT(bytesPerSample(SampleFormat::Int32) == 4, "Int32");
    TEST_ASSERT(bytesPerSample(SampleFormat::Unknown) == 0, "Unknown");
    TEST_PASS("bytes_per_sample");
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== TestAudioBuffer ===" << std::endl;

    test_create_zero();
    test_create_empty();
    test_from_copy();
    test_from_copy_nullptr();
    test_from_view();
    test_from_vector();
    test_duration_sec();
    test_zero();
    test_clone();
    test_slice();
    test_slice_out_of_bounds();
    test_sample_at();
    test_int16_format();
    test_bytes_per_sample();

    std::cout << "\nResults: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    return g_failed > 0 ? 1 : 0;
}