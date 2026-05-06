/// @file PathCompat.h
/// @brief Cross-platform file I/O helpers for correct Unicode path handling.
///
/// On Windows, path::string() returns ANSI-encoded strings that corrupt CJK
/// characters. These helpers use wstring/wchar_t APIs on Windows and UTF-8
/// string() on Unix, ensuring correct behavior on all platforms.

#pragma once

#include <filesystem>
#include <fstream>
#include <sndfile.hh>

#include <audio-util/AudioUtilGlobal.h>

namespace AudioUtil {

/// Open a SndfileHandle with correct encoding on all platforms.
/// On Windows: uses sf_wchar_open via wstring().c_str().
/// On Unix: uses string() (UTF-8).
inline SndfileHandle openSndfile(const std::filesystem::path &path,
                                  int mode = SFM_READ,
                                  int format = 0,
                                  int channels = 0,
                                  int samplerate = 0) {
#ifdef _WIN32
    return SndfileHandle(path.wstring().c_str(), mode, format, channels, samplerate);
#else
    return SndfileHandle(path.string(), mode, format, channels, samplerate);
#endif
}

/// Open a std::ifstream with correct encoding on all platforms.
/// MSVC's std::ifstream accepts std::filesystem::path directly.
/// On other compilers, uses path.string() (UTF-8 on Unix).
inline std::ifstream openIfstream(const std::filesystem::path &path,
                                   std::ios_base::openmode mode = std::ios_base::in) {
    return std::ifstream(path, mode);
}

/// Open a std::ofstream with correct encoding on all platforms.
inline std::ofstream openOfstream(const std::filesystem::path &path,
                                   std::ios_base::openmode mode = std::ios_base::out) {
    return std::ofstream(path, mode);
}

} // namespace AudioUtil
