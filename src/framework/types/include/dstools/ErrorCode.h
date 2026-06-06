#pragma once
/// @file ErrorCode.h
/// @brief Backward compatibility shim. Use <dsfw/ErrorCode.h> instead.

#include <dsfw/ErrorCode.h>

namespace dstools {
    using dsfw::ErrorCode;
    using dsfw::errorCodeMessage;
}
