#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int g_passed = 0;
static int g_failed = 0;

#define TEST_ASSERT(expr, msg)                                                                                         \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            std::cerr << "  FAIL: " << (msg) << "  (" << #expr << ")" << std::endl;                                   \
            g_failed++;                                                                                                \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define TEST_PASS(name)                                                                                                \
    do {                                                                                                               \
        std::cout << "  PASS: " << (name) << std::endl;                                                               \
        g_passed++;                                                                                                    \
    } while (0)

// Generate a sine wave buffer
static std::vector<float> generate_sine(int sampleRate, float freq, float durationSec, float amplitude = 0.5f) {
    const size_t n = static_cast<size_t>(sampleRate * durationSec);
    std::vector<float> buf(n);
    for (size_t i = 0; i < n; i++) {
        buf[i] = amplitude * static_cast<float>(std::sin(2.0 * M_PI * freq * i / sampleRate));
    }
    return buf;
}

// ---------------------------------------------------------------------------
// TC-AU-001: Basic resample + WAV write
// ---------------------------------------------------------------------------
static void test_resample_wav(const std::filesystem::path &audioIn, const std::filesystem::path &wavOut) {
    std::cout << "[TC-AU-001] Basic resample + WAV write" << std::endl;

    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(audioIn, errorMsg, 1, 44100);

    TEST_ASSERT(errorMsg.empty(), "resample_to_vio returned error: " + errorMsg);
    TEST_ASSERT(sf_vio.size() > 0, "VIO data is empty");

    SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                     sf_vio.info.samplerate);
    const auto totalSize = sf.frames();
    TEST_ASSERT(totalSize > 0, "Resampled audio has 0 frames");
    TEST_ASSERT(sf_vio.info.samplerate == 44100, "Output sample rate != 44100");
    TEST_ASSERT(sf_vio.info.channels == 1, "Output channels != 1 (mono)");

    std::vector<float> audio(totalSize);
    sf.seek(0, SEEK_SET);
    sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

    bool ok = AudioUtil::write_vio_to_wav(sf_vio, wavOut, 1);
    TEST_ASSERT(ok, "write_vio_to_wav returned false");
    TEST_ASSERT(std::filesystem::exists(wavOut), "Output WAV file does not exist");
    TEST_ASSERT(std::filesystem::file_size(wavOut) > 0, "Output WAV file is empty");

    TEST_PASS("TC-AU-001");
}

// ---------------------------------------------------------------------------
// TC-AU-002: Error handling - nonexistent input file
// ---------------------------------------------------------------------------
static void test_nonexistent_input() {
    std::cout << "[TC-AU-002] Error handling - nonexistent input" << std::endl;

    const std::filesystem::path bad_path = "/nonexistent/path/does_not_exist.wav";

    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(bad_path, errorMsg, 1, 44100);

    // Should either have an error message or produce empty output - must not crash
    const bool handled = !errorMsg.empty() || sf_vio.size() == 0;
    TEST_ASSERT(handled, "Nonexistent file was not handled (no error and non-empty VIO)");

    TEST_PASS("TC-AU-002");
}

// ---------------------------------------------------------------------------
// TC-AU-003: Error handling - invalid audio file (garbage data)
// ---------------------------------------------------------------------------
static void test_invalid_audio(const std::filesystem::path &tmpDir) {
    std::cout << "[TC-AU-003] Error handling - invalid audio file" << std::endl;

    const auto fakePath = tmpDir / "fake_audio.wav";
    {
        std::ofstream f(fakePath);
        f << "this is not a valid audio file";
    }

    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(fakePath, errorMsg, 1, 44100);

    const bool handled = !errorMsg.empty() || sf_vio.size() == 0;
    TEST_ASSERT(handled, "Invalid audio file was not handled");

    // Cleanup
    std::filesystem::remove(fakePath);

    TEST_PASS("TC-AU-003");
}

// ---------------------------------------------------------------------------
// TC-AU-004: Slicer basic functionality
//   Slicer(sampleRate, threshold, hopSize, winSize, minLength, minInterval, maxSilKept)
//   slice(samples) -> MarkerList (vector<pair<int64_t, int64_t>>)
// ---------------------------------------------------------------------------
static void test_slicer_basic() {
    std::cout << "[TC-AU-004] Slicer basic functionality" << std::endl;

    const int sr = 44100;

    // Construct audio: silence [0,1s], voice [1s,3s], silence [3s,4s], voice [4s,5s]
    std::vector<float> audio(sr * 5, 0.0f);
    for (int i = sr * 1; i < sr * 3; i++) {
        audio[i] = 0.5f * static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / sr));
    }
    for (int i = sr * 4; i < sr * 5; i++) {
        audio[i] = 0.5f * static_cast<float>(std::sin(2.0 * M_PI * 440.0 * i / sr));
    }

    // Parameters: sampleRate, threshold(dB), hopSize, winSize, minLength, minInterval, maxSilKept
    AudioUtil::Slicer slicer(sr, -40.0f, 400, 800, 5000, 300, 500);
    auto markers = slicer.slice(audio);

    std::cout << "    Slicer returned " << markers.size() << " segment(s)" << std::endl;
    for (size_t i = 0; i < markers.size(); i++) {
        std::cout << "    Segment " << i << ": [" << markers[i].first << ", " << markers[i].second << "] samples"
                  << std::endl;
    }

    // Should detect at least 2 voiced segments
    TEST_ASSERT(markers.size() >= 2, "Expected >= 2 segments from slicer, got " + std::to_string(markers.size()));

    // All segments should have positive length
    for (const auto &[start, end] : markers) {
        TEST_ASSERT(end > start, "Segment has non-positive length");
    }

    TEST_PASS("TC-AU-004");
}

