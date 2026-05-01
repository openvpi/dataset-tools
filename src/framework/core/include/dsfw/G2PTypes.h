#pragma once

#include <string>
#include <vector>

namespace dstools {

struct G2PResult {
    std::string word;
    std::vector<std::string> phonemes;
};

} // namespace dstools
