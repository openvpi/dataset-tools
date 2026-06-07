/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the HFA namespace.

#pragma once
#include <dsfw/infer/ExecutionProvider.h>

namespace HFA {
    using ExecutionProvider = dsfw::infer::ExecutionProvider; ///< Execution provider alias.
}
