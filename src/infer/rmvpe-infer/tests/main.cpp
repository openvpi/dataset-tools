#include <cmath>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <rmvpe-infer/Rmvpe.h>

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
// CSV output (TC-RI-002: restored and adapted for RmvpeRes)
// ---------------------------------------------------------------------------
static void writeCsv(const std::string &csvFilename, const std::vector<Rmvpe::RmvpeRes> &res) {
    std::ofstream csvFile(csvFilename);
    if (!csvFile.is_open()) {
        std::cerr << "Error opening " << csvFilename << " for writing!" << std::endl;
        return;
    }

    for (const auto &chunk : res) {
        const float offsetMs = chunk.offset;
        for (size_t i = 0; i < chunk.f0.size(); ++i) {
            const float timeSec = (offsetMs + static_cast<float>(i) * 10.0f) / 1000.0f;
            csvFile << std::fixed << std::setprecision(5) << timeSec << "," << chunk.f0[i] << ","
                    << (chunk.uv[i] ? 1 : 0) << "\n";
        }
    }

    csvFile.close();
    std::cout << "CSV file '" << csvFilename << "' created successfully." << std::endl;
}

// ---------------------------------------------------------------------------
// Inference helper
// ---------------------------------------------------------------------------
static bool runInference(const Rmvpe::Rmvpe &rmvpe, const std::filesystem::path &wavPath, const float threshold,
                         std::vector<Rmvpe::RmvpeRes> &res, std::string &msg) {
    return rmvpe.get_f0(wavPath, threshold, res, msg, progressChanged);
}

// ---------------------------------------------------------------------------
// TC-RI-001: Basic F0 extraction (async)
// ---------------------------------------------------------------------------
static void test_basic_f0(const Rmvpe::Rmvpe &rmvpe, const std::filesystem::path &wavPath,
                          const std::string &csvOutput) {
    std::cout << "[TC-RI-001] Basic F0 extraction (async)" << std::endl;

    constexpr float threshold = 0.03f;
    std::vector<Rmvpe::RmvpeRes> res;
    std::string msg;

    auto inferenceTask = [&]() { return runInference(rmvpe, wavPath, threshold, res, msg); };

    std::future<bool> inferenceFuture = std::async(std::launch::async, inferenceTask);
    const bool success = inferenceFuture.get();

    TEST_ASSERT(success, "Inference failed: " + msg);
    TEST_ASSERT(!res.empty(), "Result is empty");

    // Count total F0 frames
    size_t totalFrames = 0;
    for (const auto &chunk : res) {
        totalFrames += chunk.f0.size();
        TEST_ASSERT(chunk.f0.size() == chunk.uv.size(), "f0 and uv size mismatch in chunk");
    }
    std::cout << "    Total F0 frames: " << totalFrames << std::endl;
    TEST_ASSERT(totalFrames > 0, "No F0 frames produced");

    // TC-RI-002: Write CSV if requested
    if (!csvOutput.empty()) {
        writeCsv(csvOutput, res);
        TEST_ASSERT(std::filesystem::exists(csvOutput), "CSV file was not created");
        TEST_ASSERT(std::filesystem::file_size(csvOutput) > 0, "CSV file is empty");
        std::cout << "  PASS: TC-RI-002 (CSV output)" << std::endl;
        g_passed++;
    }

    TEST_PASS("TC-RI-001");
}

// ---------------------------------------------------------------------------
// TC-RI-005: F0 value range validation
// ---------------------------------------------------------------------------
static void test_f0_range(const Rmvpe::Rmvpe &rmvpe, const std::filesystem::path &wavPath) {
    std::cout << "[TC-RI-005] F0 value range validation" << std::endl;

    constexpr float threshold = 0.03f;
    std::vector<Rmvpe::RmvpeRes> res;
    std::string msg;

    const bool success = rmvpe.get_f0(wavPath, threshold, res, msg, progressChanged);
    TEST_ASSERT(success, "Inference failed: " + msg);
    TEST_ASSERT(!res.empty(), "Result is empty");

    size_t voicedFrames = 0;
    size_t outOfRange = 0;
    for (const auto &chunk : res) {
        for (size_t i = 0; i < chunk.f0.size(); i++) {
            if (!chunk.uv[i]) {
                voicedFrames++;
                // Voiced frames should have F0 in 50Hz ~ 2000Hz
                if (chunk.f0[i] < 50.0f || chunk.f0[i] > 2000.0f) {
                    outOfRange++;
                }
            }
        }
    }

    std::cout << "    Voiced frames: " << voicedFrames << ", out of range: " << outOfRange << std::endl;
    TEST_ASSERT(outOfRange == 0,
                "Found " + std::to_string(outOfRange) + " voiced frames with F0 outside [50, 2000] Hz");

    TEST_PASS("TC-RI-005");
}

// ---------------------------------------------------------------------------
// TC-RI-007: Parameter count error (tested in main via argc check)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(const int argc, char *argv[]) {
    if (argc != 5 && argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <model_path> <wav_path> <dml/cpu> <device_id> [csv_output]" << std::endl;
        return 1;
    }

    const std::filesystem::path modelPath = argv[1];
    const std::filesystem::path wavPath = argv[2];
    const std::string provider = argv[3];
    const int device_id = std::stoi(argv[4]);
    const std::string csvOutput = argc == 6 ? argv[5] : "";

    const auto rmProvider = provider == "dml" ? Rmvpe::ExecutionProvider::DML : Rmvpe::ExecutionProvider::CPU;

    std::cout << "========================================" << std::endl;
    std::cout << " TestRmvpe" << std::endl;
    std::cout << "========================================" << std::endl;

    // Construct model
    Rmvpe::Rmvpe rmvpe(modelPath, rmProvider, device_id);

    if (!rmvpe.is_open()) {
        std::cerr << "Error: Failed to load RMVPE model from " << modelPath << std::endl;
        std::cerr << "  (This covers TC-RI-003: model load failure handling)" << std::endl;
        return 1;
    }

    std::cout << "Model loaded successfully." << std::endl;

    // TC-RI-001 + TC-RI-002
    test_basic_f0(rmvpe, wavPath, csvOutput);

    // TC-RI-005
    test_f0_range(rmvpe, wavPath);

    std::cout << "========================================" << std::endl;
    std::cout << " Results: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
