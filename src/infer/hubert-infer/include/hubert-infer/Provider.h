/// @file Provider.h
/// @brief Re-exports ExecutionProvider into the HFA namespace.

#ifndef HFAPROVIDER_H
#define HFAPROVIDER_H

#include <dstools/ExecutionProvider.h>

namespace HFA {
    using ExecutionProvider = dstools::infer::ExecutionProvider; ///< Execution provider alias.
}

#endif // HFAPROVIDER_H
