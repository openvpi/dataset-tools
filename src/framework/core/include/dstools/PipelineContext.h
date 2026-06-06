#pragma once
/// @file PipelineContext.h
/// @brief Backward compatibility shim. Use <dsfw/PipelineContext.h> instead.

#include <dsfw/PipelineContext.h>

namespace dstools {
    using dsfw::PipelineContext;
    using dsfw::StepRecord;
}