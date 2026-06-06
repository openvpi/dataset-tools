#pragma once
/// @file ConfigTypesJson.h
/// @brief Backward compatibility shim. Use <dsfw/ConfigTypesJson.h> instead.

#include <dsfw/ConfigTypesJson.h>

namespace dstools {
    using dsfw::configMapFromJson;
    using dsfw::configMapToJson;
}