#pragma once
/// @file OnnxModelBase.h
/// @brief Backward compatibility shim. Use <dsfw/infer/OnnxModelBase.h> instead.

#include <dsfw/infer/OnnxModelBase.h>

namespace dstools::infer {
    using dsfw::infer::OnnxModelBase;
    using dsfw::infer::CancellableOnnxModel;
}