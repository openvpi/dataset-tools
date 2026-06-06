#pragma once
/// @file AppSettings.h
/// @brief Backward compatibility shim. Use <dsfw/AppSettings.h> instead.

#include <dsfw/AppSettings.h>

namespace dstools {
    using dsfw::AppSettings;
    using dsfw::SettingsKey;
}