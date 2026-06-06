#pragma once
/// @file Log.h
/// @brief Backward compatibility shim. Use <dsfw/Log.h> instead.

#include <dsfw/Log.h>

namespace dstools {
    using dsfw::Logger;
    using dsfw::LogLevel;
    using dsfw::LogEntry;
    using dsfw::LogSink;
}