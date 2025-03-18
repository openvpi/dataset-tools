#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include <rmvpe-infer/Rmvpe.h>

static void progressChanged(const int progress) { std::cout << "progress: " << progress << "%" << std::endl; }

static void writeCsv(const std::string &csvFilename, const std::vector<float> &f0, const std::vector<bool> &uv) {
    std::ofstream csvFile(csvFilename);
    if (!csvFile.is_open()) {
        std::cerr << "Error opening " << csvFilename << " for writing!" << std::endl;
        return;
    }

    int timeMs = 0;
    for (size_t i = 0; i < f0.size(); ++i) {
        csvFile << std::fixed << std::setprecision(5) << static_cast<float>(timeMs) / 1000.0f << "," << f0[i] << ","
                << (uv[i] ? 1 : 0) << "\n";
        timeMs += 10;
    }

    csvFile.close();
    std::cout << "CSV file '" << csvFilename << "' created successfully." << std::endl;
}

void runInference(const Rmvpe::Rmvpe &rmvpe, const std::filesystem::path &wavPath, const float threshold,
                  std::vector<Rmvpe::RmvpeRes> &res, std::string &msg,
                  const std::function<void(int)> &progressChanged) {
    const bool success = rmvpe.get_f0(wavPath, threshold, res, msg, progressChanged);

    if (!success) {
        std::cerr << "Error: " << msg << std::endl;
    }
}

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

    Rmvpe::Rmvpe rmvpe(modelPath, rmProvider, device_id);
    constexpr float threshold = 0.03f;

    std::vector<Rmvpe::RmvpeRes> res;
    std::string msg;

    auto inferenceTask = [&rmvpe, &wavPath, &threshold, &res, &msg]
    { runInference(rmvpe, wavPath, threshold, res, msg, progressChanged); };

    std::future<void> inferenceFuture = std::async(std::launch::async, inferenceTask);
    //
    // std::thread terminateThread(terminateRmvpeAfterDelay, std::ref(rmvpe), 10);
    // terminateThread.join();

    inferenceFuture.get();

    // if (!f0.empty()) {
    //     // std::cout << "midi output:" << std::endl;
    //     // const auto midi = freqToMidi(f0);
    //     // for (const float value : midi) {
    //     //     std::cout << value << " ";
    //     // }
    //     // std::cout << std::endl;
    //
    //     if (!csvOutput.empty() && csvOutput.substr(csvOutput.find_last_of('.')) == ".csv") {
    //         writeCsv(csvOutput, f0, uv);
    //     } else if (!csvOutput.empty()) {
    //         std::cerr << "Error: The CSV output path must end with '.csv'." << std::endl;
    //     }
    // } else {
    //     std::cerr << "Error: " << msg << std::endl;
    // }

    return 0;
}
