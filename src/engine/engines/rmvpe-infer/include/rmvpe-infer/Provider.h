/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the Rmvpe namespace.

#pragma once
#include <dsfw/infer/ExecutionProvider.h>

namespace Rmvpe
{
    using ExecutionProvider = dsfw::infer::ExecutionProvider; ///< Execution provider alias.
}
