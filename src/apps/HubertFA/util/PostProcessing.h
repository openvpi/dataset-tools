#pragma once

#include "AlignWord.h"
#include <string>

namespace HFA {
    constexpr float MIN_SP_LENGTH = 0.1f;

    void fillSmallGaps(const WordList &words, float wavLength);

    WordList addSP(const WordList &words, float wavLength, const std::string &addPhone = "SP");

} // namespace HFA