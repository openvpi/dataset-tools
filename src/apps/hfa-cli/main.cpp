#include <hubert-infer/Hfa.h>

#include <audio-util/Util.h>

#include <nlohmann/json.hpp>
#include <sndfile.hh>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void printUsage(const char *prog) {
    std::cerr << "Usage: " << prog << " --model <model_folder> --wav <audio.wav> --lab <lyrics.lab>\n"
              << "       [--save-wav <out.wav>]\n";
}

static std::string readFile(const fs::path &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Error: cannot open " << path << std::endl;
        std::exit(1);
    }
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    fs::path modelPath, wavPath, labPath, saveWavPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--model" && i + 1 < argc) modelPath = argv[++i];
        else if (arg == "--wav" && i + 1 < argc) wavPath = argv[++i];
        else if (arg == "--lab" && i + 1 < argc) labPath = argv[++i];
        else if (arg == "--save-wav" && i + 1 < argc) saveWavPath = argv[++i];
        else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
        else { std::cerr << "Unknown argument: " << arg << std::endl; printUsage(argv[0]); return 1; }
    }

    if (modelPath.empty() || wavPath.empty() || labPath.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    int modelSampleRate = 0;
    {
        auto cfgPath = modelPath / "config.json";
        std::ifstream f(cfgPath);
        if (f.is_open()) {
            auto cfg = nlohmann::json::parse(f);
            modelSampleRate = cfg["mel_spec_config"]["sample_rate"].get<int>();
        }
    }

    if (!saveWavPath.empty() && modelSampleRate > 0) {
        std::string errMsg;
        auto sf_vio = AudioUtil::resample_to_vio(wavPath, errMsg, 1, modelSampleRate);
        if (!errMsg.empty()) {
            std::cerr << "Resample error: " << errMsg << std::endl;
            return 1;
        }
        AudioUtil::write_vio_to_wav(sf_vio, saveWavPath, 1);
    }

    std::string lyrics = readFile(labPath);
    if (!lyrics.empty() && lyrics.back() == '\n') lyrics.pop_back();

    HFA::HFA hfa;
    auto loadResult = hfa.load(modelPath, HFA::ExecutionProvider::CPU, -1);
    if (!loadResult) {
        std::cerr << "Failed to load model: " << loadResult.error() << std::endl;
        return 1;
    }

    std::vector<std::string> nonSpeechPh = {"AP", "SP"};
    HFA::WordList words;
    auto result = hfa.recognize(wavPath, "zh", nonSpeechPh, lyrics, words);
    if (!result) {
        std::cerr << "FA failed: " << result.error() << std::endl;
        return 1;
    }

    nlohmann::json j;
    j["words"] = nlohmann::json::array();
    j["phonemes"] = nlohmann::json::array();

    for (const auto &word : words) {
        nlohmann::json wj;
        wj["text"] = word.text;
        wj["start"] = word.start;
        wj["end"] = word.end;
        wj["phones"] = nlohmann::json::array();
        for (const auto &ph : word.phones) {
            nlohmann::json pj;
            pj["phone"] = ph.text;
            pj["start"] = ph.start;
            pj["end"] = ph.end;
            wj["phones"].push_back(pj);
            j["phonemes"].push_back(pj);
        }
        j["words"].push_back(std::move(wj));
    }

    std::cout << j.dump(2) << std::endl;
    return 0;
}
