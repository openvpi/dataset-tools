#pragma once

#include <string>

namespace dstools {

struct ItemQualityReport {
    std::string sourceFile;
    int phonemeCount = 0;
    float durationSec = 0.0f;
};

} // namespace dstools
