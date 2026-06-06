#pragma once
/// @file PipelineRunner.h
/// @brief Backward compatibility shim. Use <dsfw/PipelineRunner.h> instead.

#include <dsfw/PipelineRunner.h>

namespace dstools {
    using dsfw::PipelineRunner;
    using dsfw::PipelineOptions;
    using dsfw::StepConfig;
    using dsfw::ValidationCallback;
}