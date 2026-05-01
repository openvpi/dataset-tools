#pragma once

/// @file PathUtils.h
/// @brief Cross-platform path encoding utilities.

#include <QString>

#include <filesystem>

namespace dstools {

/// Convert QString to std::filesystem::path with correct encoding.
/// Uses wstring on Windows to handle Unicode paths safely.
inline std::filesystem::path toFsPath(const QString &qpath) {
#ifdef _WIN32
    return std::filesystem::path(qpath.toStdWString());
#else
    return std::filesystem::path(qpath.toStdString());
#endif
}

} // namespace dstools
