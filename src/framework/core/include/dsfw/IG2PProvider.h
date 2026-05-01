#pragma once

#include <dsfw/G2PTypes.h>
#include <dstools/Result.h>

#include <string>
#include <vector>

namespace dstools {

class IG2PProvider {
public:
    virtual ~IG2PProvider() = default;

    virtual const char *providerName() const = 0;

    virtual Result<std::vector<G2PResult>> convert(const std::string &text,
                                                    const std::string &langCode) = 0;

    virtual Result<G2PResult> convertWord(const std::string &word,
                                           const std::string &langCode) = 0;
};

} // namespace dstools
