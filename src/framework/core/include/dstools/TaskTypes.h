#pragma once
/// @file TaskTypes.h
/// @brief Backward compatibility shim. Use <dsfw/TaskTypes.h> instead.

#include <dsfw/TaskTypes.h>

namespace dstools {
    using dsfw::ProcessorConfig;
    using dsfw::SlotSpec;
    using dsfw::TaskSpec;
    using dsfw::LayerData;
    using dsfw::TaskInput;
    using dsfw::TaskOutput;
}