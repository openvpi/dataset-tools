#pragma once
/// @file CurveTools.h
/// @brief Re-export of dsfw::signal::curve_tools functions for dstools domain layer.
/// @details All curve tool functions (resampleCurve, interpUnvoiced, movingAverage, dragBoundary, etc.)
///          are implemented in the dsfw-signal module. This header provides namespace-level using
///          declarations for convenience. Domain-layer pipeline logic should use these re-exports
///          rather than depending on dsfw::signal directly.

#include <dsfw/signal/curve_tools.h>

namespace dstools {
    using dsfw::signal::alignToLength;
    using dsfw::signal::expectedFrameCount;
    using dsfw::signal::hopSizeToTimestep;
    using dsfw::signal::hzToMhzBatch;
    using dsfw::signal::hzToMidiBatch;
    using dsfw::signal::interpUnvoiced;
    using dsfw::signal::interpUnvoicedF;
    using dsfw::signal::medianFilter;
    using dsfw::signal::mhzToHzBatch;
    using dsfw::signal::mhzToMidiBatch;
    using dsfw::signal::midiToHzBatch;
    using dsfw::signal::movingAverage;
    using dsfw::signal::resampleCurve;
    using dsfw::signal::resampleCurveF;
    using dsfw::signal::smoothstepCrossfade;
} // namespace dstools