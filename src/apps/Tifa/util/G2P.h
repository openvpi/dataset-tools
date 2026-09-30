#ifndef TIFA_G2P_H
#define TIFA_G2P_H

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Vocabulary.h"

namespace Tifa {

    /** Marker for converters that resolve their language through the vocabulary. */
    extern const char *const kAnyLanguage;

    /** A converter could not turn its input into phonemes. */
    class G2PError : public std::runtime_error {
    public:
        explicit G2PError(const std::string &message) : std::runtime_error(message) {
        }
    };

    /** A contiguous pronunciation group and its pronunciation script. */
    struct G2PGroup {
        std::string script;
        std::vector<std::string> phonemes;
    };

    using G2PPath = std::vector<G2PGroup>;

    /** Complete legal phoneme realizations of one reading. */
    struct G2PReading {
        std::vector<G2PPath> paths;
    };

    /** One converter-defined output word, with its alternative readings. */
    struct G2PWord {
        std::string text;
        /** Language tag, empty when unset, or kAnyLanguage. */
        std::string language;
        std::vector<G2PReading> readings;
    };

    /** Tab-separated pronunciation dictionary: ``key<TAB>ph1 ph2 ...``. */
    class PronunciationDictionary {
    public:
        bool load(const std::filesystem::path &path, std::string &error);
        void add(const std::string &key, std::vector<std::string> phonemes);
        const std::vector<std::vector<std::string>> *find(const std::string &key) const;
        bool contains(const std::string &key) const {
            return find(key) != nullptr;
        }
        size_t size() const {
            return m_entries.size();
        }

    private:
        std::unordered_map<std::string, std::vector<std::vector<std::string>>> m_entries;
    };

    /** Files the G2P front end needs. */
    struct G2PResources {
        /** Folder with the TIFA script dictionaries and LstmG2p-Eng assets. */
        std::filesystem::path dictDir;
        /** Folder holding the cpp-pinyin dictionaries (with mandarin/ and cantonese/). */
        std::filesystem::path pinyinDir;
    };

    class Converter {
    public:
        virtual ~Converter() = default;

        /** Language tags of this converter; empty means unset, kAnyLanguage means any. */
        std::vector<std::string> languages;

        virtual bool find(const std::u32string &text, size_t &begin, size_t &end) const = 0;
        virtual std::vector<G2PWord> convert(const std::u32string &text) const = 0;
    };

    /**
     * G2P front end of TIFA.
     *
     * Converters are tried in a fixed priority order; the first one that
     * accepts a stretch of text claims it and the remaining text is routed
     * again. The chain matches the inference G2P configuration of TIFA:
     * Mandarin, Japanese, Cantonese, English, script dictionaries and a
     * passthrough fallback.
     */
    class G2PPipeline {
    public:
        G2PPipeline();
        ~G2PPipeline();

        /**
         * Build every converter whose resources are available.
         *
         * The cpp-pinyin dictionaries and the TIFA script dictionaries are
         * required; the English OOV model is optional.
         */
        bool init(const G2PResources &resources, std::string &error);

        /**
         * Convert text into G2P words.
         *
         * @param languages Active language tags in priority order; empty enables
         *                  every converter, matching the inference pipeline.
         * @param error Receives a message when conversion fails.
         */
        bool convert(const std::u32string &text, const std::vector<std::string> &languages, std::vector<G2PWord> &words,
                     std::string &error) const;

        /** Language tags that have a usable converter. */
        const std::vector<std::string> &availableLanguages() const {
            return m_availableLanguages;
        }

        /** Warnings collected while building the converters. */
        const std::vector<std::string> &warnings() const {
            return m_warnings;
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
        std::vector<std::string> m_availableLanguages;
        std::vector<std::string> m_warnings;
    };

} // namespace Tifa

#endif // TIFA_G2P_H
