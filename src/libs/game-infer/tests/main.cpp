#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <game-infer/Game.h>
#include <game-infer/GameModel.h>

#include "wolf-midi/MidiFile.h"

static void progressChanged(const int progress) { std::cout << "progress: " << progress << "%" << std::endl; }

// Helper function to parse comma-separated values
std::vector<std::string> parseCommaSeparated(const std::string &val) {
    std::vector<std::string> result;
    size_t pos = 0;
    std::string temp = val;
    while ((pos = temp.find(',')) != std::string::npos) {
        std::string token = temp.substr(0, pos);
        result.push_back(token);
        temp.erase(0, pos + 1);
    }
    result.push_back(temp);
    return result;
}

// Helper function to generate D3PM time steps
std::vector<float> t0_n_step_to_ts(const float t0, const int n_steps) {
    if (n_steps <= 0)
        return {};
    const float step = (1.0f - t0) / n_steps;
    std::vector<float> result;
    for (int i = 0; i < n_steps; ++i) {
        result.push_back(t0 + i * step);
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <audio_path> <model_dir> <output_dir> [options...]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  --language <lang_code> (default: none)" << std::endl;
        std::cerr << "  --batch-size <int> (default: 1)" << std::endl;
        std::cerr << "  --num-workers <int> (default: 0)" << std::endl;
        std::cerr << "  --seg-threshold <float> (default: 0.2)" << std::endl;
        std::cerr << "  --seg-radius <float> (default: 0.02)" << std::endl;
        std::cerr << "  --seg-d3pm-t0 <float> (default: 0.0)" << std::endl;
        std::cerr << "  --seg-d3pm-nsteps <int> (default: 8)" << std::endl;
        std::cerr << "  --seg-d3pm-ts <float,float,...> (overrides t0 and nsteps)" << std::endl;
        std::cerr << "  --est-threshold <float> (default: 0.2)" << std::endl;
        std::cerr << "  --input-formats <wav,flac,mp3,aac,ogg> (default: wav,flac,mp3,aac,ogg)" << std::endl;
        std::cerr << "  --output-formats <mid|txt|csv,...> (default: mid)" << std::endl;
        std::cerr << "  --tempo <float> (default: 120)" << std::endl;
        std::cerr << "  --pitch-format <number|name> (default: name)" << std::endl;
        std::cerr << "  --round-pitch (flag, default: false)" << std::endl;
        std::cerr << "  --provider <cpu|dml|cuda> (default: cpu)" << std::endl;
        std::cerr << "  --device-id <int> (default: 0)" << std::endl;
        return 1;
    }

    const std::filesystem::path audioPath = argv[1];
    const std::filesystem::path modelDir = argv[2];
    const std::filesystem::path outputDir = argv[3];

    // Validate input files exist
    if (!std::filesystem::exists(audioPath)) {
        std::cerr << "Error: Audio file does not exist: " << audioPath << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(modelDir)) {
        std::cerr << "Error: Model directory does not exist: " << modelDir << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(modelDir / "config.json")) {
        std::cerr << "Error: config.json not found in model directory: " << modelDir << std::endl;
        return 1;
    }

    // Check for required ONNX models
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
    std::string segD3PMTsStr; // Will be parsed later
    float estThreshold = 0.2f;
    float tempo = 120.0f;
    bool roundPitch = false;
    std::string provider = "cpu";
    int deviceId = 0;

    // Parse additional arguments
    for (int i = 4; i < argc; i += 2) {
        if (i + 1 >= argc)
            break;

        std::string arg = argv[i];
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
        } else if (arg == "--round-pitch") {
            // This is a flag, so we don't consume the next argument
            roundPitch = true;
            i--; // Adjust index since we didn't consume the next argument
        }
    }

    // Handle the --round-pitch flag case
    if (roundPitch) {
        // If --round-pitch was the last argument, we might have incremented i too far
        if (argc > 4 && std::string(argv[argc - 1]) == "--round-pitch") {
            // This is handled correctly above
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

    std::cout << "Loading GAME model from: " << modelDir << std::endl;
    std::cout << "Using provider: " << provider << ", device ID: " << deviceId << std::endl;

    // Create the GAME instance
    Game::Game game(modelDir, gameProvider, deviceId);

    if (!game.is_open()) {
        std::cerr << "Cannot load GAME Model from " << modelDir << std::endl;
        return 1;
    }

    std::cout << "GAME model loaded successfully" << std::endl;

    // Apply command-line parameters to the model
    game.set_seg_threshold(segThreshold);
    game.set_seg_radius_seconds(segRadius);
    game.set_est_threshold(estThreshold);

    // Determine D3PM time steps based on command line arguments
    std::vector<float> d3pmTs;
    if (!segD3PMTsStr.empty()) {
        // Parse comma-separated time steps
        std::vector<std::string> tsStrs = parseCommaSeparated(segD3PMTsStr);
        for (const auto &tsStr : tsStrs) {
            d3pmTs.push_back(std::stof(tsStr));
        }
    } else {
        // Generate time steps from t0 and nsteps
        d3pmTs = t0_n_step_to_ts(segD3PMT0, segD3PMNSteps);
    }
    game.set_d3pm_ts(d3pmTs);

    // Handle language parameter if provided
    if (!language.empty()) {
        // Here we would typically map language string to an integer ID
        // For now, we'll just set a default language ID (0 for unknown/universal)
        // In a real implementation, you'd look up the language code in the config's language map
        game.set_language(0); // Default to 0 (unknown/universal)
    }

    std::cout << "Applied configuration:" << std::endl;
    std::cout << "  seg_threshold: " << segThreshold << std::endl;
    std::cout << "  seg_radius: " << segRadius << " seconds" << std::endl;
    std::cout << "  est_threshold: " << estThreshold << std::endl;
    std::cout << "  d3pm_ts count: " << d3pmTs.size() << std::endl;
    if (!language.empty()) {
        std::cout << "  language: " << language << std::endl;
    }

    // Load and process audio file
    std::string msg;
    std::vector<Game::GameMidi> midis;

    std::cout << "Processing audio file: " << audioPath << std::endl;

    if (game.get_midi(audioPath, midis, tempo, msg, progressChanged)) {
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

        midi.save(outputDir / (audioPath.stem().string() + ".mid"));
        std::cout << "Save midi to: " << outputDir / (audioPath.stem().string() + ".mid") << std::endl;
    } else {
        std::cerr << "Error: " << msg << std::endl;
    }

    return 0;
}
