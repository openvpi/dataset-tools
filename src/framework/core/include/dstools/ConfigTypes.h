#pragma once
/// @file ConfigTypes.h
/// @brief Backward compatibility shim. Use <dsfw/ConfigTypes.h> instead.

#include <dsfw/ConfigTypes.h>

namespace dstools {
    using dsfw::ConfigValue;
    using dsfw::ConfigMap;
    using dsfw::configValueString;
    using dsfw::configValueInt;
    using dsfw::configValueBool;
    using dsfw::configValueDouble;
    using dsfw::configValueStringList;
}