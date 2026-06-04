/// @file SndfileHelper.h
/// @brief Cross-platform SndfileHandle helper for correct Unicode path handling.

#pragma once

#include <filesystem>

#include <sndfile.hh>

#include <dsfw/PathUtils.h>

namespace AudioUtil {

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

} // namespace AudioUtil