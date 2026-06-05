#pragma once
/// @file IInferenceEngine.h
/// @brief Backward compatibility shim. Use <dsfw/infer/IInferenceEngine.h> instead.

#include <dsfw/infer/IInferenceEngine.h>

namespace dstools::infer {
    using dsfw::infer::IInferenceEngine;
}
