#include <dstools/PinyinG2PProvider.h>

#include <algorithm>

namespace dstools {

    const char *PinyinG2PProvider::providerName() const {
        return "PinyinG2P";
    }

    Result<std::vector<G2PResult>> PinyinG2PProvider::convert(const std::string &text,
                                                               const std::string &langCode) {
        if (langCode != "zh" && langCode != "zh-CN") {
            return Err<std::vector<G2PResult>>("PinyinG2P only supports Chinese (zh/zh-CN), got: " + langCode);
        }

        std::vector<G2PResult> results;
        std::string current;

        for (char c : text) {
            if (c == ' ' || c == '\t' || c == '\n') {
                if (!current.empty()) {
                    G2PResult r;
                    r.word = current;
                    r.phonemes = toPinyin(current);
                    results.push_back(std::move(r));
                    current.clear();
                }
            } else {
                current += c;
            }
        }

        if (!current.empty()) {
            G2PResult r;
            r.word = current;
            r.phonemes = toPinyin(current);
            results.push_back(std::move(r));
        }

        return Ok(std::move(results));
    }

    Result<G2PResult> PinyinG2PProvider::convertWord(const std::string &word,
                                                      const std::string &langCode) {
        if (langCode != "zh" && langCode != "zh-CN") {
            return Err<G2PResult>("PinyinG2P only supports Chinese (zh/zh-CN), got: " + langCode);
        }

        G2PResult result;
        result.word = word;
        result.phonemes = toPinyin(word);
        return Ok(std::move(result));
    }

    std::vector<std::string> PinyinG2PProvider::toPinyin(const std::string &text) {
        (void)text;
        return {};
    }

} // namespace dstools
