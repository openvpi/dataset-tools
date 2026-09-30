#include "Config.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include "PathUtils.h"

using json = nlohmann::json;

namespace Tifa {

    bool Config::load(const std::filesystem::path &path, std::string &error) {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            error = "Failed to open model config: " + pathToUtf8(path);
            return false;
        }
        json data;
        try {
            data = json::parse(stream);
        } catch (const std::exception &e) {
            error = "Failed to parse model config: " + pathToUtf8(path) + " (" + e.what() + ")";
            return false;
        }

        const auto readInt = [&data, &error](const char *key, int &value) {
            if (!data.contains(key)) {
                error = std::string("Model config has no '") + key + "' field.";
                return false;
            }
            if (!data[key].is_number()) {
                error = std::string("Model config field '") + key + "' is not a number.";
                return false;
            }
            value = data[key].get<int>();
            return true;
        };

        if (!readInt("samplerate", sampleRate) || !readInt("hop_size", hopSize) || !readInt("fft_size", fftSize) ||
            !readInt("win_size", winSize) || !readInt("num_mels", numMels) || !readInt("vocab_size", vocabSize)) {
            return false;
        }
        if (data.contains("timestep") && data["timestep"].is_number()) {
            timestep = data["timestep"].get<double>();
        }

        if (sampleRate <= 0 || hopSize <= 0 || fftSize <= 0 || winSize <= 0 || numMels <= 0) {
            error = "Model config contains invalid values: " + pathToUtf8(path);
            return false;
        }
        return true;
    }

    const char *languageTag(Language language) {
        switch (language) {
            case Language::Mandarin:
                return "zh";
            case Language::Cantonese:
                return "yue";
            case Language::Japanese:
                return "ja";
            case Language::English:
                return "en";
        }
        return "zh";
    }

    const char *languageName(Language language) {
        switch (language) {
            case Language::Mandarin:
                return "Mandarin (zh)";
            case Language::Cantonese:
                return "Cantonese (yue)";
            case Language::Japanese:
                return "Japanese (ja)";
            case Language::English:
                return "English (en)";
        }
        return "Mandarin (zh)";
    }

} // namespace Tifa
