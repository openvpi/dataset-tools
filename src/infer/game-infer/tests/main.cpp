#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <game-infer/Game.h>
#include <game-infer/GameModel.h>

#include "wolf-midi/MidiFile.h"

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

static void progressChanged(const int progress) { std::cout << "progress: " << progress << "%" << std::endl; }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<std::string> parseCommaSeparated(const std::string &val) {
    std::vector<std::string> result;
    size_t pos = 0;
    std::string temp = val;
    while ((pos = temp.find(',')) != std::string::npos) {
        result.push_back(temp.substr(0, pos));
        temp.erase(0, pos + 1);
    }
    result.push_back(temp);
    return result;
}

static std::vector<float> t0_n_step_to_ts(const float t0, const int n_steps) {
    if (n_steps <= 0)
        return {};
    const float step = (1.0f - t0) / n_steps;
    std::vector<float> result;
    for (int i = 0; i < n_steps; ++i) {
        result.push_back(t0 + i * step);
    }
    return result;
}

static bool saveMidi(const std::vector<Game::GameMidi> &midis, const std::filesystem::path &outPath, float tempo) {
    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();
    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    std::vector<char> trackName;
    std::string str = "Game";
    trackName.insert(trackName.end(), str.begin(), str.end());

    midi.createTrack();
    midi.createMetaEvent(1, 0, Midi::MidiEvent::MetaNumbers::TrackName, trackName);

    for (const auto &[note, start, duration] : midis) {
        midi.createNoteOnEvent(1, start, 0, note, 64);
        midi.createNoteOffEvent(1, start + duration, 0, note, 64);
    }

    midi.save(outPath);
    return std::filesystem::exists(outPath);
}

// ---------------------------------------------------------------------------
// TC-GI-001: End-to-end audio to MIDI
// ---------------------------------------------------------------------------
static void test_e2e(Game::Game &game, const std::filesystem::path &audioPath,
                     const std::filesystem::path &outputDir, float tempo) {
    std::cout << "[TC-GI-001] End-to-end audio to MIDI" << std::endl;

    std::vector<Game::GameMidi> midis;
    std::string msg;

    const bool success = game.get_midi(audioPath, midis, tempo, msg, progressChanged, 60);
    TEST_ASSERT(success, "get_midi failed: " + msg);

    std::cout << "    Notes: " << midis.size() << std::endl;

    // TC-GI-005 (partial): Validate MIDI note values
    for (const auto &[note, start, duration] : midis) {
        TEST_ASSERT(note >= 0 && note <= 127, "MIDI note out of range: " + std::to_string(note));
        TEST_ASSERT(duration > 0, "MIDI note has non-positive duration");
    }

    // Save MIDI
    const auto midiPath = outputDir / (audioPath.stem().string() + ".mid");
    const bool saved = saveMidi(midis, midiPath, tempo);
    TEST_ASSERT(saved, "Failed to save MIDI file");
    TEST_ASSERT(std::filesystem::file_size(midiPath) > 0, "MIDI file is empty");

    std::cout << "    Saved MIDI to: " << midiPath << std::endl;

    TEST_PASS("TC-GI-001");
}

