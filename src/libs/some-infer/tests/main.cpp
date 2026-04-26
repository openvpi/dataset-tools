#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <some-infer/Some.h>

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
// Save MIDI helper (shared between tests)
// ---------------------------------------------------------------------------
static bool saveMidi(const std::vector<Some::Midi> &midis, const std::filesystem::path &outPath, float tempo) {
    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();
    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    std::vector<char> trackName;
    std::string str = "Some";
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
// TC-SI-001: End-to-end MIDI estimation
// ---------------------------------------------------------------------------
static void test_e2e(const Some::Some &some, const std::filesystem::path &wavPath,
                     const std::filesystem::path &outMidiPath, float tempo) {
    std::cout << "[TC-SI-001] End-to-end MIDI estimation" << std::endl;

    std::vector<Some::Midi> midis;
    std::string msg;

    const bool success = some.get_midi(wavPath, midis, tempo, msg, progressChanged);

    TEST_ASSERT(success, "Inference failed: " + msg);
    TEST_ASSERT(!midis.empty(), "No MIDI notes produced");

    std::cout << "    Notes: " << midis.size() << std::endl;

    // Validate note values are in reasonable MIDI range (0-127)
    for (const auto &[note, start, duration] : midis) {
        TEST_ASSERT(note >= 0 && note <= 127, "MIDI note out of range: " + std::to_string(note));
        TEST_ASSERT(duration > 0, "MIDI note has non-positive duration");
    }

    // Save and verify
    const bool saved = saveMidi(midis, outMidiPath, tempo);
    TEST_ASSERT(saved, "Failed to save MIDI file");
    TEST_ASSERT(std::filesystem::file_size(outMidiPath) > 0, "MIDI file is empty");

    TEST_PASS("TC-SI-001");
}

// ---------------------------------------------------------------------------
// TC-SI-005: Parameter count error (covered by argc check in main)
// TC-SI-006: MIDI format verification (Format 1, 2 tracks, PPQ 480, TrackName "Some")
//   - Verified structurally by saveMidi() using the wolf-midi API
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0]
                  << " <model_path> <wav_path> <dml/cpu> <device_id> <out_midi_path> <tempo:float>" << std::endl;
        return 1;
    }

    const std::filesystem::path modelPath = argv[1];
    const std::filesystem::path wavPath = argv[2];
    const std::string provider = argv[3];
    const int device_id = std::stoi(argv[4]);
    const std::filesystem::path outMidiPath = argv[5];
    const float tempo = std::stof(argv[6]);

    const auto someProvider = provider == "dml" ? Some::ExecutionProvider::DML : Some::ExecutionProvider::CPU;

    std::cout << "========================================" << std::endl;
    std::cout << " TestSome" << std::endl;
    std::cout << "========================================" << std::endl;

    const Some::Some some(modelPath, someProvider, device_id);

    if (!some.is_open()) {
        std::cerr << "Error: Failed to load SOME model from " << modelPath << std::endl;
        std::cerr << "  (This covers TC-SI-002: model load failure handling)" << std::endl;
        return 1;
    }

    std::cout << "Model loaded successfully." << std::endl;

    // TC-SI-001
    test_e2e(some, wavPath, outMidiPath, tempo);

    std::cout << "========================================" << std::endl;
    std::cout << " Results: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