// ---------------------------------------------------------------------------
// TC-AU-005: Slicer edge cases
// ---------------------------------------------------------------------------
static void test_slicer_edge_cases() {
    std::cout << "[TC-AU-005] Slicer edge cases" << std::endl;

    const int sr = 44100;

    // 5a: Empty audio
    {
        AudioUtil::Slicer slicer(sr, -40.0f, 400, 800, 5000, 300, 500);
        std::vector<float> empty;
        auto markers = slicer.slice(empty);
        TEST_ASSERT(markers.empty(), "Empty audio should produce no segments");
        std::cout << "    5a (empty audio): OK" << std::endl;
    }

    // 5b: Pure silence
    {
        AudioUtil::Slicer slicer(sr, -40.0f, 400, 800, 5000, 300, 500);
        std::vector<float> silence(sr * 3, 0.0f);
        auto markers = slicer.slice(silence);
        // Pure silence should yield no voiced segments or a single segment covering everything
        std::cout << "    5b (pure silence): " << markers.size() << " segment(s) - OK" << std::endl;
    }

    // 5c: Continuous voice (no silence)
    {
        AudioUtil::Slicer slicer(sr, -40.0f, 400, 800, 5000, 300, 500);
        auto voice = generate_sine(sr, 440.0f, 3.0f, 0.5f);
        auto markers = slicer.slice(voice);
        TEST_ASSERT(markers.size() >= 1, "Continuous voice should produce >= 1 segment");
        std::cout << "    5c (continuous voice): " << markers.size() << " segment(s) - OK" << std::endl;
    }

    TEST_PASS("TC-AU-005");
}

// ---------------------------------------------------------------------------
// TC-AU-006: Resample accuracy (frame count validation)
// ---------------------------------------------------------------------------
static void test_resample_accuracy(const std::filesystem::path &audioIn) {
    std::cout << "[TC-AU-006] Resample accuracy" << std::endl;

    // Resample to 44100 mono and check frame count is reasonable
    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(audioIn, errorMsg, 1, 44100);
    TEST_ASSERT(errorMsg.empty(), "resample_to_vio error: " + errorMsg);

    SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                     sf_vio.info.samplerate);
    const auto frames = sf.frames();
    TEST_ASSERT(frames > 0, "Resampled output has 0 frames");

    // Also resample the same file to 16000 to test a different target rate
    std::string errorMsg2;
    auto sf_vio2 = AudioUtil::resample_to_vio(audioIn, errorMsg2, 1, 16000);
    TEST_ASSERT(errorMsg2.empty(), "resample_to_vio (16000) error: " + errorMsg2);

    SndfileHandle sf2(sf_vio2.vio, &sf_vio2.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio2.info.channels,
                      sf_vio2.info.samplerate);
    const auto frames2 = sf2.frames();
    TEST_ASSERT(frames2 > 0, "Resampled output (16000) has 0 frames");

    // Duration should be approximately equal: frames/44100 ≈ frames2/16000
    const double dur1 = static_cast<double>(frames) / 44100.0;
    const double dur2 = static_cast<double>(frames2) / 16000.0;
    const double diff = std::abs(dur1 - dur2);
    TEST_ASSERT(diff < 0.01, "Duration mismatch between resampled outputs: " + std::to_string(diff) + "s");

    std::cout << "    44100Hz: " << frames << " frames (" << dur1 << "s)" << std::endl;
    std::cout << "    16000Hz: " << frames2 << " frames (" << dur2 << "s)" << std::endl;

    TEST_PASS("TC-AU-006");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <audio_in_path> <wav_out_path> [--all]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "  Without --all: runs only TC-AU-001 (basic resample, compatible with original usage)" << std::endl;
        std::cerr << "  With --all:    runs all test cases (TC-AU-001 through TC-AU-006)" << std::endl;
        return 1;
    }

    const std::filesystem::path audio_in_path = argv[1];
    const std::filesystem::path wav_out_path = argv[2];
    const bool runAll = (argc >= 4 && std::string(argv[3]) == "--all");

    if (!std::filesystem::exists(audio_in_path)) {
        std::cerr << "Error: Input file does not exist: " << audio_in_path << std::endl;
        return 1;
    }

    const auto tmpDir = wav_out_path.parent_path().empty() ? std::filesystem::temp_directory_path()
                                                           : wav_out_path.parent_path();

    std::cout << "========================================" << std::endl;
    std::cout << " TestAudioUtil" << std::endl;
    std::cout << "========================================" << std::endl;

    // TC-AU-001: Always run (backward compatible)
    test_resample_wav(audio_in_path, wav_out_path);

    if (runAll) {
        // TC-AU-002: Nonexistent input
        test_nonexistent_input();

        // TC-AU-003: Invalid audio file
        test_invalid_audio(tmpDir);

        // TC-AU-004: Slicer basic
        test_slicer_basic();

        // TC-AU-005: Slicer edge cases
        test_slicer_edge_cases();

        // TC-AU-006: Resample accuracy
        test_resample_accuracy(audio_in_path);
    }

    std::cout << "========================================" << std::endl;
    std::cout << " Results: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
