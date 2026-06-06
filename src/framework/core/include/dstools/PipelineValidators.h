#pragma once
/// @file PipelineValidators.h
/// @brief Backward compatibility shim. Use <dsfw/PipelineValidators.h> instead.

#include <dsfw/PipelineValidators.h>

namespace dstools {
    using dsfw::makeSliceLengthValidator;
    using dsfw::makeMinFieldValidator;
    using dsfw::makeMaxFieldValidator;
    using dsfw::makeRangeFieldValidator;
    using dsfw::makeAlignmentQualityValidator;
    using dsfw::makePitchCoverageValidator;
}