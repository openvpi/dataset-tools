#pragma once
/// @file TimePos.h
/// @brief Backward compatibility shim. Use <dsfw/TimePos.h> instead.

#include <dsfw/TimePos.h>

namespace dstools {
    using dsfw::TimePos;
    using dsfw::kMicrosecondsPerSecond;
    using dsfw::kMicrosecondsPerMs;
    using dsfw::secToUs;
    using dsfw::usToSec;
    using dsfw::usToMs;
    using dsfw::msToUs;
    using dsfw::hzToMhz;
    using dsfw::mhzToHz;
}
