#pragma once
/// @file F0Curve.h
/// @brief Re-export of dsfw::signal::F0Curve (TimeSeries<float>) for dstools domain layer.
/// @details The actual F0Curve implementation lives in dsfw-signal (dsfw::signal::TimeSeries<T>).
///          This header provides a namespace alias for convenience. Pipeline logic in dstools-domain
///          should NOT depend on F0Curve internals; use the public TimeSeries API only.

#include <dsfw/signal/time_series.h>

namespace dstools {
    using F0Curve = dsfw::signal::F0Curve;
} // namespace dstools