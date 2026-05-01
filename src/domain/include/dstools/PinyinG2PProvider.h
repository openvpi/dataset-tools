#pragma once

#include <dsfw/IG2PProvider.h>

#include <string>
#include <vector>

namespace dstools {

class PinyinG2PProvider : public IG2PProvider {
public:
    PinyinG2PProvider() = default;
    ~PinyinG2PProvider() override = default;

    const char *providerName() const override;

    Result<std::vector<G2PResult>> convert(const std::string &text,
                                            const std::string &langCode) override;

    Result<G2PResult> convertWord(const std::string &word,
                                   const std::string &langCode) override;

private:
    static std::vector<std::string> toPinyin(const std::string &text);
};

} // namespace dstools
