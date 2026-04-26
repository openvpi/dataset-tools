#pragma once

#include <filesystem>
#include <fstream>

namespace dsfw {

inline std::ifstream openIfstream(const std::filesystem::path &path,
                                   std::ios_base::openmode mode = std::ios_base::in) {
    return std::ifstream(path, mode);
}

inline std::ofstream openOfstream(const std::filesystem::path &path,
                                   std::ios_base::openmode mode = std::ios_base::out) {
    return std::ofstream(path, mode);
}

} // namespace dsfw