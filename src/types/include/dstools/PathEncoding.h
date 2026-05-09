#pragma once

#include <filesystem>
#include <string>

namespace dstools {

inline std::string pathToUtf8(const std::filesystem::path &path) {
    auto u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

inline std::wstring pathToWide(const std::filesystem::path &path) {
    return path.wstring();
}

} // namespace dstools
