#pragma once

/// @file JsonHelper.h
/// @brief Qt-extended JsonHelper — adds QString specializations on top of the
///        Qt-free base from dsfw-base.

// Include the base JsonHelper from dsfw-base using a relative path to avoid
// finding this file again (which would be a circular include due to #pragma once).
#include "../../base/include/dsfw/JsonHelper.h"

#include <QString>

namespace dstools {

template <>
inline QString JsonHelper::get(const nlohmann::json &root, const char *path, const QString &defaultValue) {
    try {
        const auto *node = resolve(root, path);
        if (!node) return defaultValue;
        if (node->is_string()) {
            return QString::fromStdString(node->get<std::string>());
        }
        return defaultValue;
    } catch (const std::exception &) {
        return defaultValue;
    }
}

template <>
inline QString JsonHelper::get(const nlohmann::json &root, const std::string &key, const QString &defaultValue) {
    try {
        if (!root.is_object() || !root.contains(key))
            return defaultValue;
        const auto &node = root[key];
        if (node.is_string()) {
            return QString::fromStdString(node.get<std::string>());
        }
        return defaultValue;
    } catch (const std::exception &) {
        return defaultValue;
    }
}

} // namespace dstools