// ---------------------------------------------------------------------------
// TC-GI-006: D3PM custom time steps
// ---------------------------------------------------------------------------
static void test_d3pm_ts() {
    std::cout << "[TC-GI-006] D3PM time step generation" << std::endl;

    // Test t0_n_step_to_ts
    {
        auto ts = t0_n_step_to_ts(0.0f, 8);
        TEST_ASSERT(ts.size() == 8, "Expected 8 time steps, got " + std::to_string(ts.size()));
        TEST_ASSERT(ts[0] == 0.0f, "First time step should be 0.0");
        // Last step should be < 1.0
        TEST_ASSERT(ts.back() < 1.0f, "Last time step should be < 1.0");
    }

    // Test zero steps
    {
        auto ts = t0_n_step_to_ts(0.0f, 0);
        TEST_ASSERT(ts.empty(), "Zero steps should produce empty ts");
    }

    // Test negative steps
    {
        auto ts = t0_n_step_to_ts(0.0f, -1);
        TEST_ASSERT(ts.empty(), "Negative steps should produce empty ts");
    }

    // Test parseCommaSeparated
    {
        auto parts = parseCommaSeparated("0.0,0.125,0.25,0.375");
        TEST_ASSERT(parts.size() == 4, "Expected 4 parts from comma-separated string");
        TEST_ASSERT(parts[0] == "0.0", "First part should be 0.0");
    }

    TEST_PASS("TC-GI-006");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <audio_path> <model_dir> <output_dir> [options...]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  --language <lang_code> (default: none)" << std::endl;
        std::cerr << "  --batch-size <int> (default: 1) [not parsed]" << std::endl;
        std::cerr << "  --num-workers <int> (default: 0) [not parsed]" << std::endl;
        std::cerr << "  --seg-threshold <float> (default: 0.2)" << std::endl;
        std::cerr << "  --seg-radius <float> (default: 0.02)" << std::endl;
        std::cerr << "  --seg-d3pm-t0 <float> (default: 0.0)" << std::endl;
        std::cerr << "  --seg-d3pm-nsteps <int> (default: 8)" << std::endl;
        std::cerr << "  --seg-d3pm-ts <float,float,...> (overrides t0 and nsteps)" << std::endl;
        std::cerr << "  --est-threshold <float> (default: 0.2)" << std::endl;
        std::cerr << "  --input-formats <...> (default: wav,flac,mp3,aac,ogg) [not parsed]" << std::endl;
        std::cerr << "  --output-formats <...> (default: mid) [not parsed]" << std::endl;
        std::cerr << "  --tempo <float> (default: 120)" << std::endl;
        std::cerr << "  --pitch-format <number|name> (default: name) [not parsed]" << std::endl;
        std::cerr << "  --round-pitch (flag, default: false)" << std::endl;
        std::cerr << "  --provider <cpu|dml|cuda> (default: cpu)" << std::endl;
        std::cerr << "  --device-id <int> (default: 0)" << std::endl;
        return 1;
    }

    const std::filesystem::path audioPath = argv[1];
    const std::filesystem::path modelDir = argv[2];
    const std::filesystem::path outputDir = argv[3];

    // TC-GI-003: Audio file does not exist
    if (!std::filesystem::exists(audioPath)) {
        std::cerr << "Error: Audio file does not exist: " << audioPath << std::endl;
        return 1;
    }

    // TC-GI-002: Model directory validation
    if (!std::filesystem::exists(modelDir)) {
        std::cerr << "Error: Model directory does not exist: " << modelDir << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(modelDir / "config.json")) {
        std::cerr << "Error: config.json not found in model directory: " << modelDir << std::endl;
        return 1;
    }

    std::vector<std::string> required_models = {"encoder.onnx", "segmenter.onnx", "estimator.onnx", "dur2bd.onnx",
                                                "bd2dur.onnx"};
    for (const auto &model_name : required_models) {
        if (!std::filesystem::exists(modelDir / model_name)) {
            std::cerr << "Error: Required model file not found: " << (modelDir / model_name) << std::endl;
            return 1;
        }
    }

    // Default values
    std::string language;
    float segThreshold = 0.2f;
    float segRadius = 0.02f;
    float segD3PMT0 = 0.0f;
    int segD3PMNSteps = 8;
    std::string segD3PMTsStr;
    float estThreshold = 0.2f;
    float tempo = 120.0f;
    bool roundPitch = false;
    std::string provider = "cpu";
    int deviceId = 0;

    // Parse arguments
    for (int i = 4; i < argc; i += 2) {
        std::string arg = argv[i];

        // Handle flag arguments (no value)
        if (arg == "--round-pitch") {
            roundPitch = true;
            i--; // Adjust: flag has no value, so compensate for i += 2
            continue;
        }

        // All remaining options require a value
        if (i + 1 >= argc) {
            std::cerr << "Warning: Option " << arg << " requires a value" << std::endl;
            break;
        }

        std::string val = argv[i + 1];

        if (arg == "--language") {
            language = val;
        } else if (arg == "--seg-threshold") {
            segThreshold = std::stof(val);
        } else if (arg == "--seg-radius") {
            segRadius = std::stof(val);
        } else if (arg == "--seg-d3pm-t0") {
            segD3PMT0 = std::stof(val);
        } else if (arg == "--seg-d3pm-nsteps") {
            segD3PMNSteps = std::stoi(val);
        } else if (arg == "--seg-d3pm-ts") {
            segD3PMTsStr = val;
        } else if (arg == "--est-threshold") {
            estThreshold = std::stof(val);
        } else if (arg == "--tempo") {
            tempo = std::stof(val);
        } else if (arg == "--provider") {
            provider = val;
        } else if (arg == "--device-id") {
            deviceId = std::stoi(val);
        } else {
            // TC-GI-008: Unknown options are silently skipped
            std::cout << "Warning: Unknown option " << arg << " (skipped)" << std::endl;
        }
    }

    // Initialize execution provider
    Game::ExecutionProvider gameProvider;
    if (provider == "dml") {
        gameProvider = Game::ExecutionProvider::DML;
    } else if (provider == "cuda") {
        gameProvider = Game::ExecutionProvider::CUDA;
    } else {
        gameProvider = Game::ExecutionProvider::CPU;
    }

    std::cout << "========================================" << std::endl;
    std::cout << " TestGame" << std::endl;
    std::cout << "========================================" << std::endl;

    // TC-GI-006: D3PM time step tests (no model needed)
    test_d3pm_ts();

    // Load model
    std::cout << "Loading GAME model from: " << modelDir << std::endl;
    std::cout << "Using provider: " << provider << ", device ID: " << deviceId << std::endl;

    Game::Game game;
    std::string msg;
    game.load_model(modelDir, gameProvider, deviceId, msg);

    if (!game.is_open()) {
        std::cerr << "Cannot load GAME Model from " << modelDir << std::endl;
        if (!msg.empty())
            std::cerr << "  Message: " << msg << std::endl;
        return 1;
    }

    std::cout << "GAME model loaded successfully" << std::endl;

    // Apply parameters
    game.set_seg_threshold(segThreshold);
    game.set_seg_radius_seconds(segRadius);
    game.set_est_threshold(estThreshold);

    std::vector<float> d3pmTs;
    if (!segD3PMTsStr.empty()) {
        std::vector<std::string> tsStrs = parseCommaSeparated(segD3PMTsStr);
        for (const auto &tsStr : tsStrs) {
            d3pmTs.push_back(std::stof(tsStr));
        }
    } else {
        d3pmTs = t0_n_step_to_ts(segD3PMT0, segD3PMNSteps);
    }
    game.set_d3pm_ts(d3pmTs);

    if (!language.empty()) {
        // TODO: Map language string to ID from config.json languages map
        game.set_language(0);
    }

    std::cout << "Applied configuration:" << std::endl;
    std::cout << "  seg_threshold: " << segThreshold << std::endl;
    std::cout << "  seg_radius: " << segRadius << " seconds" << std::endl;
    std::cout << "  est_threshold: " << estThreshold << std::endl;
    std::cout << "  d3pm_ts count: " << d3pmTs.size() << std::endl;
    std::cout << "  round_pitch: " << (roundPitch ? "true" : "false") << std::endl;
    if (!language.empty()) {
        std::cout << "  language: " << language << std::endl;
    }

    // Create output directory if needed
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directories(outputDir);
    }

    // TC-GI-001: End-to-end test
    test_e2e(game, audioPath, outputDir, tempo);

    std::cout << "========================================" << std::endl;
    std::cout << " Results: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
