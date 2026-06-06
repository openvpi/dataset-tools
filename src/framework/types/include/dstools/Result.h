#pragma once
/// @file Result.h
/// @brief Backward compatibility shim. Use <dsfw/Result.h> instead.

#include <dsfw/Result.h>

namespace dstools {
    using dsfw::Result;
    using dsfw::Ok;
    using dsfw::Err;
}
