#ifndef TIFA_ALIGNER_H
#define TIFA_ALIGNER_H

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "AlignOutput.h"
#include "Encoding.h"
#include "G2P.h"
#include "Provider.h"
#include "Config.h"
#include "Model.h"
#include "Vocabulary.h"

namespace Tifa {

    /** One file the aligner should process. */
    struct AlignRequest {
        std::filesystem::path wavPath;
        std::string text;
        Language language = Language::Mandarin;
        OovHandling oovHandling = OovHandling::Raise;
        SkipHandling skipHandling = SkipHandling::Omit;
        float skipPenalty = 0.5f;
    };

    /** Outcome of one alignment. */
    struct AlignResult {
        /** Serialized TextGrid; empty when nothing must be written. */
        std::string textGrid;
        /** Notes about the tokens the decoder skipped. */
        std::string report;
        /** Non-fatal notes of this run, such as dropped pronunciations. */
        std::vector<std::string> warnings;
        /** Mean probability of the aligned tokens, in ``[0, 1]``. */
        float agreement = 0.0f;
        int64_t words = 0;
        int64_t phones = 0;
        int64_t skipped = 0;
    };

    /**
     * TIFA forced aligner.
     *
     * Holds one exported model, the vocabulary, the G2P front end and the
     * script dictionaries. The pipeline follows TIFA's inference path exactly:
     * spectrogram, candidate preparation, acoustic scoring, the whole-word
     * candidate DP, path materialization and the flat Viterbi alignment.
     */
    class Aligner {
    public:
        Aligner() = default;
        ~Aligner() = default;

        Aligner(const Aligner &) = delete;
        Aligner &operator=(const Aligner &) = delete;

        /**
         * Load a model directory and the dictionaries.
         *
         * @param modelDir Folder with config.json, vocabulary.json and the five
         *                 exported graphs.
         * @param dictDir  Folder with the TIFA dictionaries, the optional
         *                 LstmG2p-Eng assets and the cpp-pinyin dictionaries.
         */
        bool load(const std::filesystem::path &modelDir, const std::filesystem::path &dictDir,
                  ExecutionProvider provider, int deviceId, std::string &error);

        bool initialized() const {
            return m_initialized;
        }

        const Config &config() const {
            return m_config;
        }

        /** Output channels of the loaded vocabulary. */
        int vocabularySize() const {
            return m_vocabulary.vocabSize();
        }

        /** Language tags that have a usable converter. */
        const std::vector<std::string> &availableLanguages() const {
            return m_g2p.availableLanguages();
        }

        /** Warnings collected while loading the dictionaries. */
        const std::vector<std::string> &warnings() const {
            return m_g2p.warnings();
        }

        /** Align one waveform against one text. */
        bool align(const AlignRequest &request, AlignResult &result, std::string &error) const;

    private:
        bool loadAudio(const std::filesystem::path &path, std::vector<float> &samples, std::string &error) const;

        mutable std::mutex m_mutex;
        bool m_initialized = false;
        Config m_config;
        Vocabulary m_vocabulary;
        G2PPipeline m_g2p;
        Model m_model;
    };

} // namespace Tifa

#endif // TIFA_ALIGNER_H
