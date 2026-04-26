#pragma once

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