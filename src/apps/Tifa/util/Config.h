#ifndef TIFA_CONFIG_H
#define TIFA_CONFIG_H

#include <filesystem>
#include <string>

namespace Tifa {

    /** Feature extraction settings of an exported TIFA model (config.json). */
    struct Config {
        int sampleRate = 48000;
        double timestep = 0.01;
        int hopSize = 480;
        int fftSize = 2048;
        int winSize = 2048;
        int numMels = 80;
        int vocabSize = 256;

        bool load(const std::filesystem::path &path, std::string &error);
    };

    /** Languages supported by the built-in G2P front end. */
    enum class Language { Mandarin, Cantonese, Japanese, English };

    /** @return Language tag used inside the TIFA vocabulary ("zh", "yue", "ja", "en"). */
    const char *languageTag(Language language);

    /** @return Human readable name shown in the UI. */
    const char *languageName(Language language);

} // namespace Tifa

#endif // TIFA_CONFIG_H
