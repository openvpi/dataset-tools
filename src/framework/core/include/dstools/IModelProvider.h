#pragma once
/// @file IModelProvider.h
/// @brief Backward compatibility shim. Use <dsfw/IModelProvider.h> instead.

#include <dsfw/IModelProvider.h>

namespace dstools {
    using dsfw::IModelProvider;
    using dsfw::ModelTypeId;
    using dsfw::ModelStatus;
    using dsfw::ModelProgressCallback;
    using dsfw::registerModelType;
}