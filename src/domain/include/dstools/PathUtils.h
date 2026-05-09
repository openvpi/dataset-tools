#pragma once

#include <dsfw/PathUtils.h>

#include <QString>

#include <filesystem>

namespace dstools {

[[deprecated("Use dsfw::PathUtils::toStdPath() instead")]]
inline std::filesystem::path toFsPath(const QString &qpath) {
    return dsfw::PathUtils::toStdPath(qpath);
}

} // namespace dstools
