#ifndef TIFA_LSTMG2P_H
#define TIFA_LSTMG2P_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Tifa {

    /**
     * LSTM letter-to-sound model for out-of-vocabulary words.
     *
     * Port of the ONNX beam search of TIFA's ``lstm`` G2P converter. The model
     * folder holds ``encoder.onnx``, ``decoder.onnx``, ``char.json`` and
     * ``phonemes.json``. The converter is optional: without the model folder
     * every word must be present in the pronunciation dictionary.
     */
    class LstmG2P {
    public:
        LstmG2P();
        ~LstmG2P();

        /** Load the vocabularies and create both ONNX sessions. */
        bool init(const std::filesystem::path &modelDir, std::string &error);

        bool available() const;

        /** Whether the model can encode every character of the token. */
        bool usesCharacters(const std::u32string &token) const;

        /** Beam search pronunciations, ranked by mean log probability. */
        std::vector<std::vector<std::string>> predict(const std::u32string &word, std::string &error) const;

        int beamSize = 16;
        int maxLength = 48;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace Tifa

#endif // TIFA_LSTMG2P_H
