/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the Rmvpe namespace.

#pragma once
#include <dstools/ExecutionProvider.h>

namespace Rmvpe
{
    using ExecutionProvider = dstools::infer::ExecutionProvider; ///< Execution provider alias.
}
