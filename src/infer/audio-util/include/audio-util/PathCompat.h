/// @file PathCompat.h
/// @brief Cross-platform file I/O helpers for correct Unicode path handling.
///
/// On Windows, path::string() returns ANSI-encoded strings that corrupt CJK
/// characters. These helpers use wstring/wchar_t APIs on Windows and UTF-8
/// string() on Unix, ensuring correct behavior on all platforms.

#pragma once

#ifdef _WIN32
#    include <windows.h>
#endif

#include <filesystem>
#include <fstream>
#include <sndfile.hh>

#include <audio-util/AudioUtilGlobal.h>
#include <dsfw/PathCompat.h>
#include <dsfw/PathUtils.h>

namespace AudioUtil
{

    /// Convert a filesystem path to a UTF-8 string that preserves Unicode
    /// characters (e.g. CJK) on all platforms.
    /// On Windows: uses WideCharToMultiByte(CP_UTF8).
    /// On Unix: uses path.u8string().
    [[deprecated("Use dsfw::PathUtils::toUtf8() instead")]]
    inline std::string pathToUtf8(const std::filesystem::path &p) {
#ifdef _WIN32
        if (p.empty())
            return {};
        std::wstring wstr = p.wstring();
        if (wstr.empty())
            return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                                       nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return {};
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                            result.data(), size, nullptr, nullptr);
        return result;
#else
        auto u8 = p.u8string();
        return std::string(u8.begin(), u8.end());
#endif
    }

    /// Open a SndfileHandle with correct encoding on all platforms.
    /// On Windows: uses sf_wchar_open via wstring().c_str().
    /// On Unix: uses string() (UTF-8).
    inline SndfileHandle openSndfile(const std::filesystem::path &path, int mode = SFM_READ, int format = 0,
                                     int channels = 0, int samplerate = 0) {
#ifdef _WIN32
        return SndfileHandle(dsfw::PathUtils::toWide(path).c_str(), mode, format, channels, samplerate);
#else
        return SndfileHandle(dsfw::PathUtils::toUtf8(path).c_str(), mode, format, channels, samplerate);
#endif
    }

    /// Open a std::ifstream with correct encoding on all platforms.
    /// Delegates to dsfw::openIfstream.
    [[deprecated("Use dsfw::openIfstream() instead")]]
    inline std::ifstream openIfstream(const std::filesystem::path &path,
                                      std::ios_base::openmode mode = std::ios_base::in) {
        return dsfw::openIfstream(path, mode);
    }

    /// Open a std::ofstream with correct encoding on all platforms.
    /// Delegates to dsfw::openOfstream.
    [[deprecated("Use dsfw::openOfstream() instead")]]
    inline std::ofstream openOfstream(const std::filesystem::path &path,
                                      std::ios_base::openmode mode = std::ios_base::out) {
        return dsfw::openOfstream(path, mode);
    }

} // namespace AudioUtil
