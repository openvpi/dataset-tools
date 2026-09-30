#include "G2P.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <regex>
#include <tuple>

#include <cpp-pinyin/Jyutping.h>
#include <cpp-pinyin/Pinyin.h>

#include "LstmG2P.h"
#include "PathUtils.h"
#include "TextUtils.h"

namespace Tifa {

    const char *const kAnyLanguage = "*";

    namespace {

        using PronMap = std::unordered_map<std::string, std::vector<std::vector<std::string>>>;
        using CharReadings = std::unordered_map<char32_t, std::vector<std::string>>;
        using TransMap = std::unordered_map<char32_t, char32_t>;

        bool containsValue(const std::vector<std::string> &values, const std::string &value) {
            return std::find(values.begin(), values.end(), value) != values.end();
        }

        std::string trimLine(std::string line) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            return line;
        }

        bool isConsonant(char c) {
            static const std::string consonants = "bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ";
            return consonants.find(c) != std::string::npos;
        }

        /**
         * Tab-separated pronunciation dictionary.
         *
         * Port of ``load_pronunciation_dict``: ``<key>\t<ph1> <ph2> ...``, lines
         * without a phoneme column are ignored and duplicate keys accumulate.
         */
        bool loadPronunciationDictFile(const std::filesystem::path &path, PronMap &entries, std::string &error) {
            std::ifstream stream(path);
            if (!stream) {
                error = "Failed to open the pronunciation dictionary: " + pathToUtf8(path);
                return false;
            }
            std::string line;
            while (std::getline(stream, line)) {
                line = trimLine(std::move(line));
                if (line.empty()) {
                    continue;
                }
                const size_t separator = line.find('\t');
                if (separator == std::string::npos || separator + 1 >= line.size()) {
                    continue;
                }
                const std::string key = line.substr(0, separator);
                const std::string phonemes = line.substr(separator + 1);
                std::vector<std::string> tokens;
                size_t index = 0;
                while (index < phonemes.size()) {
                    while (index < phonemes.size() && std::isspace(static_cast<unsigned char>(phonemes[index]))) {
                        index += 1;
                    }
                    const size_t begin = index;
                    while (index < phonemes.size() && !std::isspace(static_cast<unsigned char>(phonemes[index]))) {
                        index += 1;
                    }
                    if (index > begin) {
                        tokens.push_back(phonemes.substr(begin, index - begin));
                    }
                }
                entries[key].push_back(std::move(tokens));
            }
            return true;
        }

        /** Tone marks accepted by cpp-pinyin, mapped to their base letter. */
        const std::unordered_map<char32_t, char32_t> &toneMarkMap() {
            static const std::unordered_map<char32_t, char32_t> map = {
                {0x0101, U'a'}, {0x00E1, U'a'}, {0x01CE, U'a'}, {0x00E0, U'a'},
                {0x0113, U'e'}, {0x00E9, U'e'}, {0x011B, U'e'}, {0x00E8, U'e'},
                {0x012B, U'i'}, {0x00ED, U'i'}, {0x01D0, U'i'}, {0x00EC, U'i'},
                {0x014D, U'o'}, {0x00F3, U'o'}, {0x01D2, U'o'}, {0x00F2, U'o'},
                {0x016B, U'u'}, {0x00FA, U'u'}, {0x01D4, U'u'}, {0x00F9, U'u'},
                {0x01D6, U'v'}, {0x01D8, U'v'}, {0x01DA, U'v'}, {0x01DC, U'v'},
                {0x00FC, U'v'},
                {0x0144, U'n'}, {0x0148, U'n'}, {0x01F9, U'n'},
                {0x1E3F, U'm'},
            };
            return map;
        }

        /**
         * ``apply_tone(..., STYLE_NORMAL)``: drop the tone, spell ü as v.
         *
         * Accepts both tone-marked Mandarin input and TONE3 input.
         */
        std::string normalStyle(const std::string &pinyin) {
            if (pinyin.empty()) {
                return pinyin;
            }
            const std::u32string decoded = Text::decode(pinyin);
            std::u32string base;
            if (pinyin.back() >= '0' && pinyin.back() <= '9') {
                base = decoded.substr(0, decoded.size() - 1);
            } else {
                for (const char32_t c : decoded) {
                    const auto it = toneMarkMap().find(c);
                    base.push_back(it != toneMarkMap().end() ? it->second : c);
                }
            }
            for (char32_t &c : base) {
                if (c == 0x00FC) {
                    c = U'v';
                }
            }
            return Text::encode(base);
        }

        /** Character tables of one cpp-pinyin dictionary directory. */
        struct ChineseDicts {
            /** Simplified character to every reading, in NORMAL style. */
            CharReadings readings;
            /** Traditional to simplified character. */
            TransMap trans;

