/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the Rmvpe namespace.

#ifndef RMVPEPROVIDER_H
#define RMVPEPROVIDER_H

#include <dstools/ExecutionProvider.h>

namespace Rmvpe
{
    using ExecutionProvider = dstools::infer::ExecutionProvider; ///< Execution provider alias.
}

#endif // RMVPEPROVIDER_H
