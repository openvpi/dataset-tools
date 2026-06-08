#include <dstools/PinyinG2PProvider.h>

#include <algorithm>

namespace dstools {


dsfw::PinyinG2PProvider& PinyinG2PProvider::instance() {
    static dsfw::PinyinG2PProvider s_instance;
    return s_instance;
}

const char* PinyinG2PProvider::providerName() const {
    return "PinyinG2P";
}

dsfw::Result<std::vector<dsfw::G2PResult>> PinyinG2PProvider::convert(const std::string& text,
                                                                      const std::string& langCode) {
    if (langCode != "zh" && langCode != "zh-CN") {
        return dsfw::Err<std::vector<dsfw::G2PResult>>("PinyinG2P only supports Chinese (zh/zh-CN), got: " + langCode);
    }

    std::vector<dsfw::G2PResult> results;
    std::string current;

    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '\n') {
            if (!current.empty()) {
                dsfw::G2PResult r;
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
        dsfw::G2PResult r;
        r.word = current;
        r.phonemes = toPinyin(current);
        results.push_back(std::move(r));
    }

    return dsfw::Ok(std::move(results));
}

dsfw::Result<dsfw::G2PResult> PinyinG2PProvider::convertWord(const std::string& word, const std::string& langCode) {
    if (langCode != "zh" && langCode != "zh-CN") {
        return dsfw::Err<dsfw::G2PResult>("PinyinG2P only supports Chinese (zh/zh-CN), got: " + langCode);
    }

    dsfw::G2PResult result;
    result.word = word;
    result.phonemes = toPinyin(word);
    return dsfw::Ok(std::move(result));
}

std::vector<std::string> PinyinG2PProvider::toPinyin(const std::string& text) {
    (void)text;
    return {};
}

}  // namespace dstools
