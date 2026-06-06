#pragma once
/// @file OnnxEnv.h
/// @brief Backward compatibility shim. Use <dsfw/infer/OnnxEnv.h> instead.

#include <dsfw/infer/OnnxEnv.h>

namespace dstools::infer {
    using dsfw::infer::OnnxEnv;
}