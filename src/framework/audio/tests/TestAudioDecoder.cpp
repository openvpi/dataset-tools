/// @file TestAudioDecoder.cpp
/// @brief Unit tests for dsfw::audio::FfmpegAudioDecoder.

#include <dsfw/audio/FfmpegAudioDecoder.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
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

// ── WAV file helpers ────────────────────────────────────────────────────────

/// Write a minimal PCM 16-bit mono WAV file.
static void writeWav(const std::string& path, const std::vector<int16_t>& samples, int sampleRate) {
    std::ofstream f(path, std::ios::binary);
    uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataSize;

    // RIFF header
    f.write("RIFF", 4);
    f.write(reinterpret_cast<const char*>(&fileSize), 4);
    f.write("WAVE", 4);

    // fmt chunk
    f.write("fmt ", 4);
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
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

    // data chunk
    f.write("data", 4);
    f.write(reinterpret_cast<const char*>(&dataSize), 4);
    f.write(reinterpret_cast<const char*>(samples.data()), dataSize);
}

/// Generate a sine wave as int16 samples.
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

static void test_probe_wav() {
    // Generate a 1-second 440 Hz sine wave at 44100 Hz
    auto samples = generateSine(44100, 440.0f, 1.0f);
    std::string path = "test_sine_44100.wav";
    writeWav(path, samples, 44100);

    dsfw::audio::FfmpegAudioDecoder decoder;
    auto result = decoder.probe(path);
    TEST_ASSERT(result.ok(), "probe ok");
    auto& info = result.value();
    TEST_ASSERT(info.sampleRate == 44100, "sampleRate");
    TEST_ASSERT(info.channelCount == 1, "channelCount");
    TEST_ASSERT(info.durationSec > 0.9 && info.durationSec < 1.1, "durationSec");

    std::remove(path.c_str());
    TEST_PASS("probe_wav");
}

static void test_probe_nonexistent() {
    dsfw::audio::FfmpegAudioDecoder decoder;
    auto result = decoder.probe("nonexistent_file.wav");
    TEST_ASSERT(!result.ok(), "probe nonexistent fails");
    TEST_PASS("probe_nonexistent");
}

static void test_decode_all() {
    auto samples = generateSine(22050, 220.0f, 0.5f);
    std::string path = "test_decode_22050.wav";
    writeWav(path, samples, 22050);

    dsfw::audio::FfmpegAudioDecoder decoder;
    auto openResult = decoder.open(path);
    TEST_ASSERT(openResult.ok(), "open ok");
    TEST_ASSERT(decoder.isOpen(), "isOpen");

    auto& info = decoder.formatInfo();
    TEST_ASSERT(info.sampleRate == 22050, "formatInfo sampleRate");

    auto result = decoder.decodeAll();
    TEST_ASSERT(result.ok(), "decodeAll ok");
    auto buf = result.value();
    TEST_ASSERT(!buf.empty(), "decodeAll not empty");
    TEST_ASSERT(buf.frameCount() > 0, "decodeAll frameCount");

    // Should be decoded as float32
    TEST_ASSERT(buf.format() == SampleFormat::Float32, "decodeAll format");

    decoder.close();
    TEST_ASSERT(!decoder.isOpen(), "isOpen after close");

    std::remove(path.c_str());
    TEST_PASS("decode_all");
}

static void test_decode_segment() {
    auto samples = generateSine(44100, 440.0f, 2.0f);
    std::string path = "test_segment_44100.wav";
    writeWav(path, samples, 44100);

    dsfw::audio::FfmpegAudioDecoder decoder;
    auto openResult = decoder.open(path);
    TEST_ASSERT(openResult.ok(), "open ok");

    auto result = decoder.decodeSegment(0.5, 1.0);
    TEST_ASSERT(result.ok(), "decodeSegment ok");
    auto buf = result.value();
    TEST_ASSERT(!buf.empty(), "decodeSegment not empty");
    // Should be ~0.5 seconds worth of frames
    double expectedFrames = 0.5 * 44100;
    TEST_ASSERT(std::abs(buf.frameCount() - expectedFrames) < expectedFrames * 0.1, "decodeSegment frameCount");

    decoder.close();
    std::remove(path.c_str());
    TEST_PASS("decode_segment");
}

static void test_streaming_read() {
    auto samples = generateSine(16000, 440.0f, 1.0f);
    std::string path = "test_stream_16000.wav";
    writeWav(path, samples, 16000);

    dsfw::audio::FfmpegAudioDecoder decoder;
    TEST_ASSERT(decoder.open(path).ok(), "open ok");

    int64_t totalFrames = 0;
    int chunkCount = 0;
    while (true) {
        auto result = decoder.read(1024);
        TEST_ASSERT(result.ok(), "read ok");
        auto chunk = result.value();
        if (chunk.empty())
            break;
        totalFrames += chunk.frameCount();
        chunkCount++;
    }
    TEST_ASSERT(totalFrames > 0, "totalFrames > 0");
    TEST_ASSERT(chunkCount > 0, "chunkCount > 0");

    decoder.close();
    std::remove(path.c_str());
    TEST_PASS("streaming_read");
}

static void test_seek() {
    auto samples = generateSine(44100, 440.0f, 2.0f);
    std::string path = "test_seek_44100.wav";
    writeWav(path, samples, 44100);

    dsfw::audio::FfmpegAudioDecoder decoder;
    TEST_ASSERT(decoder.open(path).ok(), "open ok");

    double totalDur = decoder.totalDuration();
    TEST_ASSERT(totalDur > 1.5, "totalDuration");

    // Seek to 1.0 sec
    auto seekResult = decoder.seekToTime(1.0);
    TEST_ASSERT(seekResult.ok(), "seekToTime ok");

    auto currentTime = decoder.currentTime();
    TEST_ASSERT(currentTime > 0.5, "currentTime after seek");

    // Read from seek position
    auto result = decoder.read(1024);
    TEST_ASSERT(result.ok(), "read after seek");
    auto chunk = result.value();
    TEST_ASSERT(!chunk.empty(), "read after seek not empty");

    decoder.close();
    std::remove(path.c_str());
    TEST_PASS("seek");
}

static void test_double_open() {
    auto samples = generateSine(44100, 440.0f, 0.5f);
    std::string path1 = "test_double_1.wav";
    std::string path2 = "test_double_2.wav";
    writeWav(path1, samples, 44100);
    writeWav(path2, samples, 44100);

    dsfw::audio::FfmpegAudioDecoder decoder;
    TEST_ASSERT(decoder.open(path1).ok(), "open first");
    TEST_ASSERT(decoder.isOpen(), "isOpen first");
    TEST_ASSERT(decoder.open(path2).ok(), "open second");
    TEST_ASSERT(decoder.isOpen(), "isOpen second");

    decoder.close();
    std::remove(path1.c_str());
    std::remove(path2.c_str());
    TEST_PASS("double_open");
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== TestAudioDecoder ===" << std::endl;

    test_probe_wav();
    test_probe_nonexistent();
    test_decode_all();
    test_decode_segment();
    test_streaming_read();
    test_seek();
    test_double_open();

    std::cout << "\nResults: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    return g_failed > 0 ? 1 : 0;
}