            bool load(const std::filesystem::path &dir, std::string &error) {
                std::ifstream wordStream(dir / "word.txt");
                if (!wordStream) {
                    error = "Failed to open the cpp-pinyin dictionary: " + pathToUtf8(dir / "word.txt");
                    return false;
                }
                std::string line;
                while (std::getline(wordStream, line)) {
                    line = trimLine(std::move(line));
                    const size_t separator = line.find(':');
                    if (separator == std::string::npos || separator == 0) {
                        continue;
                    }
                    const std::u32string symbol = Text::decode(line.substr(0, separator));
                    if (symbol.size() != 1) {
                        continue;
                    }
                    std::vector<std::string> readingsOfChar;
                    size_t index = separator + 1;
                    while (index < line.size()) {
                        const size_t end = line.find(',', index);
                        const std::string pron = line.substr(index, end == std::string::npos ? std::string::npos
                                                                                            : end - index);
                        if (!pron.empty()) {
                            readingsOfChar.push_back(normalStyle(pron));
                        }
                        if (end == std::string::npos) {
                            break;
                        }
                        index = end + 1;
                    }
                    if (!readingsOfChar.empty()) {
                        readings[symbol.front()] = std::move(readingsOfChar);
                    }
                }

                std::ifstream transStream(dir / "trans_word.txt");
                if (!transStream) {
                    error = "Failed to open the cpp-pinyin dictionary: " + pathToUtf8(dir / "trans_word.txt");
                    return false;
                }
                while (std::getline(transStream, line)) {
                    line = trimLine(std::move(line));
                    const size_t separator = line.find(':');
                    if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size()) {
                        continue;
                    }
                    const std::u32string traditional = Text::decode(line.substr(0, separator));
                    const std::u32string simplified = Text::decode(line.substr(separator + 1));
                    if (traditional.size() == 1 && simplified.size() == 1) {
                        trans[traditional.front()] = simplified.front();
                    }
                }
                return true;
            }
        };

        std::u32string simplify(const std::u32string &symbol, const ChineseDicts &dicts) {
            std::u32string result;
            result.reserve(symbol.size());
            for (const char32_t c : symbol) {
                const auto it = dicts.trans.find(c);
                result.push_back(it != dicts.trans.end() ? it->second : c);
            }
            return result;
        }

        /**
         * Shared base of the script converters: words are rendered into
         * pronunciation-script tokens and every script token is expanded to the
         * complete paths of the script dictionary.
         */
        class ScriptConverter : public Converter {
        public:
            ScriptConverter(const PronMap *scriptDict, std::vector<std::string> wordLanguages)
                : m_scriptDict(scriptDict) {
                languages = std::move(wordLanguages);
            }

        protected:
            const PronMap &scriptDict() const {
                return *m_scriptDict;
            }

            std::vector<G2PPath> scriptToPaths(const std::string &script) const {
                const auto it = m_scriptDict->find(script);
                if (it == m_scriptDict->end()) {
                    throw G2PError("Script token '" + script + "' not found in the script-to-phoneme dictionary.");
                }
                std::vector<G2PPath> paths;
                for (const auto &phonemes : it->second) {
                    G2PPath path;
                    if (!phonemes.empty()) {
                        path.push_back(G2PGroup{script, phonemes});
                    }
                    const bool duplicate = std::any_of(paths.begin(), paths.end(), [&path](const G2PPath &other) {
                        return groupKey(other) == groupKey(path);
                    });
                    if (!duplicate) {
                        paths.push_back(std::move(path));
                    }
                }
                return paths;
            }

            /** Reads one word with every script alternative, dropping duplicates. */
            void appendWord(std::vector<G2PWord> &result, const std::u32string &token,
                            const std::vector<std::string> &scripts) const {
                G2PWord word;
                word.text = Text::encode(token);
                std::vector<std::string> seenScripts;
                for (const std::string &script : scripts) {
                    if (containsValue(seenScripts, script)) {
                        continue;
                    }
                    seenScripts.push_back(script);
                    G2PReading reading;
                    reading.paths = scriptToPaths(script);
                    word.readings.push_back(std::move(reading));
                }
                result.push_back(std::move(word));
            }

        private:
            static std::string groupKey(const G2PPath &path) {
                std::string key;
                for (const G2PGroup &group : path) {
                    key += group.script;
                    key.push_back('\x1f');
                    for (const std::string &phoneme : group.phonemes) {
                        key += phoneme;
                        key.push_back(' ');
                    }
                    key.push_back('\x1e');
                }
                return key;
            }

            const PronMap *m_scriptDict;
        };

        /**
         * Mandarin and Cantonese hanzi converters.
         *
         * Port of TIFA's ``_ChineseScriptConverter``: hanzi are simplified and
         * rendered to pinyin (or jyutping) by cpp-pinyin, the alternative
         * readings are taken from cpp-pinyin's character table, and only the
         * readings known to the script dictionary survive.
         */
        class ChineseConverter final : public ScriptConverter {
        public:
            ChineseConverter(bool cantonese, Pinyin::Pinyin *mandarin, Pinyin::Jyutping *jyutping,
                             const ChineseDicts *dicts, const PronMap *scriptDict, std::vector<std::string> wordLanguages)
                : ScriptConverter(scriptDict, std::move(wordLanguages)), m_cantonese(cantonese), m_mandarin(mandarin),
                  m_jyutping(jyutping), m_dicts(dicts) {
            }

            bool find(const std::u32string &text, size_t &begin, size_t &end) const override {
                return Text::findRun(text, [](char32_t c) { return Text::isHanzi(c); }, begin, end);
            }

            std::vector<G2PWord> convert(const std::u32string &text) const override {
                const std::vector<std::u32string> words = Text::splitWords(text);
                std::vector<std::u32string> simplified;
                simplified.reserve(words.size());
                for (const std::u32string &word : words) {
                    simplified.push_back(simplify(word, *m_dicts));
                }
                const std::vector<std::string> best = query(simplified);

                std::vector<G2PWord> result;
                result.reserve(words.size());
                for (size_t i = 0; i < words.size(); ++i) {
                    const std::string primary = i < best.size() ? best[i] : std::string();
                    std::vector<std::string> scripts{primary};
                    for (const std::string &reading : readings(simplified[i])) {
                        if (reading != primary) {
                            scripts.push_back(reading);
                        }
                    }
                    std::vector<std::string> supported;
                    for (const std::string &script : scripts) {
                        if (scriptDict().count(script)) {
                            supported.push_back(script);
                        }
                    }
                    // Keep the unconverted readings so that the lookup error names them.
                    appendWord(result, words[i], supported.empty() ? scripts : supported);
                }
                return result;
            }

        private:
            /** cpp-pinyin queryRaw() over the whole run, so phrases disambiguate. */
            std::vector<std::string> query(const std::vector<std::u32string> &symbols) const {
                std::u32string joined;
                for (const std::u32string &symbol : symbols) {
                    joined += symbol;
                }
                std::vector<std::string> result = querySingle(joined);
                if (result.size() == symbols.size()) {
                    return result;
                }
                // cpp-pinyin merged or dropped characters: fall back to one query per character.
                result.clear();
                for (const std::u32string &symbol : symbols) {
                    const std::vector<std::string> single = querySingle(symbol);
                    result.push_back(single.empty() ? Text::encode(symbol) : single.front());
                }
                return result;
            }

            std::vector<std::string> querySingle(const std::u32string &symbol) const {
                const std::string input = Text::encode(symbol);
                std::vector<std::string> result;
                if (m_cantonese) {
                    const auto readingsOfRun =
                        m_jyutping->hanziToPinyin(input, Pinyin::CanTone::NORMAL, Pinyin::Error::Default);
                    for (const auto &item : readingsOfRun) {
                        result.push_back(normalStyle(item.pinyin));
                    }
                } else {
                    const auto readingsOfRun =
                        m_mandarin->hanziToPinyin(input, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, false, false);
                    for (const auto &item : readingsOfRun) {
                        result.push_back(normalStyle(item.pinyin));
                    }
                }
                return result;
            }

            /** Every reading of a simplified character, in dictionary order. */
            std::vector<std::string> readings(const std::u32string &symbol) const {
                if (symbol.size() == 1) {
                    const auto it = m_dicts->readings.find(symbol.front());
                    if (it != m_dicts->readings.end()) {
                        return it->second;
                    }
                }
                return {Text::encode(symbol)};
            }

            bool m_cantonese;
            Pinyin::Pinyin *m_mandarin;
            Pinyin::Jyutping *m_jyutping;
            const ChineseDicts *m_dicts;
        };

        /**
         * Japanese kana converter.
         *
         * Port of TIFA's ``JapaneseKanaConverter``. TIFA routes Japanese text
         * through MeCab first and only falls back to this converter for kana;
         * without a morphological analyzer kanji cannot be read, which is
         * reported as an error instead of being aligned to the wrong phonemes.
         */
        class KanaConverter final : public ScriptConverter {
        public:
            KanaConverter(const PronMap *scriptDict, std::vector<std::string> wordLanguages)
                : ScriptConverter(scriptDict, std::move(wordLanguages)) {
            }

            bool find(const std::u32string &text, size_t &begin, size_t &end) const override {
                // Matches the ``japanese-mecab`` converter, which claims kanji as well.
                return Text::findRun(text, [](char32_t c) { return Text::isJapaneseChar(c); }, begin, end);
            }

            std::vector<G2PWord> convert(const std::u32string &text) const override {
                std::vector<G2PWord> result;
                for (const std::u32string &word : Text::splitWords(text)) {
                    const bool kanaOnly = std::all_of(word.begin(), word.end(), [](char32_t c) {
                        return Text::isKana(c);
                    });
                    if (!kanaOnly) {
                        throw G2PError("Japanese word '" + Text::encode(word) +
                                       "' contains kanji: reading it needs the MeCab converter of TIFA "
                                       "(fugashi/unidic), which is not available.");
                    }
                    const std::u32string hiragana = katakanaToHiragana(word);
                    std::string script;
                    // The long vowel mark and the handakuten carry no phoneme of their own.
                    if (hiragana != U"ー" && hiragana != U"゜") {
                        const auto it = kanaToRomaji().find(hiragana);
                        script = it != kanaToRomaji().end() ? it->second : Text::encode(hiragana);
                    }

                    G2PWord converted;
                    converted.text = Text::encode(word);
                    G2PReading reading;
                    if (script.empty()) {
                        reading.paths.push_back({});
                    } else if (scriptDict().count(script)) {
                        reading.paths = scriptToPaths(script);
                    } else if (script.size() == 1 && isConsonant(script.front())) {
                        // A lone consonant left over from gemination passes through.
                        reading.paths.push_back({G2PGroup{script, {script}}});
                    } else {
                        throw G2PError("Script token '" + script +
                                       "' not found in the script-to-phoneme dictionary.");
                    }
                    converted.readings.push_back(std::move(reading));
                    result.push_back(std::move(converted));
                }
                return result;
            }

        private:
            static std::u32string katakanaToHiragana(const std::u32string &text) {
                constexpr char32_t katakanaStart = 0x30A1;
                constexpr char32_t hiraganaStart = 0x3041;
                constexpr char32_t kanaSpan = 0x5E;
                std::u32string result;
                result.reserve(text.size());
                for (const char32_t c : text) {
                    if (c >= katakanaStart && c < katakanaStart + kanaSpan) {
                        result.push_back(c - katakanaStart + hiraganaStart);
                    } else {
                        result.push_back(c);
                    }
                }
                return result;
            }

            /** Hiragana romaji table of TIFA, a port of cpp-kana's kanaToRomajiMap. */
            static const std::unordered_map<std::u32string, std::string> &kanaToRomaji() {
                static const std::unordered_map<std::u32string, std::string> map = {
                    {U"っ", "cl"},
                    {U"あ", "a"},   {U"い", "i"},   {U"う", "u"},   {U"え", "e"},   {U"お", "o"},
                    {U"か", "ka"},  {U"き", "ki"},  {U"く", "ku"},  {U"け", "ke"},  {U"こ", "ko"},
                    {U"さ", "sa"},  {U"し", "shi"}, {U"す", "su"},  {U"せ", "se"},  {U"そ", "so"},
                    {U"た", "ta"},  {U"ち", "chi"}, {U"つ", "tsu"}, {U"て", "te"},  {U"と", "to"},
                    {U"な", "na"},  {U"に", "ni"},  {U"ぬ", "nu"},  {U"ね", "ne"},  {U"の", "no"},
                    {U"は", "ha"},  {U"ひ", "hi"},  {U"ふ", "fu"},  {U"へ", "he"},  {U"ほ", "ho"},
                    {U"ま", "ma"},  {U"み", "mi"},  {U"む", "mu"},  {U"め", "me"},  {U"も", "mo"},
                    {U"や", "ya"},  {U"ゆ", "yu"},  {U"よ", "yo"},
                    {U"ら", "ra"},  {U"り", "ri"},  {U"る", "ru"},  {U"れ", "re"},  {U"ろ", "ro"},
                    {U"わ", "wa"},  {U"ゐ", "wi"},  {U"ゑ", "we"},  {U"ん", "n"},
                    {U"が", "ga"},  {U"ぎ", "gi"},  {U"ぐ", "gu"},  {U"げ", "ge"},  {U"ご", "go"},
                    {U"ざ", "za"},  {U"じ", "ji"},  {U"ず", "zu"},  {U"ぜ", "ze"},  {U"ぞ", "zo"},
                    {U"だ", "da"},  {U"ぢ", "ji"},  {U"づ", "zu"},  {U"で", "de"},  {U"ど", "do"},
                    {U"ば", "ba"},  {U"び", "bi"},  {U"ぶ", "bu"},  {U"べ", "be"},  {U"ぼ", "bo"},
                    {U"ぱ", "pa"},  {U"ぴ", "pi"},  {U"ぷ", "pu"},  {U"ぺ", "pe"},  {U"ぽ", "po"},
                    {U"きゃ", "kya"}, {U"きゅ", "kyu"}, {U"きょ", "kyo"}, {U"きぇ", "kye"},
                    {U"ぎゃ", "gya"}, {U"ぎゅ", "gyu"}, {U"ぎょ", "gyo"}, {U"ぎぇ", "gye"},
                    {U"しゃ", "sha"}, {U"しゅ", "shu"}, {U"しょ", "sho"}, {U"しぇ", "she"},
                    {U"じゃ", "ja"},  {U"じゅ", "ju"},  {U"じょ", "jo"},  {U"じぇ", "je"},
                    {U"ちゃ", "cha"}, {U"ちゅ", "chu"}, {U"ちょ", "cho"}, {U"ちぇ", "che"},
                    {U"にゃ", "nya"}, {U"にゅ", "nyu"}, {U"にょ", "nyo"}, {U"にぇ", "nye"},
                    {U"ひゃ", "hya"}, {U"ひゅ", "hyu"}, {U"ひょ", "hyo"}, {U"ひぇ", "hye"},
                    {U"びゃ", "bya"}, {U"びゅ", "byu"}, {U"びょ", "byo"}, {U"びぇ", "bye"},
                    {U"ぴゃ", "pya"}, {U"ぴゅ", "pyu"}, {U"ぴょ", "pyo"}, {U"ぴぇ", "pye"},
                    {U"みゃ", "mya"}, {U"みゅ", "myu"}, {U"みょ", "myo"}, {U"みぇ", "mye"},
                    {U"りゃ", "rya"}, {U"りゅ", "ryu"}, {U"りょ", "ryo"}, {U"りぇ", "rye"},
                    {U"いぇ", "ye"},
                    {U"うぁ", "wa"},  {U"うぃ", "wi"},  {U"うぇ", "we"},  {U"うぉ", "wo"},
                    {U"くぁ", "kwa"}, {U"くぃ", "kwi"}, {U"くぇ", "kwe"}, {U"くぉ", "kwo"},
                    {U"ぐぁ", "gwa"}, {U"ぐぃ", "gwi"}, {U"ぐぇ", "gwe"}, {U"ぐぉ", "gwo"},
                    {U"すぁ", "swa"}, {U"すぃ", "swi"}, {U"すぇ", "swe"}, {U"すぉ", "swo"},
                    {U"ずぁ", "zwa"}, {U"ずぃ", "zwi"}, {U"ずぇ", "zwe"}, {U"ずぉ", "zwo"},
                    {U"つぁ", "tsa"}, {U"つぃ", "tsi"}, {U"つぇ", "tse"}, {U"つぉ", "tso"},
                    {U"てぃ", "ti"},  {U"てゅ", "tyu"}, {U"でぃ", "di"},  {U"でゅ", "dyu"},
                    {U"とぅ", "tu"},  {U"どぅ", "du"},
                    {U"ふぁ", "fa"},  {U"ふぃ", "fi"},  {U"ふぇ", "fe"},  {U"ふぉ", "fo"},
                    {U"ぶぁ", "bwa"}, {U"ぶぃ", "bwi"}, {U"ぶぇ", "bwe"}, {U"ぶぉ", "bwo"},
                    {U"ぷぁ", "pwa"}, {U"ぷぃ", "pwi"}, {U"ぷぇ", "pwe"}, {U"ぷぉ", "pwo"},
                    {U"ぬぁ", "nwa"}, {U"ぬぃ", "nwi"}, {U"ぬぇ", "nwe"}, {U"ぬぉ", "nwo"},
                    {U"むぁ", "mwa"}, {U"むぃ", "mwi"}, {U"むぇ", "mwe"}, {U"むぉ", "mwo"},
                    {U"るぁ", "rwa"}, {U"るぃ", "rwi"}, {U"るぇ", "rwe"}, {U"るぉ", "rwo"},
                    {U"ゔ", "vu"},
                    {U"ゔぁ", "va"},  {U"ゔぃ", "vi"},  {U"ゔぇ", "ve"},  {U"ゔぉ", "vo"},
                    {U"を", "o"},
                };
                return map;
            }
        };

        /**
         * English lexicon converter with an optional LSTM for out-of-vocabulary
         * words -- port of TIFA's ``LSTMConverter``.
         */
        class EnglishConverter final : public Converter {
        public:
            EnglishConverter(const PronMap *dictionary, const LstmG2P *oov, std::vector<std::string> wordLanguages)
                : m_dictionary(dictionary), m_oov(oov) {
                languages = std::move(wordLanguages);
            }

            bool find(const std::u32string &text, size_t &begin, size_t &end) const override {
                for (const auto &[left, right] : Text::wordSpans(text)) {
                    const std::u32string word = Text::lower(text.substr(left, right - left));
                    if (m_dictionary->count(Text::encode(word)) ||
                        (m_oov->available() && m_oov->usesCharacters(word))) {
                        begin = left;
                        end = right;
                        return true;
                    }
                }
                return false;
            }

            std::vector<G2PWord> convert(const std::u32string &text) const override {
                std::vector<G2PWord> result;
                for (const std::u32string &token : Text::splitWords(text)) {
                    const std::string key = Text::encode(token);
                    const auto it = m_dictionary->find(key);
                    const std::vector<std::vector<std::string>> *pronunciations =
                        it != m_dictionary->end() ? &it->second : nullptr;
                    if (pronunciations == nullptr) {
                        std::string error;
                        const std::vector<std::vector<std::string>> inferred = m_oov->predict(token, error);
                        if (!error.empty()) {
                            throw G2PError(error);
                        }
                        append(result, token, inferred);
                        continue;
                    }
                    append(result, token, *pronunciations);
                }
                return result;
            }

        private:
            static void append(std::vector<G2PWord> &result, const std::u32string &token,
                               const std::vector<std::vector<std::string>> &pronunciations) {
                G2PWord word;
                word.text = Text::encode(token);
                G2PReading reading;
                for (const std::vector<std::string> &phonemes : pronunciations) {
                    G2PPath path;
                    if (!phonemes.empty()) {
                        path.push_back(G2PGroup{word.text, phonemes});
                    }
                    reading.paths.push_back(std::move(path));
                }
                word.readings.push_back(std::move(reading));
                result.push_back(std::move(word));
            }

            const PronMap *m_dictionary;
            const LstmG2P *m_oov;
        };

        /**
         * Pronunciation dictionary lookup -- port of TIFA's ``DictionaryConverter``.
         *
         * ``word(N)`` and ``word (N)`` are treated as variants of the same word.
         */
        class DictionaryConverter final : public Converter {
        public:
            DictionaryConverter(const PronMap &raw, std::vector<std::string> wordLanguages) {
                languages = std::move(wordLanguages);
                const std::regex variant(R"(\s*\(\d+\)$)");
                for (const auto &[key, pronunciations] : raw) {
                    const std::string base = Text::encode(Text::lower(Text::decode(std::regex_replace(key, variant, ""))));
                    auto &entry = m_dictionary[base];
                    entry.insert(entry.end(), pronunciations.begin(), pronunciations.end());
                }
            }

            bool find(const std::u32string &text, size_t &begin, size_t &end) const override {
                for (const auto &[left, right] : Text::wordSpans(text)) {
                    const std::u32string word = Text::lower(text.substr(left, right - left));
                    if (m_dictionary.count(Text::encode(word))) {
                        begin = left;
                        end = right;
                        return true;
                    }
                }
                return false;
            }

            std::vector<G2PWord> convert(const std::u32string &text) const override {
                std::vector<G2PWord> result;
                for (const std::u32string &token : Text::splitWords(text)) {
                    const std::string key = Text::encode(Text::lower(token));
                    const auto it = m_dictionary.find(key);
                    if (it == m_dictionary.end()) {
                        throw G2PError("DictionaryConverter: token '" + Text::encode(token) +
                                       "' is not in the dictionary.");
                    }
                    G2PWord word;
                    word.text = Text::encode(token);
                    G2PReading reading;
                    for (const std::vector<std::string> &phonemes : it->second) {
                        G2PPath path;
                        if (!phonemes.empty()) {
                            path.push_back(G2PGroup{word.text, phonemes});
                        }
                        reading.paths.push_back(std::move(path));
                    }
                    word.readings.push_back(std::move(reading));
                    result.push_back(std::move(word));
                }
                return result;
            }

        private:
            PronMap m_dictionary;
        };

        /**
         * Final fallback -- port of TIFA's ``PassthroughConverter``. Every token
         * becomes its own phoneme, which the vocabulary check rejects in most
         * cases, so unmatched text is reported as out of vocabulary.
         */
        class PassthroughConverter final : public Converter {
        public:
            PassthroughConverter() {
                languages = {kAnyLanguage};
            }

            bool find(const std::u32string &text, size_t &begin, size_t &end) const override {
                const std::vector<Text::Range> spans = Text::wordSpans(text);
                if (spans.empty()) {
                    return false;
                }
                begin = spans.front().first;
                end = spans.front().second;
                return true;
            }

            std::vector<G2PWord> convert(const std::u32string &text) const override {
                std::vector<G2PWord> result;
                for (const std::u32string &token : Text::splitWords(text)) {
                    G2PWord word;
                    word.text = Text::encode(token);
                    G2PReading reading;
                    reading.paths.push_back({G2PGroup{word.text, {word.text}}});
                    word.readings.push_back(std::move(reading));
                    result.push_back(std::move(word));
                }
                return result;
            }
        };

        /** ``resolve_language``: keeps ANY for the vocabulary-based passthrough. */
        std::string resolveLanguage(const std::vector<std::string> &converterLanguages,
                                    const std::vector<std::string> &languages) {
            if (converterLanguages.empty()) {
                return std::string();
            }
            if (converterLanguages.size() == 1 && converterLanguages.front() == kAnyLanguage) {
                return kAnyLanguage;
            }
            if (languages.empty()) {
                return converterLanguages.front();
            }
            for (const std::string &tag : converterLanguages) {
                if (containsValue(languages, tag)) {
                    return tag;
                }
            }
            return std::string();
        }

    } // namespace

    bool PronunciationDictionary::load(const std::filesystem::path &path, std::string &error) {
        m_entries.clear();
        return loadPronunciationDictFile(path, m_entries, error);
    }

    void PronunciationDictionary::add(const std::string &key, std::vector<std::string> phonemes) {
        m_entries[key].push_back(std::move(phonemes));
    }

    const std::vector<std::vector<std::string>> *PronunciationDictionary::find(const std::string &key) const {
        const auto it = m_entries.find(key);
        return it != m_entries.end() ? &it->second : nullptr;
    }

    struct G2PPipeline::Impl {
        std::unique_ptr<Pinyin::Pinyin> mandarin;
        std::unique_ptr<Pinyin::Jyutping> jyutping;
        std::unique_ptr<LstmG2P> oov;
        /** Stand-in used when the OOV model is missing; reports itself unavailable. */
        LstmG2P unavailableOov;
        PronMap zhPhonemes;
        PronMap jaPhonemes;
        PronMap yuePhonemes;
        PronMap englishPhonemes;
        ChineseDicts mandarinDicts;
        ChineseDicts cantoneseDicts;
        std::vector<std::unique_ptr<Converter>> converters;
    };

    G2PPipeline::G2PPipeline() : m_impl(std::make_unique<Impl>()) {
    }

    G2PPipeline::~G2PPipeline() = default;

    bool G2PPipeline::init(const G2PResources &resources, std::string &error) {
        m_warnings.clear();
        m_availableLanguages.clear();
        m_impl->converters.clear();

        const std::filesystem::path &dictDir = resources.dictDir;
        const std::filesystem::path &pinyinDir = resources.pinyinDir;

        for (const auto &path : {dictDir / "ds-zh-pinyin-lite.txt", dictDir / "jyutping_dict.txt",
                                 dictDir / "japanese_dict_full.txt", dictDir / "ds_cmudict-07b.txt"}) {
            if (!std::filesystem::exists(path)) {
                error = "Missing G2P dictionary: " + pathToUtf8(path);
                return false;
            }
        }
        for (const auto &path : {pinyinDir / "mandarin" / "word.txt", pinyinDir / "mandarin" / "trans_word.txt",
                                 pinyinDir / "cantonese" / "word.txt", pinyinDir / "cantonese" / "trans_word.txt"}) {
            if (!std::filesystem::exists(path)) {
                error = "Missing cpp-pinyin dictionary: " + pathToUtf8(path) +
                        "\nThe dictionaries are installed next to the application in the dict folder.";
                return false;
            }
        }

        if (!loadPronunciationDictFile(dictDir / "ds-zh-pinyin-lite.txt", m_impl->zhPhonemes, error) ||
            !loadPronunciationDictFile(dictDir / "jyutping_dict.txt", m_impl->yuePhonemes, error) ||
            !loadPronunciationDictFile(dictDir / "japanese_dict_full.txt", m_impl->jaPhonemes, error) ||
            !loadPronunciationDictFile(dictDir / "ds_cmudict-07b.txt", m_impl->englishPhonemes, error)) {
            return false;
        }
        if (!m_impl->mandarinDicts.load(pinyinDir / "mandarin", error) ||
            !m_impl->cantoneseDicts.load(pinyinDir / "cantonese", error)) {
            return false;
        }

        try {
            m_impl->mandarin = std::make_unique<Pinyin::Pinyin>();
            m_impl->jyutping = std::make_unique<Pinyin::Jyutping>();
        } catch (const std::exception &e) {
            error = std::string("Failed to initialise the cpp-pinyin engine: ") + e.what();
            return false;
        }

        m_impl->oov = std::make_unique<LstmG2P>();
        std::string oovError;
        std::filesystem::path oovDir = dictDir / "LstmG2p-Eng";
        if (!std::filesystem::exists(oovDir)) {
            oovDir = dictDir.parent_path() / "assets" / "LstmG2p-Eng";
        }
        if (m_impl->oov->init(oovDir, oovError)) {
            m_impl->oov->beamSize = 16;
        } else {
            m_warnings.push_back("English out-of-vocabulary model not loaded (" + oovError +
                                 "). English words missing from the pronunciation dictionary fall through to the "
                                 "passthrough converter and are reported as out of vocabulary.");
            m_impl->oov.reset();
        }

        auto &converters = m_impl->converters;
        converters.push_back(std::make_unique<ChineseConverter>(false, m_impl->mandarin.get(), m_impl->jyutping.get(),
                                                                &m_impl->mandarinDicts, &m_impl->zhPhonemes,
                                                                std::vector<std::string>{"zh", "zho", "cmn"}));
        converters.push_back(std::make_unique<KanaConverter>(&m_impl->jaPhonemes,
                                                             std::vector<std::string>{"ja", "jpn"}));
        converters.push_back(std::make_unique<ChineseConverter>(true, m_impl->mandarin.get(), m_impl->jyutping.get(),
                                                                &m_impl->cantoneseDicts, &m_impl->yuePhonemes,
                                                                std::vector<std::string>{"yue"}));
        converters.push_back(std::make_unique<EnglishConverter>(
            &m_impl->englishPhonemes, m_impl->oov ? m_impl->oov.get() : &m_impl->unavailableOov,
            std::vector<std::string>{"en"}));
        converters.push_back(
            std::make_unique<DictionaryConverter>(m_impl->zhPhonemes, std::vector<std::string>{"zh"}));
        converters.push_back(
            std::make_unique<DictionaryConverter>(m_impl->jaPhonemes, std::vector<std::string>{"ja"}));
        converters.push_back(
            std::make_unique<DictionaryConverter>(m_impl->yuePhonemes, std::vector<std::string>{"yue"}));
        converters.push_back(std::make_unique<PassthroughConverter>());

        m_warnings.push_back("The Japanese converter has no MeCab backend: kana is supported, kanji is reported as "
                             "unsupported instead of being aligned with wrong phonemes.");

        m_availableLanguages = {"zh", "ja", "yue", "en"};
        return true;
    }

    bool G2PPipeline::convert(const std::u32string &text, const std::vector<std::string> &languages,
                              std::vector<G2PWord> &words, std::string &error) const {
        error.clear();
        std::vector<size_t> active;
        for (size_t i = 0; i < m_impl->converters.size(); ++i) {
            const std::vector<std::string> &tags = m_impl->converters[i]->languages;
            if (languages.empty() || tags.empty() ||
                (tags.size() == 1 && tags.front() == kAnyLanguage) ||
                std::any_of(tags.begin(), tags.end(),
                            [&languages](const std::string &tag) { return containsValue(languages, tag); })) {
                active.push_back(i);
            }
        }
        if (active.empty()) {
            error = "No converter matches the requested languages.";
            return false;
        }

        // filter-punctuation, strip-whitespace, lowercase
        std::vector<std::u32string> fragments = Text::filterPunctuation({text});
        fragments = Text::stripWhitespace(fragments);
        fragments = Text::lower(fragments);

        std::vector<std::pair<size_t, std::u32string>> claimed;
        std::vector<std::u32string> unrecognized;
        for (const std::u32string &fragment : fragments) {
            std::vector<std::tuple<size_t, size_t, size_t>> ranges;
            std::vector<std::tuple<size_t, size_t, size_t>> pending{{0, fragment.size(), 0}};
            while (!pending.empty()) {
                const auto [begin, end, priority] = pending.back();
                pending.pop_back();
                if (begin == end) {
                    continue;
                }
                const std::u32string part = fragment.substr(begin, end - begin);
                bool matched = false;
                for (size_t index = priority; index < active.size(); ++index) {
                    size_t left = 0;
                    size_t right = 0;
                    if (!m_impl->converters[active[index]]->find(part, left, right)) {
                        continue;
                    }
                    if (left >= right || right > part.size()) {
                        error = "A G2P converter returned an invalid range.";
                        return false;
                    }
                    ranges.emplace_back(begin + left, begin + right, index);
                    // LIFO order routes the left remainder before the right one.
                    pending.emplace_back(begin + right, end, index);
                    pending.emplace_back(begin, begin + left, index + 1);
                    matched = true;
                    break;
                }
                if (!matched) {
                    const bool blank = std::all_of(part.begin(), part.end(), [](char32_t c) { return Text::isSpace(c); });
                    if (!blank) {
                        unrecognized.push_back(part);
                    }
                }
            }
            std::sort(ranges.begin(), ranges.end());
            for (const auto &[begin, end, index] : ranges) {
                claimed.emplace_back(index, fragment.substr(begin, end - begin));
            }
        }

        if (!unrecognized.empty()) {
            std::string tokens;
            for (const std::u32string &part : unrecognized) {
                tokens += (tokens.empty() ? "" : ", ") + Text::encode(part);
            }
            error = "The following tokens could not be converted by any converter in the chain: " + tokens;
            return false;
        }

        words.clear();
        for (auto &[index, part] : claimed) {
            Converter &converter = *m_impl->converters[active[index]];
            const std::string language = resolveLanguage(converter.languages, languages);
            try {
                std::vector<G2PWord> converted = converter.convert(part);
                for (G2PWord &word : converted) {
                    word.language = language;
                }
                words.insert(words.end(), std::make_move_iterator(converted.begin()),
                             std::make_move_iterator(converted.end()));
            } catch (const std::exception &e) {
                error = e.what();
                return false;
            }
        }
        return true;
    }

} // namespace Tifa
