/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the HFA namespace.

#pragma once
#include <dstools/ExecutionProvider.h>

namespace HFA {
    using ExecutionProvider = dstools::infer::ExecutionProvider; ///< Execution provider alias.
}
