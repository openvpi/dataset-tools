#pragma once

#include <filesystem>
#include <string>

namespace dstools {

    [[deprecated("Use dsfw::PathUtils::toUtf8() instead")]]
    inline std::string pathToUtf8(const std::filesystem::path &path) {
        auto u8 = path.u8string();
        return std::string(u8.begin(), u8.end());
    }

    [[deprecated("Use dsfw::PathUtils::toWide() instead")]]
    inline std::wstring pathToWide(const std::filesystem::path &path) {
        return path.wstring();
    }

} // namespace dstools
